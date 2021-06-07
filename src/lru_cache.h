#ifndef MKXP_LRU_CACHE_H
#define MKXP_LRU_CACHE_H

#include <list>

#include <robin_hood.h>

namespace mkxp::utils {
  /* NOT thread-safe */
  template <typename V>
  requires std::is_same_v<V, std::remove_const_t<std::remove_reference_t<V>>>
  struct cache_traits {
    using value_type = V;

    inline static auto element_size(const value_type& v) noexcept -> size_t {
      return sizeof(V);
    }
  };

  template <typename K, typename V, typename CacheTraits = cache_traits<V>>
  requires std::is_same_v<K, std::remove_const_t<std::remove_reference_t<K>>> && std::is_same_v<V, std::remove_const_t<std::remove_reference_t<V>>> &&
    std::is_same_v<V, typename CacheTraits::value_type>
  class lru_cache {
   public:
    using key_type = K;
    using value_type = V;
    using traits_type = CacheTraits;
    using size_type = size_t; // decltype(traits_type::element_size(std::declval<value_type>()));
    using tracker_type = std::list<K>;
    struct storage_type {
      V value;
      size_type size;
      typename tracker_type::const_iterator iterator;
    };
    using map_type = robin_hood::unordered_flat_map<K, storage_type>;


    explicit lru_cache(size_type capacity) noexcept : m_capacity{capacity} {}

    template <typename T> requires std::is_same_v<T, value_type> && std::is_copy_constructible_v<value_type>
    auto put(key_type&& k, const T& v) -> value_type* {
      assert(m_map.find(k) == m_map.end());

      /* Make space if new object exceeds capacity */
      auto v_size = traits_type::element_size(v) + k_element_overhead;
      while(m_size > 0 && v_size + m_size > m_capacity)
        evict_one();

      auto key_it = m_keys.insert(m_keys.end(), std::move(k));
      auto [map_it, success] = m_map.insert_or_assign(*key_it, storage_type{v, v_size, key_it});
      assert(success);

      m_size += v_size;
      return std::addressof(map_it->second.value);
    }

    auto put(key_type&& k, value_type&& v) -> value_type* {
      assert(m_map.find(k) == m_map.end());

      /* Make space if new object exceeds capacity */
      auto v_size = traits_type::element_size(v) + k_element_overhead;
      while(m_size > 0 && v_size + m_size > m_capacity)
        evict_one();

      auto key_it = m_keys.insert(m_keys.end(), std::move(k));
      auto [map_it, success] = m_map.insert_or_assign(*key_it, storage_type{std::move(v), v_size, key_it});
      assert(success);

      m_size += v_size;
      return std::addressof(map_it->second.value);
    }

    /* You MUST copy the returned value (if any) if you want to store it because the pointers and references for the map_type are NOT stable. */
    auto get(const key_type& k) -> value_type* {
      auto map_it = m_map.find(k);
      if(map_it == m_map.end())
        return nullptr;

      m_keys.splice(m_keys.end(), m_keys, map_it->second.iterator);

      return std::addressof(map_it->second.value);
    }

   private:
    void evict_one() noexcept {
      assert(!m_keys.empty());

      auto it = m_map.find(m_keys.front());
      assert(it != m_map.end());
      auto v_size = it->second.size;

      m_map.erase(it);
      m_keys.pop_front();
      m_size -= v_size;
    }

    inline static size_type k_element_overhead = sizeof(storage_type::size) + sizeof(storage_type::iterator);


    const size_type m_capacity;

    size_type m_size{0};
    tracker_type m_keys;
    map_type m_map;
  };

}    // namespace mkxp::utils

#endif    //MKXP_LRU_CACHE_H
