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
    struct data {
      std::atomic<enum state> state{state::e_Closed};
      std::atomic_uint64_t processed_frames{0};
    };

    explicit stream(bool looped);
    ~stream();

    void close();
    void open(const char* filename);
    void stop();
    void play(float offset = 0.0f);
    void pause();

    auto query_state() const noexcept -> state {
      if(!mp_data_source)
        return state::e_Closed;
      if(!mp_data)
        return state::e_Stopped;

      return mp_data->state;
    }

    auto get_offset() const -> float;
    void set_pitch(float value);
    void set_volume(float value);

   private:
    void open_source(const char* filename);
    void close_source();

    auto start_stream(float offset) noexcept -> cppcoro::task<>;
    auto loop_stream() noexcept -> cppcoro::task<>;
    auto pause_stream() noexcept -> cppcoro::task<>;
    auto resume_stream() noexcept -> cppcoro::task<>;
    auto stop_stream() noexcept -> cppcoro::task<>;

    bool m_looped;

    al::source m_source;
    std::array<al::buffer, 3> m_buffers;

    std::unique_ptr<ALDataSource> mp_data_source;
    std::shared_ptr<data> mp_data;
  };
}

#endif // ALSTREAM_H
