/*
** audio.cpp
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

#include "audio.h"

#include <cppcoro/sync_wait.hpp>

#include "audiostream.h"
#include "audio_thread.h"
#include "coro_utils.h"
#include "soundemitter.h"
#include "sharedstate.h"
#include "sharedmidistate.h"
#include "eventthread.h"
#include "sdl-util.h"

#include <string>

#include <SDL_thread.h>
#include <SDL_timer.h>

struct AudioPrivate
{
	AudioStream bgm;
	AudioStream bgs;
	AudioStream me;

	mkxp::sound_emitter se;

	SyncPoint &syncPoint;

	/* The 'MeWatch' is responsible for detecting
	 * a playing ME, quickly fading out the BGM and
	 * keeping it paused/stopped while the ME plays,
	 * and unpausing/fading the BGM back in again
	 * afterwards */
	enum MeWatchState
	{
		MeNotPlaying,
		BgmFadingOut,
		MePlaying,
		BgmFadingIn
	};

  cppcoro::cancellation_source m_me_watch_cancellation;

	AudioPrivate(RGSSThreadData &rtData)
	    : bgm(true, "bgm"),
	      bgs(true, "bgs"),
	      me(false, "me"),
	      se(rtData.config),
	      syncPoint(rtData.syncPoint)
	{
	}

  inline static constexpr float k_fade_out_step = 1.f/(200.f/AUDIO_SLEEP);
  inline static constexpr float k_fade_in_step = 1.f/(1000.f/AUDIO_SLEEP);

  auto play_me(cppcoro::cancellation_token token, const char *filename, int volume, int pitch) -> cppcoro::task<> {
    co_await mkxp::audio_thread::schedule();

    me.play(filename, volume, pitch);

    bgm.extPaused = true;

    /* bgm fading out */
    float vol = bgm.getVolume(AudioStream::External) - k_fade_out_step;
    while(vol > 0 && bgm.stream.query_state() != mkxp::al::state::e_Playing) {
      bgm.setVolume(AudioStream::External, vol);

      co_await mkxp::audio_thread::schedule_after(std::chrono::milliseconds{AUDIO_SLEEP});
      if(token.is_cancellation_requested())
        co_return;

      vol = bgm.getVolume(AudioStream::External) - k_fade_out_step;
    }

    bgm.setVolume(AudioStream::External, 0);
    bgm.stream.pause();
  }

  auto stop_me(cppcoro::cancellation_token token) -> cppcoro::task<> {
    co_await mkxp::audio_thread::schedule();

    me.stop();

    bgm.extPaused = false;

    if(bgm.stream.query_state() == mkxp::al::state::e_Paused) {
      /* fade in bgm */
      bgm.stream.play();

      float vol = bgm.getVolume(AudioStream::External) + k_fade_in_step;
      while(vol < 1) {
        if(bgm.stream.query_state() == mkxp::al::state::e_Stopped)
          break;

        bgm.setVolume(AudioStream::External, vol);

        co_await mkxp::audio_thread::schedule_after(std::chrono::milliseconds{AUDIO_SLEEP});
        if(token.is_cancellation_requested())
          co_return;

        vol = bgm.getVolume(AudioStream::External) + k_fade_in_step;
      }

      bgm.setVolume(AudioStream::External, 1.0f);
    } else if(bgm.stream.query_state() == mkxp::al::state::e_Stopped) {
      /* bgm is stopped */
      bgm.setVolume(AudioStream::External, 1.0f);

      if(!bgm.noResumeStop)
        bgm.stream.play();
    }
  }

  auto fade_me(cppcoro::cancellation_token token, int time) -> cppcoro::task<> {
    co_await mkxp::audio_thread::schedule();

    me.fadeOut(time);

    while(me.stream.query_state() != mkxp::al::state::e_Stopped) {
      co_await mkxp::audio_thread::schedule_after(std::chrono::milliseconds{AUDIO_SLEEP});
      if(token.is_cancellation_requested())
        co_return;
    }

    co_await stop_me(token);
  }
};

Audio::Audio(RGSSThreadData &rtData)
	: p(new AudioPrivate(rtData))
{}


void Audio::bgmPlay(const char *filename,
                    int volume,
                    int pitch,
                    float pos)
{
	p->bgm.play(filename, volume, pitch, pos);
}

void Audio::bgmStop()
{
	p->bgm.stop();
}

void Audio::bgmFade(int time)
{
	p->bgm.fadeOut(time);
}


void Audio::bgsPlay(const char *filename,
                    int volume,
                    int pitch,
                    float pos)
{
	p->bgs.play(filename, volume, pitch, pos);
}

void Audio::bgsStop()
{
	p->bgs.stop();
}

void Audio::bgsFade(int time)
{
	p->bgs.fadeOut(time);
}


void Audio::mePlay(const char *filename,
                   int volume,
                   int pitch)
{
  p->m_me_watch_cancellation.request_cancellation();
  p->m_me_watch_cancellation = {};
  mkxp::coro::make_oneshot(p->play_me(p->m_me_watch_cancellation.token(), filename, volume, pitch));

//  p->me.play(filename, volume, pitch);
}

void Audio::meStop()
{
  p->m_me_watch_cancellation.request_cancellation();
  p->m_me_watch_cancellation = {};
	mkxp::coro::make_oneshot(p->stop_me(p->m_me_watch_cancellation.token()));

//  p->me.stop();
}

void Audio::meFade(int time)
{
  p->m_me_watch_cancellation.request_cancellation();
  p->m_me_watch_cancellation = {};
  mkxp::coro::make_oneshot(p->fade_me(p->m_me_watch_cancellation.token(), time));

//  p->me.fadeOut(time);
}


void Audio::sePlay(const char *filename,
                   int volume,
                   int pitch)
{
	cppcoro::sync_wait(p->se.play(filename, volume, pitch));
}

void Audio::seStop()
{
	cppcoro::sync_wait(p->se.stop());
}

void Audio::setupMidi()
{
	shState->midiState().initIfNeeded(shState->config());
}

float Audio::bgmPos()
{
	return p->bgm.playingOffset();
}

float Audio::bgsPos()
{
	return p->bgs.playingOffset();
}

void Audio::reset()
{
	p->bgm.stop();
	p->bgs.stop();
	p->me.stop();
	cppcoro::sync_wait(p->se.stop());
}

Audio::~Audio() { delete p; }
