/*
** alstream.cpp
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

#include "alstream.h"

#include <deque>
#include <thread>

#include <cppcoro/schedule_on.hpp>
#include <cppcoro/sync_wait.hpp>
#include <cppcoro/task.hpp>
#include <utility>

#include <fmt/format.h>

#include "audio_thread.h"
#include "aldatasource.h"
#include "coro_utils.h"
#include "debugwriter.h"
#include "eventthread.h"
#include "exception.h"
#include "filesystem.h"
#include "fluid-fun.h"
#include "sdl-util.h"
#include "sharedmidistate.h"
#include "sharedstate.h"

#include <SDL_mutex.h>
#include <SDL_thread.h>
#include <SDL_timer.h>

namespace {
  struct ALStreamOpenHandler : public FileSystem::OpenHandler {
    SDL_RWops* srcOps;
    bool looped;
    ALDataSource* source;
    std::string errorMsg;

    ALStreamOpenHandler(bool looped) : looped(looped), source(0), srcOps(static_cast<SDL_RWops*>(SDL_malloc(sizeof(SDL_RWops)))) {}
    ~ALStreamOpenHandler() {
      //      SDL_free(srcOps);
    }

    bool tryRead(SDL_RWops& ops, const char* ext) {
      /* Copy this because we need to keep it around,
       * as we will continue reading data from it later */
      *srcOps = ops;

      /* Try to read ogg file signature */
      char sig[5] = {0};
      SDL_RWread(srcOps, sig, 1, 4);
      SDL_RWseek(srcOps, 0, RW_SEEK_SET);

      try {
        if(!strcmp(sig, "OggS")) {
          source = createVorbisSource(*srcOps, looped);
          return true;
        }

        if(!strcmp(sig, "MThd")) {
          shState->midiState().initIfNeeded(shState->config());

          if(HAVE_FLUID) {
            source = createMidiSource(*srcOps, looped);
            return true;
          }
        }

        source = createSDLSource(*srcOps, ext, STREAM_BUF_SIZE, looped);
      } catch(const Exception& e) {
        /* All source constructors will close the passed ops
         * before throwing errors */
        errorMsg = e.msg;
        return false;
      }

      return true;
    }
  };
}    // namespace

mkxp::al::stream::stream(bool looped) : m_looped{looped}, m_source{source::create()}, m_buffers{{buffer::create(), buffer::create(), buffer::create()}} {
  if(!audio_thread::is_running())
    audio_thread::start(64);

  m_source.set_pitch(1.0f);
  m_source.set_volume(1.0f);
  m_source.detach_buffer();
}

mkxp::al::stream::~stream() {
  close();

  for(auto& buf : m_buffers)
    buf.destroy();

  m_source.destroy();
}

void mkxp::al::stream::close() {
  stop();
  mp_data_source.reset();
  m_source.clear_queue();
}

void mkxp::al::stream::open(const char* filename) {
  switch(query_state()) {
    case state::e_Playing:
    case state::e_Paused:
      cppcoro::sync_wait(stop_stream());
    case state::e_Stopped:
      close_source();
    case state::e_Closed:
      open_source(filename);
      break;
  }
}

void mkxp::al::stream::stop() {
  switch(query_state()) {
    case state::e_Closed:
    case state::e_Stopped:
      break;

    case state::e_Playing:
    case state::e_Paused:
      cppcoro::sync_wait(stop_stream());
      break;
  }
}

void mkxp::al::stream::play(float offset) {
  if(!m_source)
    return;

  switch(query_state()) {
    case state::e_Closed:
    case state::e_Playing:
      break;

    case state::e_Stopped:
      coro::make_oneshot(start_stream(offset));
      break;

    case state::e_Paused:
      cppcoro::sync_wait(resume_stream());
      break;
  }
}

void mkxp::al::stream::pause() {
  switch(query_state()) {
    case state::e_Closed:
    case state::e_Stopped:
    case state::e_Paused:
      break;

    case state::e_Playing:
      cppcoro::sync_wait(pause_stream());
      break;
  }
}

void mkxp::al::stream::open_source(const char* filename) {
  assert(!mp_data_source);

  ALStreamOpenHandler handler{m_looped};
  shState->fileSystem().openRead(handler, filename);
  if(!handler.source) {
    Debug() << fmt::format("Unable to decode audio stream: {}: {}", filename, handler.errorMsg);
    return;
  }

  mp_data_source.reset(handler.source);
}

auto mkxp::al::stream::start_stream(float offset) noexcept -> cppcoro::task<> {
  assert(m_source);
  assert(!mp_data);
  assert(mp_data_source);

  mp_data = std::make_shared<data>();
  mp_data->processed_frames = static_cast<uint64_t>(offset * static_cast<double>(mp_data_source->sampleRate()));

  m_source.clear_queue();
  mp_data_source->seekToOffset(offset);

  auto last_buffer = al::buffer{};
  for(auto& buf : m_buffers) {
    auto status = mp_data_source->fillBuffer(buf);
    if(status == ALDataSource::Error) {
      mp_data.reset();
      co_return;
    }

    m_source.queue_buffer(buf);

    if(status == ALDataSource::EndOfStream || status == ALDataSource::WrapAround) {
      if(status == ALDataSource::WrapAround)
        last_buffer = buf;
      break;
    }
  }

  m_source.play();
  mp_data->state = state::e_Paused;

  co_await loop_stream();
}

template <typename T>
auto unqueue_buffer(mkxp::al::source source, std::weak_ptr<T> w_sentinel) -> cppcoro::task<mkxp::al::buffer> {
  do {
    if(auto data = w_sentinel.lock()) {
      if(source.get_processed_buffer_count() > 0)
        co_return source.unqueue_buffer();
      else
        co_await mkxp::audio_thread::schedule_after(std::chrono::milliseconds{8});
    } else {
      /* suspend forever, but we should eventually get deleted from main thread */
      co_await std::suspend_always{};
    }
  } while(true);
}

auto wait_until_source_stops(mkxp::al::source source) -> cppcoro::task<> {
  do {
    if(source.get_state() == AL_STOPPED)
      co_return;
    else
      co_await mkxp::audio_thread::schedule_after(std::chrono::milliseconds{8});
  } while(true);
}

auto mkxp::al::stream::loop_stream() noexcept -> cppcoro::task<> {
  auto w_data = std::weak_ptr<data>(mp_data);

  co_await audio_thread::schedule();

  auto last_buffer = al::buffer{};
  while(true) {
    auto buf = co_await unqueue_buffer(m_source, w_data);

    /* If something went wrong, try again later */
    if(!buf)
      continue;

    if(last_buffer) {
      mp_data->processed_frames = mp_data_source->loopStartFrames();
      last_buffer = {};
    } else {
      auto bits = buf.get_bits();
      auto size = buf.get_size();
      auto chan = buf.get_channels();

      if(bits != 0 && chan != 0)
        mp_data->processed_frames += ((size / (bits / 8)) / chan);
    }

    //    if(data_src_status == ALDataSource::EndOfStream)
    //      continue;

    auto status = mp_data_source->fillBuffer(buf);
    if(status == ALDataSource::Error)
      break;

    m_source.queue_buffer(buf);

    /* In case of buffer underrun,
     * start playing again */
    if(m_source.get_state() == AL_STOPPED)
      m_source.play();

    /* If this was the last buffer before the data
     * source loop wrapped around again, mark it as
     * such so we can catch it and reset the processed
     * sample count once it gets unqueued */
    if(status == ALDataSource::WrapAround)
      last_buffer = buf;
    else if(status == ALDataSource::EndOfStream) {
      co_await wait_until_source_stops(m_source);
      break;
    }
  }

  co_await stop_stream();
}

auto mkxp::al::stream::resume_stream() noexcept -> cppcoro::task<> {
  co_await audio_thread::schedule();

  assert(m_source);
  assert(mp_data);

  m_source.play();

  mp_data->state = state::e_Playing;
}

auto mkxp::al::stream::pause_stream() noexcept -> cppcoro::task<> {
  co_await audio_thread::schedule();

  assert(m_source);
  assert(mp_data);

  m_source.pause();

  mp_data->state = state::e_Paused;
}

auto mkxp::al::stream::stop_stream() noexcept -> cppcoro::task<> {
  co_await audio_thread::schedule();

  assert(m_source);
  assert(mp_data);

  mp_data.reset();
  m_source.stop();
}

void mkxp::al::stream::close_source() {
  assert(!mp_data);
  assert(!mp_data_source);

  mp_data_source.reset();
}

auto mkxp::al::stream::get_offset() const -> float {
  if(!mp_data)
    return 0;

  return cppcoro::sync_wait([this]() -> cppcoro::task<float> {
    auto data = mp_data;
    if(!data)
      co_return 0;

    assert(mp_data_source);

    co_return(static_cast<float>(data->processed_frames) / mp_data_source->sampleRate()) + m_source.get_sec_offset();
  }());
}
void mkxp::al::stream::set_pitch(float value) {
  coro::schedule_oneshot(*audio_thread::get(), [](ALDataSource* data_src, al::source src, float value) -> cppcoro::task<> {
    if(data_src && data_src->setPitch(value))
      src.set_pitch(1.0f);
    else
      src.set_pitch(value);

    co_return;
  }(mp_data_source.get(), m_source, value));
}
void mkxp::al::stream::set_volume(float value) {
  coro::schedule_oneshot(*audio_thread::get(), [](al::source src, float value) -> cppcoro::task<> {
    src.set_volume(value);
    co_return;
  }(m_source, value));
}
