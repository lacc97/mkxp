/*
** soundemitter.cpp
**
** This file is part of mkxp.
**
** Copyright (C) 2014 Jonas Kulla <Nyocurio@gmail.com>
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

#include "soundemitter.h"

#include <ranges>

#include "audio_thread.h"
#include "config.h"
#include "debugwriter.h"
#include "filesystem.h"
#include "sharedstate.h"

#include <SDL_sound.h>

#define SE_CACHE_MEM (10*1024*1024) // 10 MB

namespace {
  struct sound_open_handle : FileSystem::OpenHandler {
    bool tryRead(SDL_RWops& ops, const char* ext) override {
      Sound_Sample *sample = Sound_NewSample(&ops, ext, 0, STREAM_BUF_SIZE);

      if (!sample)
      {
        SDL_RWclose(&ops);
        return false;
      }

      /* Do all of the decoding in the handler so we don't have
       * to keep the source ops around */
      uint32_t decBytes = Sound_DecodeAll(sample);
      uint8_t sampleSize = formatSampleSize(sample->actual.format);
      uint32_t sampleCount = decBytes / sampleSize;

      al_buffer = mkxp::al::buffer::create();
      bytes_size = sampleSize * sampleCount;

      ALenum alFormat = chooseALFormat(sampleSize, sample->actual.channels);

      al_buffer.upload_data(alFormat, sample->buffer, bytes_size, sample->actual.rate);

      Sound_FreeSample(sample);

      return true;
    }

    mkxp::al::buffer al_buffer;
    size_t bytes_size;
  };
}

mkxp::sound_emitter::sound_emitter(const Config& conf) noexcept : m_sources(conf.SE.sourceCount), m_buffer_cache{SE_CACHE_MEM} {
  for(auto& src : m_sources)
    src = {al::source::create(), {}};
}

mkxp::sound_emitter::~sound_emitter() {
  for(auto& src : m_sources) {
    src.al_source.stop();
    src.al_source.destroy();
  }
}

auto mkxp::sound_emitter::acquire_source(const char* filename) noexcept -> cppcoro::task<source> {
  auto p_buf = std::shared_ptr<buffer>{};
  if(auto* p_elem = m_buffer_cache.get(filename)) {
    p_buf = *p_elem;
  } else {
    auto handler = sound_open_handle{};
    shState->fileSystem().openRead(handler, filename);
    if (!handler.al_buffer)
    {
      Debug() << fmt::format("Unable to decode sound: {}: {}", filename, Sound_GetError());
      co_return source{};
    }

    p_buf = std::shared_ptr<buffer>{new buffer{handler.al_buffer, handler.bytes_size}, [](buffer* buf) { buf->al_buffer.destroy(); delete buf; }};
    m_buffer_cache.put(filename, p_buf);
  }

  /* We have our buffer. Now we find our source. */
  auto src = source{};
  if(auto it = std::ranges::find_if(m_sources, [](const source& src) -> bool { return !src.p_buffer || src.al_source.get_state() != AL_PLAYING; });
     it != m_sources.end()) {
    /* Found a source that is not playing anything. */

    m_sources.splice(m_sources.end(), m_sources, it);
    src = *it;
  } else if(it = std::ranges::find_if(m_sources, [&p_buf](const source& src) -> bool { return src.p_buffer == p_buf; }); it != m_sources.end()) {
    /* Found a source that is playing the same thing we want to play. */

    m_sources.splice(m_sources.end(), m_sources, it);
    src = *it;
  } else {
    /* Pick the lowest priority source. */

    m_sources.splice(m_sources.end(), m_sources, m_sources.begin());
    src = m_sources.back();
  }

  src.al_source.stop();
  if(src.p_buffer != p_buf) {
    src.al_source.detach_buffer();
    src.p_buffer = std::move(p_buf);
    src.al_source.attach_buffer(src.p_buffer->al_buffer);
  }

  co_return src;
}

auto mkxp::sound_emitter::play(const char* filename, int volume, int pitch) noexcept -> cppcoro::task<> {
  auto src = co_await acquire_source(filename);

  src.al_source.set_volume(std::clamp<int>(volume, 0, 100) / 100.0f);
  src.al_source.set_pitch(std::clamp<int>(pitch, 50, 150) / 100.0f);
  src.al_source.play();
}

//struct SoundBuffer
//{
//	/* Uniquely identifies this or equal buffer */
//	std::string key;
//
//	AL::Buffer::ID alBuffer;
//
//	/* Link into the buffer cache priority list */
//	IntruListLink<SoundBuffer> link;
//
//	/* Buffer byte count */
//	uint32_t bytes;
//
//	/* Reference count */
//	uint8_t refCount;
//
//	SoundBuffer()
//	    : link(this),
//	      refCount(1)
//
//	{
//		alBuffer = AL::Buffer::gen();
//	}
//
//	static SoundBuffer *ref(SoundBuffer *buffer)
//	{
//		++buffer->refCount;
//
//		return buffer;
//	}
//
//	static void deref(SoundBuffer *buffer)
//	{
//		if (--buffer->refCount == 0)
//			delete buffer;
//	}
//
//private:
//	~SoundBuffer()
//	{
//		AL::Buffer::del(alBuffer);
//	}
//};
//
///* Before: [a][b][c][d], After (index=1): [a][c][d][b] */
//static void
//arrayPushBack(std::vector<size_t> &array, size_t size, size_t index)
//{
//	size_t v = array[index];
//
//	for (size_t t = index; t < size-1; ++t)
//		array[t] = array[t+1];
//
//	array[size-1] = v;
//}
//
//SoundEmitter::SoundEmitter(const Config &conf)
//    : bufferBytes(0),
//      srcCount(conf.SE.sourceCount),
//      alSrcs(srcCount),
//      atchBufs(srcCount),
//      srcPrio(srcCount)
//{
//	for (size_t i = 0; i < srcCount; ++i)
//	{
//		alSrcs[i] = AL::Source::gen();
//		atchBufs[i] = 0;
//		srcPrio[i] = i;
//	}
//}
//
//SoundEmitter::~SoundEmitter()
//{
//	for (size_t i = 0; i < srcCount; ++i)
//	{
//		AL::Source::stop(alSrcs[i]);
//		AL::Source::del(alSrcs[i]);
//
//		if (atchBufs[i])
//			SoundBuffer::deref(atchBufs[i]);
//	}
//
//	BufferHash::const_iterator iter;
//	for (iter = bufferHash.cbegin(); iter != bufferHash.cend(); ++iter)
//		SoundBuffer::deref(iter->second);
//}
//
//void SoundEmitter::play(const std::string &filename,
//                        int volume,
//                        int pitch)
//{
//	float _volume = clamp<int>(volume, 0, 100) / 100.0f;
//	float _pitch  = clamp<int>(pitch, 50, 150) / 100.0f;
//
//	SoundBuffer *buffer = allocateBuffer(filename);
//
//	if (!buffer)
//		return;
//
//	/* Try to find first free source */
//	size_t i;
//	for (i = 0; i < srcCount; ++i)
//		if (AL::Source::getState(alSrcs[srcPrio[i]]) != AL_PLAYING)
//			break;
//
//	/* If we didn't find any, try to find the lowest priority source
//	 * with the same buffer to overtake */
//	if (i == srcCount)
//		for (size_t j = 0; j < srcCount; ++j)
//			if (atchBufs[srcPrio[j]] == buffer)
//				i = j;
//
//	/* If we didn't find any, overtake the one with lowest priority */
//	if (i == srcCount)
//		i = 0;
//
//	size_t srcIndex = srcPrio[i];
//
//	/* Only detach/reattach if it's actually a different buffer */
//	bool switchBuffer = (atchBufs[srcIndex] != buffer);
//
//	/* Push the used source to the back of the priority list */
//	arrayPushBack(srcPrio, srcCount, i);
//
//	AL::Source::ID src = alSrcs[srcIndex];
//	AL::Source::stop(src);
//
//	if (switchBuffer)
//		AL::Source::detachBuffer(src);
//
//	SoundBuffer *old = atchBufs[srcIndex];
//
//	if (old)
//		SoundBuffer::deref(old);
//
//	atchBufs[srcIndex] = SoundBuffer::ref(buffer);
//
//	if (switchBuffer)
//		AL::Source::attachBuffer(src, buffer->alBuffer);
//
//	AL::Source::setVolume(src, _volume * GLOBAL_VOLUME);
//	AL::Source::setPitch(src, _pitch);
//
//	AL::Source::play(src);
//}
//
//void SoundEmitter::stop()
//{
//	for (size_t i = 0; i < srcCount; i++)
//		AL::Source::stop(alSrcs[i]);
//}
//
//
//SoundBuffer *SoundEmitter::allocateBuffer(const std::string &filename)
//{
//	SoundBuffer *buffer = bufferHash.value(filename, 0);
//
//	if (buffer)
//	{
//		/* Buffer still in cashe.
//		 * Move to front of priority list */
//		buffers.remove(buffer->link);
//		buffers.append(buffer->link);
//
//		return buffer;
//	}
//	else
//	{
//		/* Buffer not in cache, needs to be loaded */
//		SoundOpenHandler handler;
//		shState->fileSystem().openRead(handler, filename.c_str());
//		buffer = handler.buffer;
//
//		if (!buffer)
//		{
//			char buf[512];
//			snprintf(buf, sizeof(buf), "Unable to decode sound: %s: %s",
//			         filename.c_str(), Sound_GetError());
//			Debug() << buf;
//
//			return 0;
//		}
//
//		buffer->key = filename;
//		uint32_t wouldBeBytes = bufferBytes + buffer->bytes;
//
//		/* If memory limit is reached, delete lowest priority buffer
//		 * until there is room or no buffers left */
//		while (wouldBeBytes > SE_CACHE_MEM && !buffers.isEmpty())
//		{
//			SoundBuffer *last = buffers.tail();
//			bufferHash.remove(last->key);
//			buffers.remove(last->link);
//
//			wouldBeBytes -= last->bytes;
//
//			SoundBuffer::deref(last);
//		}
//
//		bufferHash.insert(filename, buffer);
//		buffers.prepend(buffer->link);
//
//		bufferBytes = wouldBeBytes;
//
//		return buffer;
//	}
//}
