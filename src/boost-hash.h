/*
** boost-hash.h
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

#ifndef BOOSTHASH_H
#define BOOSTHASH_H

#include <unordered_map>
#include <unordered_set>

#include <utility>

namespace std {
  template<typename T1, typename T2>
  struct hash<std::pair<T1, T2>> {
    std::size_t operator()(std::pair<T1, T2> const &p) const {
      auto fn_hash_combine = [](std::size_t& seed, const auto& key) {
        hash<typename std::remove_const<typename std::remove_reference<decltype(key)>::type>::type> hasher;
        seed ^= hasher(key) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      };

      std::size_t seed1(0);
      fn_hash_combine(seed1, p.first);
      fn_hash_combine(seed1, p.second);

      std::size_t seed2(0);
      fn_hash_combine(seed2, p.second);
      fn_hash_combine(seed2, p.first);

      return std::min(seed1, seed2);
    }
  };
}

/* Wrappers around the boost unordered template classes,
 * exposing an interface similar to Qt's QHash/QSet */

template<typename K, typename V>
class BoostHash
{
private:
	typedef std::unordered_map<K, V> BoostType;
	typedef std::pair<K, V> PairType;
	BoostType p;

public:
	typedef typename BoostType::const_iterator const_iterator;

	inline bool contains(const K &key) const
	{
		const_iterator iter = p.find(key);

		return (iter != p.cend());
	}

	inline void insert(const K &key, const V &value)
	{
		p.insert(PairType(key, value));
	}

	inline void remove(const K &key)
	{
		p.erase(key);
	}

	inline const V value(const K &key) const
	{
		const_iterator iter = p.find(key);

		if (iter == p.cend())
			return V();

		return iter->second;
	}

	inline const V value(const K &key, const V &defaultValue) const
	{
		const_iterator iter = p.find(key);

		if (iter == p.cend())
			return defaultValue;

		return iter->second;
	}

	inline V &operator[](const K &key)
	{
		return p[key];
	}

	inline const_iterator cbegin() const
	{
		return p.cbegin();
	}

	inline const_iterator cend() const
	{
		return p.cend();
	}
};

template<typename K>
class BoostSet
{
private:
	typedef std::unordered_set<K> BoostType;
	BoostType p;

public:
	typedef typename BoostType::const_iterator const_iterator;

	inline bool contains(const K &key)
	{
		const_iterator iter = p.find(key);

		return (iter != p.cend());
	}

	inline void insert(const K &key)
	{
		p.insert(key);
	}

	inline void remove(const K &key)
	{
		p.erase(key);
	}

	inline const_iterator cbegin() const
	{
		return p.cbegin();
	}

	inline const_iterator cend() const
	{
		return p.cend();
	}
};

#endif // BOOSTHASH_H
