#include "coro_utils.h"

mkxp::coro::worker_thread::worker_thread(uint32_t capacity) : mp_state{std::make_unique<thread_state>(capacity)} {
  m_thread = std::jthread{[](std::stop_token token, thread_state* state) {
    while(!token.stop_requested()) {
      if(state->queue.was_empty())
        state->sleep();

      while(!state->queue.was_empty()) {
        auto coroutine = std::coroutine_handle<>::from_address(state->queue.pop());
        coroutine.resume();
      }
    }

    state->stopped.store(true, std::memory_order_release);
  }, mp_state.get()};
}