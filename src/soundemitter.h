/*
** soundemitter.h
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

#ifndef SOUNDEMITTER_H
#define SOUNDEMITTER_H

#include <list>
#include <memory>
#include <string>

#include <cppcoro/task.hpp>

#include <robin_hood.h>

#include "al-util.h"
#include "lru_cache.h"


struct Config;

namespace mkxp {
  class sound_emitter {
    struct buffer {
      ~buffer() noexcept {
        al_buffer.destroy();
      }

      al::buffer al_buffer;
      const size_t size;
    };

    struct source {
      inline auto is_available() const noexcept -> bool {
        return p_buffer && al_source.get_state() != AL_PLAYING;
      }

      al::source al_source;
      std::shared_ptr<buffer> p_buffer;
    };

    struct buffer_cache_traits {
      using value_type = std::shared_ptr<buffer>;

      inline static auto element_size(const std::shared_ptr<buffer>& buf) noexcept -> size_t {
        return buf ? buf->size : 0;
      }
    };

   public:
    explicit sound_emitter(const Config& conf) noexcept;
    ~sound_emitter();

    auto play(const char* filename, int volume, int pitch) noexcept -> cppcoro::task<>;

    auto stop() noexcept -> cppcoro::task<> {
      for(auto& src : m_sources)
        src.al_source.stop();

      co_return;
    }

   private:
    auto acquire_source(const char* filename) noexcept -> cppcoro::task<source>;

    std::list<source> m_sources;
//    std::list<buffer> m_buffers;

    utils::lru_cache<std::string, std::shared_ptr<buffer>, buffer_cache_traits> m_buffer_cache;

//    const size_t m_cache_capacity;
//    size_t m_cache_size{};
//    robin_hood::unordered_flat_map<std::string, std::list<buffer>::iterator> m_cache;
  };
}

//struct SoundBuffer;
//struct Config;
//
//struct SoundEmitter
//{
//	typedef BoostHash<std::string, SoundBuffer*> BufferHash;
//
//	IntruList<SoundBuffer> buffers;
//	BufferHash bufferHash;
//
//	/* Byte count sum of all cached / playing buffers */
//	uint32_t bufferBytes;
//
//	const size_t srcCount;
//	std::vector<AL::Source::ID> alSrcs;
//	std::vector<SoundBuffer*> atchBufs;
//
//	/* Indices of sources, sorted by priority (lowest first) */
//	std::vector<size_t> srcPrio;
//
//	SoundEmitter(const Config &conf);
//	~SoundEmitter();
//
//	void play(const std::string &filename,
//	          int volume,
//	          int pitch);
//
//	void stop();
//
//private:
//	SoundBuffer *allocateBuffer(const std::string &filename);
//};

#endif // SOUNDEMITTER_H
