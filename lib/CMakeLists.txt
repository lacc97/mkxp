set(BUILD_SHARED_LIBS OFF)

add_library(atomic_queue INTERFACE)
  target_include_directories(atomic_queue
      INTERFACE
        atomic_queue/include)

target_link_libraries(mkxp
    PRIVATE
      atomic_queue
      cppcoro::cppcoro
      fontconfig++
      mio::mio-headers
      robin_hood
      SDL_sound-static
      win32api)

add_subdirectory(cppcoro)
add_subdirectory(fontconfig)
add_subdirectory(mio)
add_subdirectory(robin_hood)
add_subdirectory(SDL_Sound)
add_subdirectory(win32api)


target_compile_definitions(cppcoro PUBLIC CPPCORO_COMPILER_SUPPORTS_SYMMETRIC_TRANSFER=1)
add_library(cppcoro::cppcoro ALIAS cppcoro)