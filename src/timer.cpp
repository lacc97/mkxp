#include "timer.h"

auto mkxp::detail::timer_info::create(std::any param) noexcept -> timer_info* {
  return new timer_info{std::move(param)};
}
void mkxp::detail::timer_info::destroy() noexcept {
  delete this;
}

#if defined(__linux__)

#  include <poll.h>
#  include <sys/eventfd.h>
#  include <sys/timerfd.h>

#  include <cassert>

#  include <semaphore>
#  include <thread>
#  include <vector>

#  include <robin_hood.h>

namespace {
  class inline_mutex {
   public:
    inline_mutex() noexcept = default;
    inline_mutex(const inline_mutex&) = delete;
    inline_mutex(inline_mutex&&) = delete;

    auto operator=(const inline_mutex&) -> inline_mutex& = delete;
    auto operator=(inline_mutex&&) -> inline_mutex& = delete;

    inline void lock() noexcept {
      bool b;
      while(!m_locked.compare_exchange_weak(b = false, true, std::memory_order_acq_rel, std::memory_order_relaxed))
        m_locked.wait(true, std::memory_order_relaxed);
    }
    inline void unlock() noexcept {
      m_locked.store(false, std::memory_order_release);
      m_locked.notify_one();
    }

   private:
    std::atomic_bool m_locked;
  };

  class event_fd {
   public:
    static inline auto create(uint32_t count, bool semaphore = false) noexcept -> event_fd {
      return {::eventfd(count, EFD_CLOEXEC | EFD_NONBLOCK | (semaphore ? EFD_SEMAPHORE : 0))};
    }


    inline event_fd() noexcept : m_fd{-1} {}
    inline event_fd(int fd) noexcept : m_fd{fd} {}

    inline void close() noexcept {
      ::close(m_fd);
    }

    inline auto read() noexcept -> uint64_t {
      auto buf = uint64_t{};
      if(::read(m_fd, std::addressof(buf), sizeof(buf)) < 0)
        return 0;
      else
        return buf;
    }

    inline void write(uint64_t val) noexcept {
      ::write(m_fd, std::addressof(val), sizeof(val));
    }

    inline auto operator==(event_fd other) const noexcept -> bool {
      return m_fd == other.m_fd;
    }

    inline explicit operator int() const noexcept {
      return m_fd;
    }

   private:
    int m_fd;
  };

  class timer_fd {
   public:
    enum class clock_id { e_Realtime = CLOCK_REALTIME, e_Monotonic = CLOCK_MONOTONIC };

    static inline auto create(clock_id clock) noexcept -> timer_fd {
      return {::timerfd_create(static_cast<int>(clock), TFD_NONBLOCK | TFD_CLOEXEC)};
    }


    inline timer_fd() noexcept : m_fd{-1} {}
    inline timer_fd(int fd) noexcept : m_fd{fd} {}

    inline void close() noexcept {
      ::close(m_fd);
    }

    inline auto read() noexcept -> uint64_t {
      auto buf = uint64_t{};
      if(::read(m_fd, std::addressof(buf), sizeof(buf)) < 0)
        return 0;
      else
        return buf;
    }

    inline auto set_time(itimerspec spec) noexcept -> itimerspec {
      auto old_spec = itimerspec{};
      ::timerfd_settime(m_fd, 0, std::addressof(spec), std::addressof(old_spec));
      return old_spec;
    }

    inline auto operator==(timer_fd other) const noexcept -> bool {
      return m_fd == other.m_fd;
    }

    inline explicit operator bool() const noexcept {
      return m_fd != -1;
    }

    inline explicit operator int() const noexcept {
      return m_fd;
    }

   private:
    int m_fd;
  };
}    // namespace

namespace std {
  template <>
  struct hash<timer_fd> : hash<int> {
    inline auto operator()(timer_fd t) const noexcept {
      return hash<int>::operator()(static_cast<int>(t));
    }
  };
}    // namespace std

namespace {
  class timer_scheduler {
   public:
    struct timer_data {
      uint32_t interval;
      SDL_TimerCallback callback;
      void* params;
    };


    timer_scheduler() : m_wakeup{event_fd::create(0)}, m_finished{false} {
      m_thread = std::jthread{[this]() {
        const auto fn_poll = [](pollfd* fds, nfds_t nfds, int timeout) -> int {
          auto ret = int{};
          while((ret = ::poll(fds, nfds, timeout)) <= 0) {
            if(errno != EINTR)
              break;
          }
          return ret;
        };

        auto pollfds = std::vector<pollfd>();
        const auto fn_fill_pollfds = [this, &pollfds]() {
          auto guard = std::lock_guard{m_mutex};

          pollfds.resize(m_timers.size() + 1);

          size_t ii = 0;
          pollfds[ii++] = {static_cast<int>(m_wakeup), POLLIN, 0};
          for(auto& elem : m_timers)
            pollfds[ii++] = {static_cast<int>(elem.first), POLLIN, 0};
          ;
        };

        fn_fill_pollfds();
        while(true) {
          fn_poll(pollfds.data(), pollfds.size(), -1);
          if(m_finished.load(std::memory_order_acquire))
            break;

          {
            auto beg = pollfds.begin(), end = pollfds.end();
            for(auto cur = beg + 1; cur != end; ++cur) {
              if(cur->revents & POLLIN) {
                auto guard = std::lock_guard{m_mutex};

                auto it = m_timers.find(cur->fd);
                assert(it != m_timers.end());

                it->second.interval = it->second.callback(it->second.interval, it->second.params);
                if(it->second.interval > 0) {
                  it->first.set_time({{}, from_interval(it->second.interval)});
                } else {
                  it->first.close();
                  m_timers.erase(it);
                }
              }
            }
          }

          m_wakeup.read();

          fn_fill_pollfds();
        }
      }};
    }
    ~timer_scheduler() {
      m_finished.store(true, std::memory_order_release);
      m_wakeup.write(1);
    }


    inline auto schedule(uint32_t interval, SDL_TimerCallback callback, void* params) -> bool {
      auto timer = timer_fd::create(timer_fd::clock_id::e_Monotonic);
      if(!timer)
        return false;

      timer.set_time({{}, from_interval(interval)});

      {
        auto guard = std::lock_guard{m_mutex};
        assert(m_timers.find(timer) == m_timers.end());
        m_timers.insert(robin_hood::pair<timer_fd, timer_data>{timer, timer_data{interval, callback, params}});
      }

      m_wakeup.write(1);

      return true;
    }

   private:
    inline static auto from_interval(uint32_t interval) noexcept -> timespec {
      timespec spec;
      spec.tv_sec = interval / 1000;
      spec.tv_nsec = (interval - 1000 * spec.tv_sec) * 1000;
      return spec;
    }


    event_fd m_wakeup;
    std::atomic_bool m_finished;

    mutable std::mutex m_mutex;
    robin_hood::unordered_flat_map<timer_fd, timer_data> m_timers;

    std::jthread m_thread;
  };
}    // namespace

auto mkxp::detail::set_timer(std::chrono::milliseconds interval, std::any params, SDL_TimerCallback cb) noexcept -> bool {
  static auto s_scheduler = timer_scheduler{};

  auto* ti = timer_info::create(std::move(params));
  if(!s_scheduler.schedule(interval.count(), cb, ti)) {
    ti->destroy();
    return false;
  } else {
    return true;
  }
}

#else

auto mkxp::detail::set_timer(std::chrono::milliseconds interval, std::any params, SDL_TimerCallback cb) noexcept -> bool {
  auto* ti = timer_info::create(std::move(params));
  auto id = SDL_AddTimer(interval.count(), cb, ti);
  if(id == 0) {
    ti->destroy();
    return false;
  } else {
    return true;
  }
}

#endif