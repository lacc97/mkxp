/*
** filesystem.cpp
**
** This file is part of mkxp.
**
** Copyright (C) 2013 Jonas Kulla <Nyocurio@gmail.com>
**
** mkxp is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 2 of the License, or
** (at your option) any later version.
**
** mkxp is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with mkxp.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "filesystem.h"

#include "rgssad.h"
#include "font.h"
#include "util.h"
#include "exception.h"
#include "sharedstate.h"
#include "boost-hash.h"
#include "debugwriter.h"

#include <mio/shared_mmap.hpp>

#include <physfs.h>

#include <SDL_sound.h>

#include <stdio.h>
#include <cstring>
#include <algorithm>
#include <vector>
#include <stack>

#include <memory>
#include <optional>

#ifdef __APPLE__
#include <iconv.h>
#endif

namespace {
  std::optional<mio::shared_mmap_source> map_file(const char* filename) noexcept {
    mio::shared_mmap_source mapped;

    std::error_code ec;
    mapped.map(filename, ec);
    if(ec)
      return {};

    return mapped;
  }

  PHYSFS_Io* create_io(mio::shared_mmap_source src) noexcept {
    struct opaque {
      mio::shared_mmap_source src;

      const char* beg{nullptr};
      const char* cur{nullptr};
      const char* end{nullptr};
    };

    auto io = std::make_unique<PHYSFS_Io>();
    io->read = [](PHYSFS_Io *io, void *buf, PHYSFS_uint64 len) noexcept -> PHYSFS_sint64 {
      auto* data = static_cast<struct opaque*>(io->opaque);

      const auto read_count = std::min<size_t>(len, data->end - data->cur);

      std::memcpy(buf, data->cur, read_count);
      data->cur += read_count;

      return read_count;
    };
    io->seek = [](PHYSFS_Io *io, PHYSFS_uint64 offset) noexcept -> int {
      auto* data = static_cast<struct opaque*>(io->opaque);

      if(offset > (data->end - data->beg))
        return false;

      data->cur = data->beg + offset;
      return true;
    };
    io->tell = [](PHYSFS_Io* io) noexcept -> PHYSFS_sint64 {
      auto* data = static_cast<struct opaque*>(io->opaque);

      return (data->cur - data->beg);
    };
    io->length = [](PHYSFS_Io* io) noexcept -> PHYSFS_sint64 {
      auto* data = static_cast<struct opaque*>(io->opaque);

      return (data->end - data->beg);
    };
    io->duplicate = [](PHYSFS_Io* io) noexcept -> PHYSFS_Io* {
      auto* data = static_cast<struct opaque*>(io->opaque);

      auto new_io = std::make_unique<PHYSFS_Io>(*io);
      new_io->opaque = new opaque{*data};

      return new_io.release();
    };
    io->destroy = [](PHYSFS_Io* io) noexcept {
      delete static_cast<struct opaque*>(io->opaque);
      delete io;
    };

    auto data = std::make_unique<opaque>();
    {
      data->src = std::move(src);
      data->beg = data->cur = data->src.data();
      data->end = data->beg + data->src.size();
    }
    io->opaque = data.release();

    return io.release();
  }
}

static inline PHYSFS_File *sdlPHYS(SDL_RWops *ops)
{
	return static_cast<PHYSFS_File*>(ops->hidden.unknown.data1);
}

static Sint64 SDL_RWopsSize(SDL_RWops *ops)
{
	PHYSFS_File *f = sdlPHYS(ops);

	if (!f)
		return -1;

	return PHYSFS_fileLength(f);
}

static Sint64 SDL_RWopsSeek(SDL_RWops *ops, int64_t offset, int whence)
{
	PHYSFS_File *f = sdlPHYS(ops);

	if (!f)
		return -1;

	int64_t base;

	switch (whence)
	{
	default:
	case RW_SEEK_SET :
		base = 0;
		break;
	case RW_SEEK_CUR :
		base = PHYSFS_tell(f);
		break;
	case RW_SEEK_END :
		base = PHYSFS_fileLength(f);
		break;
	}

	int result = PHYSFS_seek(f, base + offset);

	return (result != 0) ? PHYSFS_tell(f) : -1;
}

static size_t SDL_RWopsRead(SDL_RWops *ops, void *buffer, size_t size, size_t maxnum)
{
	PHYSFS_File *f = sdlPHYS(ops);

	if (!f)
		return 0;

	PHYSFS_sint64 result = PHYSFS_readBytes(f, buffer, size*maxnum);

	return (result != -1) ? (result / size) : 0;
}

static size_t SDL_RWopsWrite(SDL_RWops *ops, const void *buffer, size_t size, size_t num)
{
	PHYSFS_File *f = sdlPHYS(ops);

	if (!f)
		return 0;

	PHYSFS_sint64 result = PHYSFS_writeBytes(f, buffer, size*num);

	return (result != -1) ? (result / size) : 0;
}

static int SDL_RWopsClose(SDL_RWops *ops)
{
	PHYSFS_File *f = sdlPHYS(ops);

	if (!f)
		return -1;

	int result = PHYSFS_close(f);
	ops->hidden.unknown.data1 = 0;

	return (result != 0) ? 0 : -1;
}

static int SDL_RWopsCloseFree(SDL_RWops *ops)
{
	int result = SDL_RWopsClose(ops);

	SDL_FreeRW(ops);

	return result;
}

/* Copies the first srcN characters from src into dst,
 * or the full string if srcN == -1. Never writes more
 * than dstMax, and guarantees dst to be null terminated.
 * Returns copied bytes (minus terminating null) */
static size_t
strcpySafe(char *dst, const char *src,
           size_t dstMax, int srcN)
{
	if (srcN < 0)
		srcN = strlen(src);

	size_t cpyMax = std::min<size_t>(dstMax-1, srcN);

	memcpy(dst, src, cpyMax);
	dst[cpyMax] = '\0';

	return cpyMax;
}

/* Attempt to locate an extension string in a filename.
 * Either a pointer into the input string pointing at the
 * extension, or null is returned */
static const char *
findExt(const char *filename)
{
	size_t len;

	for (len = strlen(filename); len > 0; --len)
	{
		if (filename[len] == '/')
			return 0;

		if (filename[len] == '.')
			return &filename[len+1];
	}

	return 0;
}

static void
initReadOps(PHYSFS_File *handle,
            SDL_RWops &ops,
            bool freeOnClose)
{
	ops.size  = SDL_RWopsSize;
	ops.seek  = SDL_RWopsSeek;
	ops.read  = SDL_RWopsRead;
	ops.write = SDL_RWopsWrite;

	if (freeOnClose)
		ops.close = SDL_RWopsCloseFree;
	else
		ops.close = SDL_RWopsClose;

	ops.type = SDL_RWOPS_PHYSFS;
	ops.hidden.unknown.data1 = handle;
}

static void strTolower(std::string &str)
{
	for (size_t i = 0; i < str.size(); ++i)
		str[i] = tolower(str[i]);
}

const Uint32 SDL_RWOPS_PHYSFS = SDL_RWOPS_UNKNOWN+10;

struct FileSystemPrivate
{
	/* Maps: lower case full filepath,
	 * To:   mixed case full filepath */
	BoostHash<std::string, std::string> pathCache;
	/* Maps: lower case directory path,
	 * To:   list of lower case filenames */
	BoostHash<std::string, std::vector<std::string> > fileLists;

	/* This is for compatibility with games that take Windows'
	 * case insensitivity for granted */
	bool havePathCache;
};

static void throwPhysfsError(const char *desc)
{
	PHYSFS_ErrorCode ec = PHYSFS_getLastErrorCode();
	const char *englishStr = PHYSFS_getErrorByCode(ec);

	throw Exception(Exception::PHYSFSError, "%s: %s", desc, englishStr);
}

FileSystem::FileSystem(const char *argv0,
                       bool allowSymlinks)
{
	if (PHYSFS_init(argv0) == 0)
		throwPhysfsError("Error initializing PhysFS");

	/* One error (=return 0) turns the whole product to 0 */
	int er = 1;
	er *= PHYSFS_registerArchiver(&RGSS1_Archiver);
	er *= PHYSFS_registerArchiver(&RGSS2_Archiver);
	er *= PHYSFS_registerArchiver(&RGSS3_Archiver);

	if (er == 0)
		throwPhysfsError("Error registering PhysFS RGSS archiver");

	p = new FileSystemPrivate;
	p->havePathCache = false;

	if (allowSymlinks)
		PHYSFS_permitSymbolicLinks(1);
}

FileSystem::~FileSystem()
{
	delete p;

	if (PHYSFS_deinit() == 0)
		Debug() << "PhyFS failed to deinit.";
}

void FileSystem::addPath(const char *path)
{
  if(auto o_mapped = map_file(path); o_mapped)
    PHYSFS_mountIo(create_io(*o_mapped), path, nullptr,true);
  else
    PHYSFS_mount(path, nullptr, true);
}

struct CacheEnumData
{
	FileSystemPrivate *p;
	std::stack<std::vector<std::string>*> fileLists;

#ifdef __APPLE__
	iconv_t nfd2nfc;
	char buf[512];
#endif

	CacheEnumData(FileSystemPrivate *p)
	    : p(p)
	{
#ifdef __APPLE__
		nfd2nfc = iconv_open("utf-8", "utf-8-mac");
#endif
	}

	~CacheEnumData()
	{
#ifdef __APPLE__
		iconv_close(nfd2nfc);
#endif
	}

	/* Converts in-place */
	void toNFC(char *inout)
	{
#ifdef __APPLE__
		size_t srcSize = strlen(inout);
		size_t bufSize = sizeof(buf);
		char *bufPtr = buf;
		char *inoutPtr = inout;

		/* Reserve room for null terminator */
		--bufSize;

		iconv(nfd2nfc,
			  &inoutPtr, &srcSize,
			  &bufPtr, &bufSize);
		/* Null-terminate */
		*bufPtr = 0;
		strcpy(inout, buf);
#else
		(void) inout;
#endif
	}
};

static PHYSFS_EnumerateCallbackResult
cacheEnumCB(void *d, const char *origdir, const char *fname)
{
	CacheEnumData &data = *static_cast<CacheEnumData*>(d);
	char fullPath[512];

	if (!*origdir)
		snprintf(fullPath, sizeof(fullPath), "%s", fname);
	else
		snprintf(fullPath, sizeof(fullPath), "%s/%s", origdir, fname);

	/* Deal with OSX' weird UTF-8 standards */
	data.toNFC(fullPath);

	std::string mixedCase(fullPath);
	std::string lowerCase = mixedCase;
	strTolower(lowerCase);

	PHYSFS_Stat stat;
	PHYSFS_stat(fullPath, &stat);

	if (stat.filetype == PHYSFS_FILETYPE_DIRECTORY)
	{
		/* Create a new list for this directory */
		std::vector<std::string> &list = data.p->fileLists[lowerCase];

		/* Iterate over its contents */
		data.fileLists.push(&list);
		PHYSFS_enumerate(fullPath, cacheEnumCB, d);
		data.fileLists.pop();
	}
	else
	{
		/* Get the file list for the directory we're currently
		 * traversing and append this filename to it */
		std::vector<std::string> &list = *data.fileLists.top();
		std::string lowerFilename(fname);
		strTolower(lowerFilename);
		list.push_back(lowerFilename);

		/* Add the lower -> mixed mapping of the file's full path */
		data.p->pathCache.insert(lowerCase, mixedCase);
	}

	return PHYSFS_ENUM_OK;
}

void FileSystem::createPathCache()
{
	CacheEnumData data(p);
	data.fileLists.push(&p->fileLists[""]);
	PHYSFS_enumerate("", cacheEnumCB, &data);

	p->havePathCache = true;
}

struct FontSetsCBData
{
	FileSystemPrivate *p;
	SharedFontState *sfs;
};

static PHYSFS_EnumerateCallbackResult
fontSetEnumCB (void *data, const char *dir, const char *fname)
{
	FontSetsCBData *d = static_cast<FontSetsCBData*>(data);

	/* Only consider filenames with font extensions */
	const char *ext = findExt(fname);

	if (!ext)
		return PHYSFS_ENUM_OK;

	char lowExt[8];
	size_t i;

	for (i = 0; i < sizeof(lowExt)-1 && ext[i]; ++i)
		lowExt[i] = tolower(ext[i]);
	lowExt[i] = '\0';

	if (strcmp(lowExt, "ttf") && strcmp(lowExt, "otf"))
		return PHYSFS_ENUM_OK;

	char filename[512];
	snprintf(filename, sizeof(filename), "%s/%s", dir, fname);

	PHYSFS_File *handle = PHYSFS_openRead(filename);

	if (!handle)
		return PHYSFS_ENUM_ERROR;

	SDL_RWops ops;
	initReadOps(handle, ops, false);

	d->sfs->initFontSetCB(ops, filename);

	SDL_RWclose(&ops);

	return PHYSFS_ENUM_OK;
}

/* Basically just a case-insensitive search
 * for the folder "Fonts"... */
static PHYSFS_EnumerateCallbackResult
findFontsFolderCB(void *data, const char *, const char *fname)
{
	size_t i = 0;
	char buffer[512];
	const char *s = fname;

	while (*s && i < sizeof(buffer))
		buffer[i++] = tolower(*s++);

	buffer[i] = '\0';

	if (strcmp(buffer, "fonts") == 0)
		PHYSFS_enumerate(fname, fontSetEnumCB, data);

	return PHYSFS_ENUM_OK;
}

void FileSystem::initFontSets(SharedFontState &sfs)
{
	FontSetsCBData d = { p, &sfs };

	PHYSFS_enumerate("", findFontsFolderCB, &d);
}

struct OpenReadEnumData
{
	FileSystem::OpenHandler &handler;
	SDL_RWops ops;

	/* The filename (without directory) we're looking for */
	const char *filename;
	size_t filenameN;

	/* Optional hash to translate full filepaths
	 * (used with path cache) */
	BoostHash<std::string, std::string> *pathTrans;

	/* Number of files we've attempted to read and parse */
	size_t matchCount;
	bool stopSearching;

	/* In case of a PhysFS error, save it here so it
	 * doesn't get changed before we get back into our code */
	const char *physfsError;

	OpenReadEnumData(FileSystem::OpenHandler &handler,
	                 const char *filename, size_t filenameN,
	                 BoostHash<std::string, std::string> *pathTrans)
	    : handler(handler), filename(filename), filenameN(filenameN),
	      pathTrans(pathTrans), matchCount(0), stopSearching(false),
	      physfsError(0)
	{}
};

static PHYSFS_EnumerateCallbackResult
openReadEnumCB(void *d, const char *dirpath, const char *filename)
{
	OpenReadEnumData &data = *static_cast<OpenReadEnumData*>(d);
	char buffer[512];
	const char *fullPath;

	if (data.stopSearching)
		return PHYSFS_ENUM_STOP;

	/* If there's not even a partial match, continue searching */
	if (strncmp(filename, data.filename, data.filenameN) != 0)
		return PHYSFS_ENUM_OK;

	if (!*dirpath)
	{
		fullPath = filename;
	}
	else
	{
		snprintf(buffer, sizeof(buffer), "%s/%s", dirpath, filename);
		fullPath = buffer;
	}

	char last = filename[data.filenameN];

	/* If fname matches up to a following '.' (meaning the rest is part
	 * of the extension), or up to a following '\0' (full match), we've
	 * found our file */
	if (last != '.' && last != '\0')
		return PHYSFS_ENUM_STOP;

	/* If the path cache is active, translate from lower case
	 * to mixed case path */
	if (data.pathTrans)
		fullPath = (*data.pathTrans)[fullPath].c_str();

	PHYSFS_File *phys = PHYSFS_openRead(fullPath);

	if (!phys)
	{
		/* Failing to open this file here means there must
		 * be a deeper rooted problem somewhere within PhysFS.
		 * Just abort alltogether. */
		data.stopSearching = true;
		data.physfsError = PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());

		return PHYSFS_ENUM_ERROR;
	}

	initReadOps(phys, data.ops, false);

	const char *ext = findExt(filename);

	if (data.handler.tryRead(data.ops, ext))
		data.stopSearching = true;

	++data.matchCount;
	return PHYSFS_ENUM_OK;
}

void FileSystem::openRead(OpenHandler &handler, const char *filename)
{
	char buffer[512];
	size_t len = strcpySafe(buffer, filename, sizeof(buffer), -1);
	char *delim;

	if (p->havePathCache)
		for (size_t i = 0; i < len; ++i)
			buffer[i] = tolower(buffer[i]);

	/* Find the deliminator separating directory and file name */
	for (delim = buffer + len; delim > buffer; --delim)
		if (*delim == '/')
			break;

	const bool root = (delim == buffer);

	const char *file = buffer;
	const char *dir = "";

	if (!root)
	{
		/* Cut the buffer in half so we can use it
		 * for both filename and directory path */
		*delim = '\0';
		file = delim+1;
		dir = buffer;
	}

	OpenReadEnumData data(handler, file, len + buffer - delim - !root,
	                      p->havePathCache ? &p->pathCache : 0);

	if (p->havePathCache)
	{
		/* Get the list of files contained in this directory
		 * and manually iterate over them */
		const std::vector<std::string> &fileList = p->fileLists[dir];

		for (size_t i = 0; i < fileList.size(); ++i)
			openReadEnumCB(&data, dir, fileList[i].c_str());
	}
	else
	{
		PHYSFS_enumerate(dir, openReadEnumCB, &data);
	}

	if (data.physfsError)
		throw Exception(Exception::PHYSFSError, "PhysFS: %s", data.physfsError);

	if (data.matchCount == 0)
		throw Exception(Exception::NoFileError, "%s", filename);
}

void FileSystem::openReadRaw(SDL_RWops &ops,
                             const char *filename,
                             bool freeOnClose)
{
	PHYSFS_File *handle = PHYSFS_openRead(filename);
  if(!handle) {
    auto ec = PHYSFS_getLastErrorCode();
    if(ec == PHYSFS_ERR_NOT_FOUND)
      throw Exception(Exception::NoFileError, "file not found: %s", filename);
    else
      throw Exception(Exception::PHYSFSError, "PhysFS error: %d", ec);
  }

	initReadOps(handle, ops, freeOnClose);
}

bool FileSystem::exists(const char *filename)
{
	return PHYSFS_exists(filename);
}
