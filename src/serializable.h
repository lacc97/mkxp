/*
** serializable.h
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

#ifndef SERIALIZABLE_H
#define SERIALIZABLE_H

#include <cassert>
#include <cstring>

#include <span>

namespace mkxp {
  namespace details {
    template <typename T>
    concept serializable_primitive = std::is_trivial_v<T> && std::is_standard_layout_v<T>;
  };

  class serializer {
   public:
    explicit serializer(std::span<char> buffer) noexcept : mp_beg{buffer.data()}, mp_cur{mp_beg}, mp_end{mp_beg + buffer.size()} {}

    [[nodiscard]] inline auto available_bytes() const noexcept -> size_t {
      return mp_end - mp_cur;
    }

    template <details::serializable_primitive T>
    inline void write_one(const T& t) noexcept {
      assert(available_bytes() >= sizeof(T));

      std::memcpy(mp_cur, std::addressof(t), sizeof(T));
      mp_cur += sizeof(T);
    }

    template <details::serializable_primitive T>
    inline void write_many(std::span<const T> ts) noexcept {
      auto cpy_size = ts.size() * sizeof(T);
      assert(available_bytes() >= cpy_size);

      std::memcpy(mp_cur, ts.data(), cpy_size);
      mp_cur += cpy_size;
    }

   private:
    std::span<char>::pointer mp_beg;
    std::span<char>::pointer mp_cur;
    std::span<char>::pointer mp_end;
  };

  class deserializer {
   public:
    explicit deserializer(std::span<const char> buffer) noexcept : mp_beg{buffer.data()}, mp_cur{mp_beg}, mp_end{mp_beg + buffer.size()} {}

    [[nodiscard]] inline auto available_bytes() const noexcept -> size_t {
      return mp_end - mp_cur;
    }

    template <details::serializable_primitive T>
    inline auto read_one() noexcept -> T {
      assert(available_bytes() >= sizeof(T));

      T t;
      std::memcpy(std::addressof(t), mp_cur, sizeof(T));
      mp_cur += sizeof(T);
      return t;
    }

    template <details::serializable_primitive T>
    inline void read_many(std::span<T> ts) noexcept {
      auto cpy_size = ts.size() * sizeof(T);
      assert(available_bytes() >= cpy_size);

      std::memcpy(ts.data(), mp_cur, cpy_size);
      mp_cur += cpy_size;
    }

   private:
    std::span<const char>::const_pointer mp_beg;
    std::span<const char>::const_pointer mp_cur;
    std::span<const char>::const_pointer mp_end;
  };
}

struct Serializable
{
	virtual int serialSize() const = 0;
	virtual void serialize(mkxp::serializer ss) const = 0;
};


#endif // SERIALIZABLE_H
