#include "timer.h"

auto mkxp::detail::timer_info::create(std::any param) noexcept -> timer_info* {
  return new timer_info{std::move(param)};
}
void mkxp::detail::timer_info::destroy() noexcept {
  delete this;
}

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
