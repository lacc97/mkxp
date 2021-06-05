#ifndef MKXP_TIMER_H
#define MKXP_TIMER_H

#include <any>
#include <chrono>

#include <SDL_timer.h>

namespace mkxp {
  namespace detail {
    struct timer_info {
      static auto create(SDL_TimerID id, std::any param) noexcept -> timer_info*;
      void destroy() noexcept;

      SDL_TimerID id;
      std::any param;

     private:
      timer_info(SDL_TimerID id, std::any param) noexcept : id{id}, param{std::move(param)} {}
      ~timer_info() = default;
    };

    auto set_timer(std::chrono::milliseconds interval, std::any params, SDL_TimerCallback cb) noexcept -> bool;
  }

  template <typename F> requires std::is_invocable_r_v<std::chrono::milliseconds, F, std::chrono::milliseconds>
  auto set_timer(std::chrono::milliseconds interval, F&& f) noexcept -> bool {
    using func_type = std::remove_const_t<std::remove_reference_t<F>>;

    return detail::set_timer(interval, std::make_any<func_type>(std::forward<F>(f)), [](uint32_t interval, void* param) noexcept -> uint32_t {
      auto* ti = static_cast<detail::timer_info*>(param);
      auto new_interval = std::any_cast<func_type>(ti->param)(std::chrono::milliseconds(interval)).count();
      if(new_interval == 0)
        ti->destroy();
      return new_interval;
    });
  }

  template <typename F> requires std::is_invocable_r_v<void, F>
  auto set_oneshot_timer(std::chrono::milliseconds delay, F&& f) noexcept -> bool {
    using func_type = std::remove_const_t<std::remove_reference_t<F>>;

    return detail::set_timer(delay, std::make_any<func_type>(std::forward<F>(f)), [](uint32_t interval, void* param) noexcept -> uint32_t {
      auto* ti = static_cast<detail::timer_info*>(param);
      std::any_cast<func_type>(ti->param)();
      ti->destroy();
      return 0;
    });
  }
}

#endif    //MKXP_TIMER_H
