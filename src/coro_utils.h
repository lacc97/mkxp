#ifndef MKXP_CORO_UTILS_H
#define MKXP_CORO_UTILS_H

#include <cassert>
#include <cstdint>

#include <coroutine>
#include <thread>

#include <atomic_queue/atomic_queue.h>

#include <cppcoro/schedule_on.hpp>
#include <cppcoro/task.hpp>

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

      std::atomic_bool stopped{false};
      std::atomic_flag wakeup{false};
      atomic_queue::AtomicQueueB<void*> queue;
    };

    class schedule_operation {
     public:
      schedule_operation(thread_state* ts) noexcept : mp_worker_state{ts} {}

      auto await_ready() noexcept -> bool {
        return false;
      }
      void await_suspend(std::coroutine_handle<> awaiting) noexcept {
        /* could deadlock? */
        mp_worker_state->queue.push(awaiting.address());

        mp_worker_state->wakeup.test_and_set(std::memory_order_release);
        mp_worker_state->wakeup.notify_one();
      }
      void await_resume() noexcept {}

     private:
      thread_state* mp_worker_state;
      std::coroutine_handle<> m_awaiting;
    };

   public:
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
      return !mp_state || mp_state->stopped.load(std::memory_order_acquire);
    }
    auto request_stop() noexcept -> bool {
      return m_thread.request_stop();
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

   private:
    std::unique_ptr<thread_state> mp_state;
    std::jthread m_thread;
  };
}    // namespace mkxp::detail

#endif    //MKXP_CORO_UTILS_H
