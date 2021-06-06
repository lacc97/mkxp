#ifndef MKXP_AUDIO_THREAD_H
#define MKXP_AUDIO_THREAD_H

#include <chrono>
#include <memory>

#include "coro_utils.h"

namespace mkxp::audio_thread {

  auto get() noexcept -> coro::worker_thread*;

  inline auto is_running() noexcept -> bool {
    return bool(get());
  }

  void start(uint32_t capacity);
  void stop();

  inline auto schedule() noexcept {
    assert(is_running());
    return get()->schedule();
  }

  template <typename Rep, typename Ratio>
  inline auto schedule_after(std::chrono::duration<Rep, Ratio> delay) noexcept {
    return get()->schedule_after<Rep, Ratio>(delay);
  }
}

#endif    //MKXP_AUDIO_THREAD_H
