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

#include <fmt/format.h>

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

namespace mkxp {
  namespace {
    class audio_thread {
      struct audio_source {
        al::source source;
        cppcoro::task<>* p_task{nullptr};
      };

     public:
      static auto get() noexcept -> audio_thread& {
        static auto g_thread = audio_thread{};
        return g_thread;
      }

      static auto unqueue_buffer(al::source source) noexcept {
        struct awaitable {
          explicit awaitable(al::source src) noexcept : source{src} {}

          auto await_ready() const noexcept -> bool {
            return source.get_processed_buffer_count() > 0;
          }
          void await_suspend(std::coroutine_handle<> handle) const noexcept {}
          auto await_resume() noexcept -> al::buffer {
            return source.unqueue_buffer();
          }

         private:
          al::source source;
        };

        return awaitable{source};
      }

      [[nodiscard]] auto is_running() const noexcept -> bool {
        return bool(mp_thread) && !mp_thread->is_stopped();
      }

      void start(uint32_t capacity) {
        assert(!is_running());
        mp_thread = std::make_unique<coro::worker_thread>(capacity);

        coro::schedule_oneshot(*mp_thread, check_sources());
      }
      void stop() {
        mp_thread.reset();
        assert(!is_running());
      }


      auto schedule_source_task(al::source source, cppcoro::task<>& task) -> cppcoro::task<> {
        co_await mp_thread->schedule();

        assert(std::find_if(m_sources.begin(), m_sources.end(), [source](const audio_source& src) -> bool { return src.source == source; }) == m_sources.end());

        m_sources.push_back({source, std::addressof(task)});
      }

      auto deschedule_source_task(al::source source) -> cppcoro::task<> {
        co_await mp_thread->schedule();

        assert(std::find_if(m_sources.begin(), m_sources.end(), [source](const audio_source& src) -> bool { return src.source == source; }) != m_sources.end());

        std::erase_if(m_sources, [source](const audio_source& src) -> bool { return src.source == source; });
      }

      inline auto schedule() noexcept {
        return mp_thread->schedule();
      }

     private:
      explicit audio_thread() = default;
      audio_thread(audio_thread&&) noexcept = default;

      auto operator=(audio_thread&&) noexcept -> audio_thread& = default;

      auto check_sources() -> cppcoro::task<> {
        for(auto& src : m_sources) {
          if(src.source.get_processed_buffer_count() > 0 && !src.p_task->is_ready())
            coro::schedule_oneshot(*mp_thread, *src.p_task);
        }

        coro::schedule_oneshot(*mp_thread, check_sources());
        co_return;
      }


      std::unique_ptr<coro::worker_thread> mp_thread;
      std::vector<audio_source> m_sources;
    };
  }    // namespace
}    // namespace mkxp

auto mkxp::al::stream::play_audio() -> cppcoro::task<> {
  auto data_src_status = ALDataSource::Status{};
  auto last_buffer = buffer{};

  for(auto& buf : m_buffers) {
    data_src_status = mp_data_source->fillBuffer(buf);
    if(data_src_status == ALDataSource::Error)
      co_return;

    m_source.queue_buffer(buf);

    if(data_src_status == ALDataSource::EndOfStream || data_src_status == ALDataSource::WrapAround) {
      if(data_src_status == ALDataSource::WrapAround)
        last_buffer = buf;
      break;
    }
  }

  while(true) {
    auto buf = co_await audio_thread::unqueue_buffer(m_source);

    /* If something went wrong, try again later */
    if(!buf)
      continue;

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

    if(data_src_status == ALDataSource::EndOfStream)
      continue;

    data_src_status = mp_data_source->fillBuffer(buf);

    if(data_src_status == ALDataSource::Error)
      co_return;

    m_source.queue_buffer(buf);

    /* In case of buffer underrun,
     * start playing again */
    if(m_source.get_state() == AL_STOPPED)
      m_source.play();

    /* If this was the last buffer before the data
     * source loop wrapped around again, mark it as
     * such so we can catch it and reset the processed
     * sample count once it gets unqueued */
    if(data_src_status == ALDataSource::WrapAround)
      last_buffer = buf;
  }
}

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
  if(!audio_thread::get().is_running())
    audio_thread::get().start(64);

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
}

void mkxp::al::stream::open(std::string_view filename) {
  {
    ALStreamOpenHandler handler{m_looped};
    shState->fileSystem().openRead(handler, filename.data());
    if(!handler.source) {
      Debug() << fmt::format("Unable to decode audio stream: {}: {}", filename, handler.errorMsg);
      return;
    }

    cppcoro::sync_wait(cppcoro::schedule_on(audio_thread::get(), [this, src = handler.source]() -> cppcoro::task<> {
      mp_data_source.reset(src);
      co_return;
    }()));
  }

  if(!mp_runner) {
    mp_runner = std::make_unique<cppcoro::task<>>(play_audio());

    coro::schedule_oneshot(audio_thread::get(), *mp_runner);
  }

  m_state = state::e_Stopped;
}

void mkxp::al::stream::stop() {
  if(!mp_runner || m_state == state::e_Stopped)
    return;

  cppcoro::sync_wait(audio_thread::get().deschedule_source_task(m_source));

  /* wait one cycle */
  cppcoro::sync_wait(cppcoro::schedule_on(audio_thread::get(), [this]() -> cppcoro::task<> {
    m_source.stop();
    co_return;
  }()));

  m_state = state::e_Stopped;
}

void mkxp::al::stream::play(float offset) {
  if(!mp_runner)
    return;

  cppcoro::sync_wait([this]() -> cppcoro::task<> {
    co_await audio_thread::get().schedule_source_task(m_source, *mp_runner);
    m_source.play();
  }());

  m_state = state::e_Playing;
}

void mkxp::al::stream::pause() {
  if(!mp_runner)
    return;

  coro::schedule_oneshot(audio_thread::get(), [this]() -> cppcoro::task<> {
    m_source.pause();
    co_return;
  }());

  m_state = state::e_Paused;
}

auto mkxp::al::stream::get_offset() const -> float {
  if(!mp_data_source)
    return 0;

  return cppcoro::sync_wait(cppcoro::schedule_on(audio_thread::get(), [this]() -> cppcoro::task<float> {
    float proc_offset = static_cast<float>(m_processed_frames) / mp_data_source->sampleRate();
    co_return proc_offset + m_source.get_sec_offset();
  }()));
}
void mkxp::al::stream::set_pitch(float value) {
  coro::schedule_oneshot(audio_thread::get(), [](ALDataSource* data_src, al::source src, float value) -> cppcoro::task<> {
    if(data_src && data_src->setPitch(value))
      src.set_pitch(1.0f);
    else
      src.set_pitch(value);

    co_return;
  }(mp_data_source.get(), m_source, value));
}
void mkxp::al::stream::set_volume(float value) {
  coro::schedule_oneshot(audio_thread::get(), [](al::source src, float value) -> cppcoro::task<> {
    src.set_volume(value);
    co_return;
  }(m_source, value));
}

//ALStream::ALStream(LoopMode loopMode,
//		           const std::string &threadId)
//	: looped(loopMode == Looped),
//	  state(Closed),
//	  source(0),
//	  thread(0),
//	  preemptPause(false),
//      pitch(1.0f)
//{
//	alSrc = AL::Source::gen();
//
//	AL::Source::setVolume(alSrc, 1.0f);
//	AL::Source::setPitch(alSrc, 1.0f);
//	AL::Source::detachBuffer(alSrc);
//
//	for (int i = 0; i < STREAM_BUFS; ++i)
//		alBuf[i] = AL::Buffer::gen();
//
//	pauseMut = SDL_CreateMutex();
//
//	threadName = std::string("al_stream (") + threadId + ")";
//}
//
//ALStream::~ALStream()
//{
//	close();
//
//	AL::Source::clearQueue(alSrc);
//	AL::Source::del(alSrc);
//
//	for (int i = 0; i < STREAM_BUFS; ++i)
//		AL::Buffer::del(alBuf[i]);
//
//	SDL_DestroyMutex(pauseMut);
//}
//
//void ALStream::close()
//{
//	checkStopped();
//
//	switch (state)
//	{
//	case Playing:
//	case Paused:
//		stopStream();
//	case Stopped:
//		closeSource();
//		state = Closed;
//	case Closed:
//		return;
//	}
//}
//
//void ALStream::open(const std::string &filename)
//{
//	checkStopped();
//
//	switch (state)
//	{
//	case Playing:
//	case Paused:
//		stopStream();
//	case Stopped:
//		closeSource();
//	case Closed:
//		openSource(filename);
//	}
//
//	state = Stopped;
//}
//
//void ALStream::stop()
//{
//	checkStopped();
//
//	switch (state)
//	{
//	case Closed:
//	case Stopped:
//		return;
//	case Playing:
//	case Paused:
//		stopStream();
//	}
//
//	state = Stopped;
//}
//
//void ALStream::play(float offset)
//{
//	if (!source)
//		return;
//
//	checkStopped();
//
//	switch (state)
//	{
//	case Closed:
//	case Playing:
//		return;
//	case Stopped:
//		startStream(offset);
//		break;
//	case Paused :
//		resumeStream();
//	}
//
//	state = Playing;
//}
//
//void ALStream::pause()
//{
//	checkStopped();
//
//	switch (state)
//	{
//	case Closed:
//	case Stopped:
//	case Paused:
//		return;
//	case Playing:
//		pauseStream();
//	}
//
//	state = Paused;
//}
//
//void ALStream::setVolume(float value)
//{
//	AL::Source::setVolume(alSrc, value);
//}
//
//void ALStream::setPitch(float value)
//{
//	/* If the source supports setting pitch natively,
//	 * we don't have to do it via OpenAL */
//	if (source && source->setPitch(value))
//		AL::Source::setPitch(alSrc, 1.0f);
//	else
//		AL::Source::setPitch(alSrc, value);
//}
//
//ALStream::State ALStream::queryState()
//{
//	checkStopped();
//
//	return state;
//}
//
//float ALStream::queryOffset()
//{
//	if (state == Closed)
//		return 0;
//
//	float procOffset = static_cast<float>(procFrames) / source->sampleRate();
//
//	return procOffset + AL::Source::getSecOffset(alSrc);
//}
//
//void ALStream::closeSource()
//{
//	delete source;
//}
//
//void ALStream::openSource(const std::string &filename)
//{
//	ALStreamOpenHandler handler(srcOps, looped);
//	shState->fileSystem().openRead(handler, filename.c_str());
//	source = handler.source;
//	needsRewind.clear();
//
//	if (!source)
//	{
//		char buf[512];
//		snprintf(buf, sizeof(buf), "Unable to decode audio stream: %s: %s",
//		         filename.c_str(), handler.errorMsg.c_str());
//
//		Debug() << buf;
//	}
//}
//
//void ALStream::stopStream()
//{
//	threadTermReq.set();
//
//	if (thread)
//	{
//		SDL_WaitThread(thread, 0);
//		thread = 0;
//		needsRewind.set();
//	}
//
//	/* Need to stop the source _after_ the thread has terminated,
//	 * because it might have accidentally started it again before
//	 * seeing the term request */
//	AL::Source::stop(alSrc);
//
//	procFrames = 0;
//}
//
//void ALStream::startStream(float offset)
//{
//	AL::Source::clearQueue(alSrc);
//
//	preemptPause = false;
//	streamInited.clear();
//	sourceExhausted.clear();
//	threadTermReq.clear();
//
//	startOffset = offset;
//	procFrames = offset * source->sampleRate();
//
//	thread = createSDLThread
//		<ALStream, &ALStream::streamData>(this, threadName);
//}
//
//void ALStream::pauseStream()
//{
//	SDL_LockMutex(pauseMut);
//
//	if (AL::Source::getState(alSrc) != AL_PLAYING)
//		preemptPause = true;
//	else
//		AL::Source::pause(alSrc);
//
//	SDL_UnlockMutex(pauseMut);
//}
//
//void ALStream::resumeStream()
//{
//	SDL_LockMutex(pauseMut);
//
//	if (preemptPause)
//		preemptPause = false;
//	else
//		AL::Source::play(alSrc);
//
//	SDL_UnlockMutex(pauseMut);
//}
//
//void ALStream::checkStopped()
//{
//	/* This only concerns the scenario where
//	 * state is still 'Playing', but the stream
//	 * has already ended on its own (EOF, Error) */
//	if (state != Playing)
//		return;
//
//	/* If streaming thread hasn't queued up
//	 * buffers yet there's not point in querying
//	 * the AL source */
//	if (!streamInited)
//		return;
//
//	/* If alSrc isn't playing, but we haven't
//	 * exhausted the data source yet, we're just
//	 * having a buffer underrun */
//	if (!sourceExhausted)
//		return;
//
//	if (AL::Source::getState(alSrc) == AL_PLAYING)
//		return;
//
//	stopStream();
//	state = Stopped;
//}
//
///* thread func */
//void ALStream::streamData()
//{
//	/* Fill up queue */
//	bool firstBuffer = true;
//	ALDataSource::Status status;
//
//	if (threadTermReq)
//		return;
//
//	if (needsRewind)
//	{
//		source->seekToOffset(startOffset);
//	}
//
//	for (int i = 0; i < STREAM_BUFS; ++i)
//	{
//		if (threadTermReq)
//			return;
//
//		AL::Buffer::ID buf = alBuf[i];
//
//		status = source->fillBuffer(buf);
//
//		if (status == ALDataSource::Error)
//			return;
//
//		AL::Source::queueBuffer(alSrc, buf);
//
//		if (firstBuffer)
//		{
//			resumeStream();
//
//			firstBuffer = false;
//			streamInited.set();
//		}
//
//		if (threadTermReq)
//			return;
//
//		if (status == ALDataSource::EndOfStream)
//		{
//			sourceExhausted.set();
//			break;
//		}
//	}
//
//	/* Wait for buffers to be consumed, then
//	 * refill and queue them up again */
//	while (true)
//	{
//		shState->rtData().syncPoint.passSecondarySync();
//
//		ALint procBufs = AL::Source::getProcBufferCount(alSrc);
//
//		while (procBufs--)
//		{
//			if (threadTermReq)
//				break;
//
//			AL::Buffer::ID buf = AL::Source::unqueueBuffer(alSrc);
//
//			/* If something went wrong, try again later */
//			if (buf == AL::Buffer::ID(0))
//				break;
//
//			if (buf == lastBuf)
//			{
//				/* Reset the processed sample count so
//				 * querying the playback offset returns 0.0 again */
//				procFrames = source->loopStartFrames();
//				lastBuf = AL::Buffer::ID(0);
//			}
//			else
//			{
//				/* Add the frame count contained in this
//				 * buffer to the total count */
//				ALint bits = AL::Buffer::getBits(buf);
//				ALint size = AL::Buffer::getSize(buf);
//				ALint chan = AL::Buffer::getChannels(buf);
//
//				if (bits != 0 && chan != 0)
//					procFrames += ((size / (bits / 8)) / chan);
//			}
//
//			if (sourceExhausted)
//				continue;
//
//			status = source->fillBuffer(buf);
//
//			if (status == ALDataSource::Error)
//			{
//				sourceExhausted.set();
//				return;
//			}
//
//			AL::Source::queueBuffer(alSrc, buf);
//
//			/* In case of buffer underrun,
//			 * start playing again */
//			if (AL::Source::getState(alSrc) == AL_STOPPED)
//				AL::Source::play(alSrc);
//
//			/* If this was the last buffer before the data
//			 * source loop wrapped around again, mark it as
//			 * such so we can catch it and reset the processed
//			 * sample count once it gets unqueued */
//			if (status == ALDataSource::WrapAround)
//				lastBuf = buf;
//
//			if (status == ALDataSource::EndOfStream)
//				sourceExhausted.set();
//		}
//
//		if (threadTermReq)
//			break;
//
//		SDL_Delay(AUDIO_SLEEP);
//	}
//}
