/*
** filesystem-binding.cpp
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

#include <sstream>

#include "binding-util.h"

#include "sharedstate.h"
#include "filesystem.h"
#include "util.h"

#ifndef RUBY_LEGACY_VERSION
#include "ruby/encoding.h"
#endif
#include "ruby/intern.h"

static std::string read_all(SDL_RWops* ops) noexcept
{
  SDL_RWseek(ops, 0, RW_SEEK_SET);

  std::string data;

  size_t read_count;
  do {
    std::array<char, BUFSIZ> buf;
    read_count = SDL_RWread(ops, buf.data(), 1, buf.size());
    data.insert(data.end(), buf.begin(), buf.begin() + read_count);
  } while(read_count == BUFSIZ);

  return data;
}

DEF_TYPE_CUSTOMFREE(FileInt, freeInstance<std::istringstream>);

static VALUE
fileIntForPath(const char *path, bool rubyExc)
{
  auto *iss = [path, rubyExc]() -> std::istringstream* {
    SDL_RWops *ops = SDL_AllocRW();

    try
    {
      shState->fileSystem().openReadRaw(*ops, path);
    }
    catch (const Exception &e)
    {
      SDL_FreeRW(ops);

      if (rubyExc)
        raiseRbExc(e);
      else
        throw e;
    }

    auto* iss = new std::istringstream{read_all(ops)};

    SDL_RWclose(ops);
    SDL_FreeRW(ops);

    return iss;
  }();

	auto obj = rb_obj_alloc(rb_const_get(rb_cObject, rb_intern("FileInt")));
	setPrivateData(obj, iss);

	return obj;
}

RB_METHOD(fileIntRead)
{
  auto length = [argc, argv]() -> long {
    int length = -1;
    rb_get_args(argc, argv, "i", &length RB_ARG_END);
    return length;
  }();

  auto* iss = getPrivateData<std::istringstream>(self);

  if(length == -1) {
    auto cur = iss->tellg();

    iss->seekg(0, std::ios::end);
    auto end = iss->tellg();

    length = end - cur;

    iss->seekg(cur);
  }

  if(length == 0)
    return Qnil;

  VALUE data = rb_str_new(nullptr, length);
  iss->read(RSTRING_PTR(data), length);

  return data;
}

RB_METHOD(fileIntClose)
{
	RB_UNUSED_PARAM;

	return Qnil;
}

RB_METHOD(fileIntGetByte)
{
	RB_UNUSED_PARAM;

  auto* iss = getPrivateData<std::istringstream>(self);

  const auto ch = iss->get();
  return (ch != std::char_traits<char>::eof() ? rb_fix_new(ch) : Qnil);
}

RB_METHOD(fileIntBinmode)
{
	RB_UNUSED_PARAM;

	return Qnil;
}

RB_METHOD(fileIntToStr) {
  RB_UNUSED_PARAM;

  auto *iss = getPrivateData<std::istringstream>(self);

  auto cur = iss->tellg();
  iss->seekg(0, std::ios::end);
  auto length = iss->tellg();

  iss->seekg(0);

  VALUE data = rb_str_new(nullptr, length);
  iss->read(RSTRING_PTR(data), length);
  iss->seekg(cur);

  return data;
}

VALUE
kernelLoadDataInt(const char *filename, bool rubyExc)
{
	rb_gc_start();

	VALUE port = fileIntForPath(filename, rubyExc);

	VALUE marsh = rb_const_get(rb_cObject, rb_intern("Marshal"));

	// FIXME need to catch exceptions here with begin rescue
	VALUE result = rb_funcall2(marsh, rb_intern("load"), 1, &port);

	rb_funcall2(port, rb_intern("close"), 0, NULL);

	return result;
}

RB_METHOD(kernelLoadData)
{
	RB_UNUSED_PARAM;

	const char *filename;
	rb_get_args(argc, argv, "z", &filename RB_ARG_END);

	return kernelLoadDataInt(filename, true);
}

RB_METHOD(kernelSaveData)
{
	RB_UNUSED_PARAM;

	VALUE obj;
	VALUE filename;

	rb_get_args(argc, argv, "oS", &obj, &filename RB_ARG_END);

	VALUE file = rb_file_open_str(filename, "wb");

	VALUE marsh = rb_const_get(rb_cObject, rb_intern("Marshal"));

	VALUE v[] = { obj, file };
	rb_funcall2(marsh, rb_intern("dump"), ARRAY_SIZE(v), v);

	rb_io_close(file);

	return Qnil;
}

static VALUE stringForceUTF8(VALUE arg)
{
	if (RB_TYPE_P(arg, RUBY_T_STRING) && ENCODING_IS_ASCII8BIT(arg))
		rb_enc_associate_index(arg, rb_utf8_encindex());

	return arg;
}

static VALUE customProc(VALUE arg, VALUE proc)
{
	VALUE obj = stringForceUTF8(arg);
	obj = rb_funcall2(proc, rb_intern("call"), 1, &obj);

	return obj;
}

RB_METHOD(_marshalLoad)
{
	RB_UNUSED_PARAM;

	VALUE port, proc = Qnil;

	rb_get_args(argc, argv, "o|o", &port, &proc RB_ARG_END);

	VALUE utf8Proc;
	// FIXME: Not implemented for Ruby 1.8
#ifndef RUBY_LEGACY_VERSION
	if (NIL_P(proc))
		utf8Proc = rb_proc_new(RUBY_METHOD_FUNC(stringForceUTF8), Qnil);
	else
		utf8Proc = rb_proc_new(RUBY_METHOD_FUNC(customProc), proc);
#endif

	VALUE marsh = rb_const_get(rb_cObject, rb_intern("Marshal"));

#ifndef RUBY_LEGACY_VERSION
	VALUE v[] = { port, utf8Proc };
#else
	VALUE v[] = { port, proc };
#endif
	return rb_funcall2(marsh, rb_intern("_mkxp_load_alias"), ARRAY_SIZE(v), v);
}

void
fileIntBindingInit()
{
	VALUE klass = rb_define_class("FileInt", rb_cIO);
	rb_define_alloc_func(klass, classAllocate<&FileIntType>);

	_rb_define_method(klass, "read", fileIntRead);
	_rb_define_method(klass, "getbyte", fileIntGetByte);
	// Used by Ruby 1.8 Marshaller
	_rb_define_method(klass, "getc", fileIntGetByte);
	_rb_define_method(klass, "binmode", fileIntBinmode);
	_rb_define_method(klass, "close", fileIntClose);

  _rb_define_method(klass, "to_str", fileIntToStr);

	_rb_define_module_function(rb_mKernel, "load_data", kernelLoadData);
	_rb_define_module_function(rb_mKernel, "save_data", kernelSaveData);

	/* We overload the built-in 'Marshal::load()' function to silently
	 * insert our utf8proc that ensures all read strings will be
	 * UTF-8 encoded */
	VALUE marsh = rb_const_get(rb_cObject, rb_intern("Marshal"));
	rb_define_alias(rb_singleton_class(marsh), "_mkxp_load_alias", "load");
	_rb_define_module_function(marsh, "load", _marshalLoad);
}
