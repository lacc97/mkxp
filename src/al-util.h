/*
** al-util.h
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

#ifndef ALUTIL_H
#define ALUTIL_H

#include <al.h>
#include <SDL_audio.h>
#include <cassert>

#include <bit>
#include <utility>

namespace AL
{

#define DEF_AL_ID \
struct ID \
{ \
	ALuint al; \
	explicit ID(ALuint al = 0)  \
	    : al(al)  \
	{}  \
	ID &operator=(const ID &o)  \
	{  \
		al = o.al;  \
		return *this; \
	}  \
	bool operator==(const ID &o) const  \
	{  \
		return al == o.al;  \
	}  \
};

namespace Buffer
{
	DEF_AL_ID

	inline Buffer::ID gen()
	{
		Buffer::ID id;
		alGenBuffers(1, &id.al);

		return id;
	}

	inline void del(Buffer::ID id)
	{
		alDeleteBuffers(1, &id.al);
	}

	inline void uploadData(Buffer::ID id, ALenum format, const ALvoid *data, ALsizei size, ALsizei freq)
	{
		alBufferData(id.al, format, data, size, freq);
	}

	inline ALint getInteger(Buffer::ID id, ALenum prop)
	{
		ALint value;
		alGetBufferi(id.al, prop, &value);

		return value;
	}

	inline ALint getSize(Buffer::ID id)
	{
		return getInteger(id, AL_SIZE);
	}

	inline ALint getBits(Buffer::ID id)
	{
		return getInteger(id, AL_BITS);
	}

	inline ALint getChannels(Buffer::ID id)
	{
		return getInteger(id, AL_CHANNELS);
	}
}

namespace Source
{
	DEF_AL_ID

	inline Source::ID gen()
	{
		Source::ID id;
		alGenSources(1, &id.al);

		return id;
	}

	inline void del(Source::ID id)
	{
		alDeleteSources(1, &id.al);
	}

	inline void attachBuffer(Source::ID id, Buffer::ID buffer)
	{
		alSourcei(id.al, AL_BUFFER, buffer.al);
	}

	inline void detachBuffer(Source::ID id)
	{
		attachBuffer(id, Buffer::ID(0));
	}

	inline void queueBuffer(Source::ID id, Buffer::ID buffer)
	{
		alSourceQueueBuffers(id.al, 1, &buffer.al);
	}

	inline Buffer::ID unqueueBuffer(Source::ID id)
	{
		Buffer::ID buffer;
		alSourceUnqueueBuffers(id.al, 1, &buffer.al);

		return buffer;
	}

	inline void clearQueue(Source::ID id)
	{
		attachBuffer(id, Buffer::ID(0));
	}

	inline ALint getInteger(Source::ID id, ALenum prop)
	{
		ALint value;
		alGetSourcei(id.al, prop, &value);

		return value;
	}

	inline ALint getProcBufferCount(Source::ID id)
	{
		return getInteger(id, AL_BUFFERS_PROCESSED);
	}

	inline ALenum getState(Source::ID id)
	{
		return getInteger(id, AL_SOURCE_STATE);
	}

	inline ALfloat getSecOffset(Source::ID id)
	{
		ALfloat value;
		alGetSourcef(id.al, AL_SEC_OFFSET, &value);

		return value;
	}

	inline void setVolume(Source::ID id, float value)
	{
		alSourcef(id.al, AL_GAIN, value);
	}

	inline void setPitch(Source::ID id, float value)
	{
		alSourcef(id.al, AL_PITCH, value);
	}

	inline void play(Source::ID id)
	{
		alSourcePlay(id.al);
	}

	inline void stop(Source::ID id)
	{
		alSourceStop(id.al);
	}

	inline void pause(Source::ID id)
	{
		alSourcePause(id.al);
	}
}

}

namespace mkxp::al {
  class source;

  class buffer {
    friend class source;

   public:
    static inline auto create() noexcept -> buffer {
      ALuint id;
      alGenBuffers(1, &id);
      return buffer{id};
    }

    inline buffer() noexcept = default;

    inline void destroy() noexcept {
      if(m_id)
        alDeleteBuffers(1, &m_id);
      m_id = 0;
    }

    inline void upload_data(ALenum format, const ALvoid *data, ALsizei size, ALsizei freq)
    {
      alBufferData(m_id, format, data, size, freq);
    }

    inline auto get_integer(ALenum prop) const noexcept -> ALint {
      ALint value;
      alGetBufferi(m_id, prop, &value);
      return value;
    }


    inline auto get_bits() const noexcept -> ALint {
      return get_integer(AL_BITS);
    }

    inline auto get_channels() const noexcept -> ALint {
      return get_integer(AL_CHANNELS);
    }

    inline auto get_size() const noexcept -> ALint {
      return get_integer(AL_SIZE);
    }


    inline explicit operator bool() const noexcept {
      return m_id != 0;
    }

    inline operator AL::Buffer::ID() const noexcept {
      return AL::Buffer::ID(m_id);
    }

   private:
    inline explicit buffer(ALuint id) noexcept : m_id{id} {}

    ALuint m_id{};
  };

  class source {
   public:
    static inline auto create() noexcept -> source {
      ALuint id;
      alGenSources(1, &id);
      return source{id};
    }

    inline source() noexcept = default;

    inline void destroy() noexcept {
      if(m_id)
        alDeleteSources(1, &m_id);
      m_id = 0;
    }

    inline void play() noexcept {
      alSourcePlay(m_id);
    }
    inline void pause() noexcept {
      alSourcePause(m_id);
    }
    inline void stop() noexcept {
      alSourceStop(m_id);
    }

    inline void attach_buffer(const buffer& buf) noexcept {
      alSourcei(m_id, AL_BUFFER, std::bit_cast<ALint>(buf.m_id));
    }
    inline void detach_buffer() noexcept {
      alSourcei(m_id, AL_BUFFER, 0);
    }

    inline void queue_buffer(const buffer& buf) noexcept {
      alSourceQueueBuffers(m_id, 1, &buf.m_id);
    }
    inline buffer unqueue_buffer() noexcept {
      ALuint id;
      alSourceUnqueueBuffers(m_id, 1, &id);
      return buffer{id};
    }

    inline void clear_queue() noexcept {
      alSourcei(m_id, AL_BUFFER, 0);
    }

    inline auto get_integer(ALenum prop) const noexcept -> ALint {
      ALint value;
      alGetSourcei(m_id, prop, &value);
      return value;
    }

    inline auto get_sec_offset() const noexcept -> ALfloat {
      ALfloat value;
      alGetSourcef(m_id, AL_SEC_OFFSET, &value);
      return value;
    }
    inline auto get_state() const noexcept -> ALint {
      return get_integer(AL_SOURCE_STATE);
    }
    inline auto get_processed_buffer_count() const noexcept -> ALfloat {
      return get_integer(AL_BUFFERS_PROCESSED);
    }

    inline void set_pitch(ALfloat value) noexcept {
      alSourcef(m_id, AL_PITCH, value);
    }
    inline void set_volume(ALfloat value) noexcept {
      alSourcef(m_id, AL_GAIN, value);
    }


    inline auto operator==(al::source other) const noexcept -> bool {
      return m_id != 0 && m_id == other.m_id;
    }

    inline explicit operator bool() const noexcept {
      return m_id != 0;
    }

   private:
    inline explicit source(ALuint id) noexcept : m_id{id} {}

    ALuint m_id{};
  };
}    // namespace mkxp::al

inline uint8_t formatSampleSize(int sdlFormat)
{
	switch (sdlFormat)
	{
	case AUDIO_U8 :
	case AUDIO_S8 :
		return 1;

	case AUDIO_U16LSB :
	case AUDIO_U16MSB :
	case AUDIO_S16LSB :
	case AUDIO_S16MSB :
		return 2;

	default :
		assert(!"Unhandled sample format");
	}

	return 0;
}

inline ALenum chooseALFormat(int sampleSize, int channelCount)
{
	switch (sampleSize)
	{
	case 1 :
		switch (channelCount)
		{
		case 1 : return AL_FORMAT_MONO8;
		case 2 : return AL_FORMAT_STEREO8;
		}
	case 2 :
		switch (channelCount)
		{
		case 1 : return AL_FORMAT_MONO16;
		case 2 : return AL_FORMAT_STEREO16;
		}
	default :
		assert(!"Unhandled sample size / channel count");
	}

	return 0;
}

#define AUDIO_SLEEP 8
#define STREAM_BUF_SIZE 32768
#define GLOBAL_VOLUME 0.8f

#endif // ALUTIL_H
