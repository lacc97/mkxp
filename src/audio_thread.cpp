#include "audio_thread.h"

namespace {
  auto g_audio_thread = std::unique_ptr<mkxp::coro::worker_thread>{};
}

auto mkxp::audio_thread::get() noexcept -> mkxp::coro::worker_thread* {
  return g_audio_thread.get();
}

void mkxp::audio_thread::start(uint32_t capacity) {
  assert(!g_audio_thread);
  g_audio_thread = std::make_unique<coro::worker_thread>(capacity);
}

void mkxp::audio_thread::stop() {
  assert(g_audio_thread);
  g_audio_thread.reset();
}
