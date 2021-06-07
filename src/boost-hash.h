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

#include <robin_hood.h>

#include <string_view>

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
  struct string_hash
  {
    using hash_type = robin_hood::hash<std::string_view>;
    using is_transparent = void;

    size_t operator()(const char* str) const        { return hash_type{}(std::string_view{str}); }
    size_t operator()(std::string_view str) const   { return hash_type{}(std::string_view{str}); }
    size_t operator()(std::string const& str) const { return hash_type{}(std::string_view{str}); }
  };

  typedef robin_hood::unordered_node_map<K, V, std::conditional_t<std::is_same_v<K, std::string>, string_hash, robin_hood::hash<K>>, std::equal_to<>> BoostType;
	typedef typename BoostType::value_type PairType;
	BoostType p;

public:
	typedef typename BoostType::const_iterator const_iterator;

 template <typename Key>
 inline bool contains(const Key& key) const {
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

  template <typename Key>
  inline V value(const Key& key) const {
		const_iterator iter = p.find(key);

		if (iter == p.cend())
			return V();

		return iter->second;
	}

  template <typename Key>
  inline V value(const Key& key, const V& defaultValue) const {
		const_iterator iter = p.find(key);

		if (iter == p.cend())
			return defaultValue;

		return iter->second;
	}

  template <typename Key>
  inline V& operator[](Key const& key) {
    if(auto it = p.find(key); it != p.end()) {
      return it->second;
    } else {
      if constexpr(std::is_same_v<Key, K>)
        return p.try_emplace(key).first->second;
      else
        return p.try_emplace(K{key}).first->second;
    }
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
  struct string_hash
  {
    using hash_type = robin_hood::hash<std::string_view>;
    using is_transparent = void;

    size_t operator()(const char* str) const        { return hash_type{}(std::string_view{str}); }
    size_t operator()(std::string_view str) const   { return hash_type{}(std::string_view{str}); }
    size_t operator()(std::string const& str) const { return hash_type{}(std::string_view{str}); }
  };

private:
	typedef robin_hood::unordered_node_set<K, std::conditional_t<std::is_same_v<K, std::string>, string_hash, robin_hood::hash<K>>, std::equal_to<>> BoostType;
	BoostType p;

public:
	typedef typename BoostType::const_iterator const_iterator;

  template <typename Key>
	inline bool contains(const Key& key)
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
