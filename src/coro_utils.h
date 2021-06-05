#ifndef MKXP_CORO_UTILS_H
#define MKXP_CORO_UTILS_H

#include <cassert>
#include <cstdint>

#include <coroutine>
#include <thread>

#include <atomic_queue/atomic_queue.h>

#include <cppcoro/schedule_on.hpp>
#include <cppcoro/task.hpp>

#include "timer.h"

namespace mkxp::coro {
  class oneshot {
   public:
    struct promise_type {
      auto get_return_object() noexcept -> oneshot {
        return oneshot{std::coroutine_handle<promise_type>::from_promise(*this)};
      }

      auto initial_suspend() const noexcept -> std::suspend_never {
        return {};
      }
      auto final_suspend() const noexcept -> std::suspend_never {
        return {};
      }

      void return_void() const noexcept {}

      void unhandled_exception() const noexcept {
        std::terminate();
      }
    };

   private:
    explicit oneshot(std::coroutine_handle<promise_type> handle) {}
  };

  template <typename SCHEDULER>
  inline void schedule_oneshot(SCHEDULER&& scheduler, cppcoro::task<>& task) noexcept {
    [](SCHEDULER&& scheduler, cppcoro::task<>& task) -> oneshot {
      co_await scheduler.schedule();
      co_await task;
    }(std::forward<SCHEDULER>(scheduler), task);
  }

  template <typename SCHEDULER>
  inline void schedule_oneshot(SCHEDULER&& scheduler, cppcoro::task<>&& task) noexcept {
    [](SCHEDULER&& scheduler, cppcoro::task<> task) -> oneshot {
      co_await scheduler.schedule();
      co_await task;
    }(std::forward<SCHEDULER>(scheduler), std::move(task));
  }

  class worker_thread {
    struct thread_state {
      explicit thread_state(uint32_t capacity) : queue{capacity} {}

      inline auto is_stopped() const noexcept -> bool {
        return stopped.load(std::memory_order_acquire);
      }

      inline void wake_up() noexcept {
        wakeup.test_and_set(std::memory_order_release);
        wakeup.notify_one();
      }

      std::atomic_bool stopped{false};
      std::atomic_flag wakeup{false};
      atomic_queue::AtomicQueueB<void*> queue;
    };

   public:
    class schedule_operation {
      friend class worker_thread;

     public:
      auto await_ready() noexcept -> bool {
        return false;
      }
      void await_suspend(std::coroutine_handle<> awaiting) noexcept {
        /* could deadlock? */
        mp_worker_state->queue.push(awaiting.address());

        mp_worker_state->wake_up();
      }
      void await_resume() noexcept {}

     private:
      schedule_operation(thread_state* ts) noexcept : mp_worker_state{ts} {}

      thread_state* mp_worker_state;
    };

    class delayed_schedule_operation {
      friend class worker_thread;

     public:
      auto await_ready() noexcept -> bool {
        return false;
      }
      void await_suspend(std::coroutine_handle<> awaiting) noexcept {
        if(m_interval.count() > 0) {
          set_oneshot_timer(m_interval, [w_state = std::move(mp_worker_state), awaiting]() {
            if(auto state = w_state.lock()) {
              state->queue.push(awaiting.address());

              state->wake_up();
            }
          });
        } else {
          if(auto state = mp_worker_state.lock()) {
            state->queue.push(awaiting.address());

            state->wake_up();
          }
        }
      }
      void await_resume() noexcept {}

     private:
      delayed_schedule_operation(std::weak_ptr<thread_state> ts, std::chrono::milliseconds interval) noexcept : mp_worker_state{std::move(ts)}, m_interval{interval} {}

      std::weak_ptr<thread_state> mp_worker_state;
      std::chrono::milliseconds m_interval;
    };


    /// Construct a worker thread with given fixed capacity for queueing.
    ///
    /// \param threadCount
    /// The maximum queue capacity.
    explicit worker_thread(uint32_t capacity);
    worker_thread(const worker_thread&) = delete;
    worker_thread(worker_thread&&) noexcept = default;
    ~worker_thread() noexcept {
      request_stop();
    }

    auto operator=(const worker_thread&) -> worker_thread& = delete;
    auto operator=(worker_thread&&) noexcept -> worker_thread& = default;

    [[nodiscard]] auto capacity() const noexcept {
      assert(mp_state);
      return mp_state->queue.capacity();
    }

    [[nodiscard]] auto is_stopped() const noexcept -> bool {
      return !mp_state || mp_state->is_stopped();
    }
    auto request_stop() noexcept -> bool {
      auto b = m_thread.request_stop();
      mp_state->wake_up();
      return b;
    }
    void stop() noexcept {
      if(m_thread.joinable()) {
        m_thread.request_stop();
        m_thread.join();
      }
      assert(is_stopped());
    }

    [[nodiscard]] auto schedule() noexcept -> schedule_operation {
      assert(!is_stopped());
      return schedule_operation{mp_state.get()};
    }
    template<typename Rep, typename Ratio>
    [[nodiscard]] auto schedule_after(std::chrono::duration<Rep, Ratio> delay) noexcept -> delayed_schedule_operation {
      assert(!is_stopped());
      return delayed_schedule_operation{mp_state, std::chrono::duration_cast<std::chrono::milliseconds>(delay)};
    }

   private:
    std::shared_ptr<thread_state> mp_state;
    std::jthread m_thread;
  };
}    // namespace mkxp::detail

#endif    //MKXP_CORO_UTILS_H
