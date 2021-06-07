#ifndef MKXP_CORO_UTILS_H
#define MKXP_CORO_UTILS_H

#include <cassert>
#include <cstdint>

#include <atomic>
#include <coroutine>
#include <functional>
#include <thread>

#include <atomic_queue/atomic_queue.h>

#include <cppcoro/schedule_on.hpp>
#include <cppcoro/task.hpp>

#include <fmt/format.h>

#include "timer.h"

namespace mkxp::coro {
  inline void make_oneshot(cppcoro::task<>&& task) noexcept {
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

    [](cppcoro::task<> task) -> oneshot {
      co_await task;
    }(std::move(task));
  }

  template <typename SCHEDULER>
  inline void schedule_oneshot(SCHEDULER&& scheduler, cppcoro::task<>& task) noexcept {
    make_oneshot([](SCHEDULER&& scheduler, cppcoro::task<>& task) -> cppcoro::task<> {
      co_await scheduler.schedule();
      co_await task;
    }(std::forward<SCHEDULER>(scheduler), task));
  }

  template <typename SCHEDULER>
  inline void schedule_oneshot(SCHEDULER&& scheduler, cppcoro::task<>&& task) noexcept {
    make_oneshot([](SCHEDULER&& scheduler, cppcoro::task<> task) -> cppcoro::task<> {
      co_await scheduler.schedule();
      co_await task;
    }(std::forward<SCHEDULER>(scheduler), std::move(task)));
  }

  /* A class to schedule up a task on a scheduler and then forgetting about it (mostly). Can be cancelled but it is not really thread safe, as it is
   * literally just an atomic bool. Auto cancelled on destruction. Call detach() if you don't want this.
   *
   * cancel() is fire-and-forget. wait_cancel() actually waits until the promise gets destroyed (can cause deadlocks).*/
  template <typename Scheduler>
  class async_task {
    struct state {
      std::atomic_flag cancelled{false};
      std::atomic_flag finished{false};
    };

   public:
    class promise_type {
      friend class async_task;

     public:
      template <typename... Args>
      explicit promise_type(Scheduler&& sched, const Args&... args) noexcept : mp_scheduler{std::addressof(sched)} {}
      ~promise_type() noexcept {
        if(mp_state)
          mp_state->finished.test_and_set(true, std::memory_order_release);
      }

      auto get_return_object() noexcept -> async_task {
        return {mp_state = std::make_shared<state>(true)};
      }

      auto initial_suspend() noexcept {
        return mp_scheduler->schedule();
      }

      auto final_suspend() noexcept -> std::suspend_never {
        return {};
      }

      void return_void() const noexcept {}

      void unhandled_exception() const noexcept {
        std::terminate();
      }

     private:
      Scheduler* mp_scheduler;
      std::shared_ptr<state> mp_state;
    };

    static auto is_cancelled() noexcept {
      struct awaiter {
        auto await_ready() const noexcept -> bool {
          return false;
        }
        auto await_suspend(std::coroutine_handle<promise_type> handle) const noexcept -> bool {
          m_handle = handle;
          return false;
        }
        auto await_resume() const noexcept -> bool {
          return m_handle.promise().mp_state->cancelled.test(std::memory_order_acquire);
        }

       private:
        std::coroutine_handle<promise_type> m_handle;
      };

      return awaiter{};
    }

    async_task() noexcept = default;
    async_task(const async_task&) = delete;
    async_task(async_task&& other) noexcept = default;
    ~async_task() noexcept {
      if(mp_state)
        cancel();
    }

    auto operator=(const async_task&) -> async_task& = delete;
    auto operator=(async_task&& other) noexcept -> async_task& {
      if(std::addressof(other) != this) {
        if(mp_state)
          cancel();

        mp_state = std::exchange(other.mp_state, nullptr);
      }
      return *this;
    }


    void cancel() noexcept {
      assert(mp_state);
      mp_state->cancelled.test_and_set(std::memory_order_release);
    }

    void detach() noexcept {
      mp_state.reset();
    }

    [[nodiscard]] auto is_finished() const noexcept -> bool {
      assert(mp_state);
      return mp_state->finished.test(std::memory_order_acquire);
    }

    void wait_cancel() noexcept {
      cancel();
      mp_state->finished.wait(false, std::memory_order_acquire);
    }

    operator bool() const noexcept {
      return bool(mp_state);
    }

   private:
    explicit async_task(std::shared_ptr<state> state) noexcept : mp_state{std::move(state)} {}


    std::shared_ptr<state> mp_state;
  };

  class worker_thread {
    struct thread_state {
      explicit thread_state(uint32_t capacity) : queue{capacity} {}

      [[nodiscard]] inline auto id() const noexcept -> std::thread::id {
        return thread_id.load(std::memory_order_acquire);
      }
      inline void set_id() noexcept {
        return thread_id.store(std::this_thread::get_id(), std::memory_order_acquire);
      }

      [[nodiscard]] inline auto is_stopped() const noexcept -> bool {
        return stopped.test(std::memory_order_acquire);
      }
      void mark_stopped() noexcept {
        stopped.test_and_set(std::memory_order_release);
      }

      inline void sleep() noexcept {
        wakeup.wait(false, std::memory_order_acquire);
        wakeup.clear(std::memory_order_relaxed);
      }
      inline void wake_up() noexcept {
        wakeup.test_and_set(std::memory_order_release);
        wakeup.notify_one();
      }

      atomic_queue::AtomicQueueB<void*> queue;

     private:
      std::atomic_flag stopped{false};
      std::atomic_flag wakeup{false};
      std::atomic<std::thread::id> thread_id;
    };

   public:
    class schedule_operation {
      friend class worker_thread;

     public:
      auto await_ready() noexcept -> bool {
        return std::this_thread::get_id() == mp_worker_state->id();
      }
      void await_suspend(std::coroutine_handle<> awaiting) noexcept {
        auto& state = *mp_worker_state;

        /* could deadlock? */
        state.queue.push(awaiting.address());
        state.wake_up();
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
        if(m_interval.count() > 0)
          return false;
        else
          return m_no_delay_op.await_ready();
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
          m_no_delay_op.await_suspend(awaiting);
        }
      }
      void await_resume() noexcept {}

     private:
      delayed_schedule_operation(const std::shared_ptr<thread_state>& ts, std::chrono::milliseconds interval) noexcept : m_no_delay_op{ts.get()}, mp_worker_state{ts}, m_interval{interval} {}

      schedule_operation m_no_delay_op;
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
      assert(mp_state);
      assert(!is_stopped());
      return schedule_operation{mp_state.get()};
    }
    template<typename Rep, typename Ratio>
    [[nodiscard]] auto schedule_after(std::chrono::duration<Rep, Ratio> delay) noexcept -> delayed_schedule_operation {
      assert(mp_state);
      assert(!is_stopped());
      return delayed_schedule_operation{mp_state, std::chrono::duration_cast<std::chrono::milliseconds>(delay)};
    }

   private:
    std::shared_ptr<thread_state> mp_state;
    std::jthread m_thread;
  };
}    // namespace mkxp::detail

#endif    //MKXP_CORO_UTILS_H
