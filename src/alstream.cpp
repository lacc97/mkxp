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
  if(!m_source || !mp_data_source)
    return;

  stop();
  close_source();
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

  m_state = state::e_Stopped;
}

auto mkxp::al::stream::start_stream(float offset) noexcept -> cppcoro::task<> {
  assert(m_source);
  assert(mp_data_source);
  assert(query_state() == state::e_Stopped);

  m_processed_frames = static_cast<uint64_t>(offset * static_cast<double>(mp_data_source->sampleRate()));

  m_source.clear_queue();
  mp_data_source->seekToOffset(offset);

  auto last_buffer = al::buffer{};
  for(auto& buf : m_buffers) {
    auto status = mp_data_source->fillBuffer(buf);
    if(status == ALDataSource::Error)
      co_return;

    m_source.queue_buffer(buf);

    if(status == ALDataSource::EndOfStream || status == ALDataSource::WrapAround) {
      if(status == ALDataSource::WrapAround)
        last_buffer = buf;
      break;
    }
  }

  m_source.play();

  m_state = state::e_Playing;

  co_await loop_stream(m_cancellation_source.token());
}

auto unqueue_buffer(const cppcoro::cancellation_token& token, mkxp::al::source source) -> cppcoro::task<mkxp::al::buffer> {
  do {
    if(!token.is_cancellation_requested()) {
      if(source.get_processed_buffer_count() > 0)
        co_return source.unqueue_buffer();
      else
        co_await mkxp::audio_thread::schedule_after(std::chrono::milliseconds{8});
    } else {
      co_return mkxp::al::buffer{};
    }
  } while(true);
}

auto wait_until_source_stops(mkxp::al::source source) -> cppcoro::task<> {
  do {
    if(source.get_state() == AL_STOPPED)
      co_return;
    else
      co_await mkxp::audio_thread::schedule_after(std::chrono::milliseconds{AUDIO_SLEEP});
  } while(true);
}

auto mkxp::al::stream::loop_stream(cppcoro::cancellation_token token) noexcept -> cppcoro::task<> {
  co_await audio_thread::schedule();

  auto last_buffer = al::buffer{};
  while(true) {
    // TODO pass secondary sync

    auto buf = co_await unqueue_buffer(token, m_source);
    if(!buf) {
      if(token.is_cancellation_requested())
        break;
      else
        continue;
    }

    if(last_buffer) {
      m_processed_frames = mp_data_source->loopStartFrames();
      last_buffer = {};
    } else {
      auto bits = buf.get_bits();
      auto size = buf.get_size();
      auto chan = buf.get_channels();

      if(bits != 0 && chan != 0)
        m_processed_frames += ((size / (bits / 8)) / chan);
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

  if(auto state = query_state(); state == state::e_Playing || state == state::e_Paused)
    co_await stop_stream();
}

auto mkxp::al::stream::resume_stream() noexcept -> cppcoro::task<> {
  co_await audio_thread::schedule();

  assert(m_source);
  assert(query_state() == state::e_Paused);

  m_source.play();

  m_state = state::e_Playing;
}

auto mkxp::al::stream::pause_stream() noexcept -> cppcoro::task<> {
  co_await audio_thread::schedule();

  assert(m_source);
  assert(query_state() == state::e_Playing);

  m_source.pause();

  m_state = state::e_Paused;
}

auto mkxp::al::stream::stop_stream() noexcept -> cppcoro::task<> {
  co_await audio_thread::schedule();

  assert(m_source);
  assert(query_state() == state::e_Playing || query_state() == state::e_Paused);

  m_cancellation_source.request_cancellation();
  m_source.stop();
  m_cancellation_source = {};

  m_state = state::e_Stopped;
}

void mkxp::al::stream::close_source() {
  assert(query_state() == state::e_Stopped);
  assert(mp_data_source);

  mp_data_source.reset();

  if(m_source)
    m_source.clear_queue();
}

auto mkxp::al::stream::get_offset() const -> float {
  if(!m_cancellation_source.can_be_cancelled())
    return 0;

  return cppcoro::sync_wait([this](cppcoro::cancellation_token token) -> cppcoro::task<float> {
    if(!token.is_cancellation_requested())
      co_return 0;

    assert(mp_data_source);

    co_return(static_cast<float>(m_processed_frames) / mp_data_source->sampleRate()) + m_source.get_sec_offset();
  }(m_cancellation_source.token()));
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
