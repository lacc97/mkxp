add_library(robin_hood INTERFACE)
  target_include_directories(robin_hood INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_sources(robin_hood INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include/robin_hood.h)