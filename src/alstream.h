/*
** alstream.h
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

#ifndef ALSTREAM_H
#define ALSTREAM_H

#include <array>
#include <memory>

#include <cppcoro/task.hpp>

#include "al-util.h"
#include "sdl-util.h"

#include <string>
#include <SDL_rwops.h>

struct ALDataSource;

namespace mkxp::al {
  enum class state {
    e_Playing,
    e_Paused,
    e_Stopped,
    e_Closed
  };

  class stream {
   public:
    static auto create() -> stream;

    stream(bool looped);
    ~stream();

    void close();
    void open(std::string_view filename);
    void stop();
    void play(float offset = 0.0f);
    void pause();

    auto query_state() const noexcept -> state {
      return m_state;
    }

    auto get_offset() const -> float;
    void set_pitch(float value);
    void set_volume(float value);

   private:
    auto play_audio() -> cppcoro::task<>;

    bool m_looped;

    al::source m_source;
    std::array<al::buffer, 3> m_buffers;
    std::unique_ptr<ALDataSource> mp_data_source;

    std::unique_ptr<cppcoro::task<>> mp_runner;

    uint64_t m_processed_frames{};
    state m_state{state::e_Closed};
  };
}

//
//struct ALDataSource;
//
//#define STREAM_BUFS 3
//
///* State-machine like audio playback stream.
// * This class is NOT thread safe */
//struct ALStream
//{
//	enum State
//	{
//		Closed,
//		Stopped,
//		Playing,
//		Paused
//	};
//
//	bool looped;
//	State state;
//
//	ALDataSource *source;
//	SDL_Thread *thread;
//
//	std::string threadName;
//
//	SDL_mutex *pauseMut;
//	bool preemptPause;
//
//	/* When this flag isn't set and alSrc is
//	 * in 'STOPPED' state, stream isn't over
//	 * (it just hasn't started yet) */
//	AtomicFlag streamInited;
//	AtomicFlag sourceExhausted;
//
//	AtomicFlag threadTermReq;
//
//	AtomicFlag needsRewind;
//	float startOffset;
//
//	float pitch;
//
//	AL::Source::ID alSrc;
//	AL::Buffer::ID alBuf[STREAM_BUFS];
//
//	uint64_t procFrames;
//	AL::Buffer::ID lastBuf;
//
//	SDL_RWops srcOps;
//
//	struct
//	{
//		ALenum format;
//		ALsizei freq;
//	} stream;
//
//	enum LoopMode
//	{
//		Looped,
//		NotLooped
//	};
//
//	ALStream(LoopMode loopMode,
//	         const std::string &threadId);
//	~ALStream();
//
//	void close();
//	void open(const std::string &filename);
//	void stop();
//	void play(float offset = 0);
//	void pause();
//
//	void setVolume(float value);
//	void setPitch(float value);
//	State queryState();
//	float queryOffset();
//	bool queryNativePitch();
//
//private:
//	void closeSource();
//	void openSource(const std::string &filename);
//
//	void stopStream();
//	void startStream(float offset);
//	void pauseStream();
//	void resumeStream();
//
//	void checkStopped();
//
//	/* thread func */
//	void streamData();
//};

#endif // ALSTREAM_H
