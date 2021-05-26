cmake_minimum_required(VERSION 3.15)
project(mkxp)


include(GNUInstallDirs)


option(SHARED_FLUID "Dynamically link fluidsynth at build time" OFF)
option(WORKDIR_CURRENT "Keep current directory on startup" OFF)
option(FORCE32 "Force 32bit compile on 64bit OS" OFF)
set(BINDING "MRI" CACHE STRING "The Binding Type (MRI, MRUBY, NULL)")
set(EXTERNAL_LIB_PATH "" CACHE PATH "External precompiled lib prefix")


find_package(PkgConfig REQUIRED)

if(EXTERNAL_LIB_PATH)
  set(CMAKE_PREFIX_PATH ${EXTERNAL_LIB_PATH})

  if(EXISTS "${EXTERNAL_LIB_PATH}/${LIB_PATH}/pkgconfig/")
    set(ENV{PKG_CONFIG_PATH} "${EXTERNAL_LIB_PATH}/${LIB_PATH}/pkgconfig/")
  endif()
endif()

find_package(Boost 1.49 COMPONENTS program_options REQUIRED)
find_package(fmt REQUIRED)

pkg_check_modules(openal REQUIRED IMPORTED_TARGET openal)
pkg_check_modules(physfs REQUIRED IMPORTED_TARGET physfs>=2.1)
pkg_check_modules(pixman REQUIRED IMPORTED_TARGET pixman-1)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
pkg_check_modules(SDL2_image REQUIRED IMPORTED_TARGET SDL2_image)
pkg_check_modules(SDL2_ttf REQUIRED IMPORTED_TARGET SDL2_ttf)
pkg_check_modules(sigcxx REQUIRED IMPORTED_TARGET sigc++-2.0)
pkg_check_modules(vorbisfile REQUIRED IMPORTED_TARGET vorbisfile)
pkg_check_modules(zlib REQUIRED IMPORTED_TARGET zlib)

if(SHARED_FLUID)
  pkg_check_modules(fluidsynth REQUIRED IMPORTED_TARGET fluidsynth)
endif()


set(EMBEDDED_INPUT
    shader/common.h
    shader/transSimple.frag
    shader/trans.frag
    shader/hue.frag
    shader/sprite.frag
    shader/plane.frag
    shader/gray.frag
    shader/bitmapBlit.frag
    shader/blend.frag
    shader/flatColor.frag
    shader/invert.frag
    shader/simple.frag
    shader/simpleColor.frag
    shader/simpleAlpha.frag
    shader/simpleAlphaUni.frag
    shader/flashMap.frag
    shader/minimal.vert
    shader/simple.vert
    shader/simpleColor.vert
    shader/sprite.vert
    shader/tilemap.vert
    shader/tilemapvx.vert
    shader/blur.frag
    shader/blurH.vert
    shader/blurV.vert
    shader/simpleMatrix.vert
    assets/liberation.ttf
    assets/icon.png)

find_program(XXD_EXE xxd
    DOC "Location of the xxd executable")

macro(processwithxxd outvar inputfile outdir)
  get_filename_component(basefile ${inputfile} NAME)
  set(outputfile ${outdir}/${basefile}.xxd)
  set_source_files_properties(${outputfile} PROPERTIES HEADER_ONLY TRUE)
  add_custom_command(OUTPUT ${outputfile}
      COMMAND ${XXD_EXE} -i ${inputfile} ${outputfile}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      COMMENT "Generating XXD for ${inputfile}")
  list(APPEND ${outvar}
      ${outputfile})
endmacro()

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/xxd)
foreach(item ${EMBEDDED_INPUT})
  processwithxxd(EMBEDDED_SRCS ${item} ${CMAKE_CURRENT_BINARY_DIR}/xxd)
endforeach()

source_group("Embedded Source" FILES ${EMBEDDED_INPUT} ${EMBEDDED_SRCS})


if(BINDING STREQUAL "MRI")
  set(MRIVERSION "1.9" CACHE STRING "Version of MRI to link with")
  if(MRIVERSION STREQUAL "0")
    pkg_check_modules(MRI REQUIRED IMPORTED_TARGET GLOBAL ruby)
    add_library(ruby::ruby ALIAS PkgConfig::MRI)
  elseif(MRIVERSION STREQUAL "1.8")
    add_subdirectory(lib/ruby-1.8)
    set(MRI_LIBRARIES ruby)
    add_library(ruby::ruby ALIAS ruby)
  elseif(MRIVERSION STREQUAL "1.9")
    add_subdirectory(lib/ruby-1.9)
    add_library(ruby::ruby ALIAS ruby)
  else()
    pkg_check_modules(MRI REQUIRED IMPORTED_TARGET ruby-${MRIVERSION})
    add_library(ruby::ruby ALIAS PkgConfig::MRI)
  endif()

  set(BINDING_HDRS
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mri/binding-util.h
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mri/binding-types.h
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mri/serializable-binding.h
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mri/disposable-binding.h
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mri/sceneelement-binding.h
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mri/viewportelement-binding.h
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mri/flashable-binding.h)

  set(BINDING_SRCS
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mri/binding-mri.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mri/binding-util.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mri/table-binding.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mri/etc-binding.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mri/bitmap-binding.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mri/font-binding.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mri/graphics-binding.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mri/input-binding.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mri/sprite-binding.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mri/viewport-binding.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mri/plane-binding.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mri/window-binding.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mri/tilemap-binding.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mri/audio-binding.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mri/module_rpg.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mri/filesystem-binding.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mri/windowvx-binding.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mri/tilemapvx-binding.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mri/tktk_bitmap-binding.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mri/win32-binding.cpp)

  add_library(ruby_binding INTERFACE)
    target_compile_definitions(ruby_binding
        INTERFACE
          BINDING_MRI)
    target_sources(ruby_binding
        INTERFACE
          ${BINDING_HDRS} ${BINDING_SRCS})
    target_link_libraries(ruby_binding
        INTERFACE
          fmt::fmt
          ruby::ruby
          unicode
          ${CMAKE_DL_LIBS})
elseif(BINDING STREQUAL "MRUBY")
  message(FATAL_ERROR "Mruby support in CMake needs to be finished")

  set(BINDING_HDRS
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mruby/binding-util.h
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mruby/disposable-binding.h
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mruby/flashable-binding.h
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mruby/binding-types.h
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mruby/sceneelement-binding.h
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mruby/viewportelement-binding.h
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mruby/serializable-binding.h
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mruby/mrb-ext/file.h
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mruby/mrb-ext/rwmem.h
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mruby/mrb-ext/marshal.h)

  set(BINDING_SRCS
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mruby/binding-mruby.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mruby/binding-util.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mruby/window-binding.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mruby/bitmap-binding.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mruby/sprite-binding.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mruby/font-binding.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mruby/viewport-binding.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mruby/plane-binding.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mruby/audio-binding.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mruby/tilemap-binding.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mruby/etc-binding.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mruby/graphics-binding.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mruby/input-binding.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mruby/table-binding.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mruby/module_rpg.c
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mruby/mrb-ext/file.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mruby/mrb-ext/marshal.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mruby/mrb-ext/rwmem.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mruby/mrb-ext/kernel.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/binding-mruby/mrb-ext/time.cpp)

  add_library(ruby_binding INTERFACE)
    target_compile_definitions(ruby_binding
        INTERFACE
          BINDING_MRUBY)
    target_sources(ruby_binding
        INTERFACE
          ${BINDING_HDRS} ${BINDING_SRCS})
elseif(BINDING STREQUAL "NULL")
  set(BINDING_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/binding-null/binding-null.cpp)

  add_library(ruby_binding INTERFACE)
    target_sources(ruby_binding
        INTERFACE
          ${BINDING_SRCS})
else()
  message(FATAL_ERROR "Must choose a valid binding type.  MRI, MRUBY, or NULL")
endif()

source_group("Binding Source" FILES ${BINDING_SOURCE} ${BINDING_HEADERS})


if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  if(CMAKE_SIZEOF_VOID_P MATCHES "8" AND NOT (FORCE32))
    set(MKXP_SUFFIX ".bin.x86_64")
  else()
    set(MKXP_SUFFIX ".bin.x86")
  endif()
endif()

set(MAIN_HDRS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/quadarray.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audio.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/binding.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bitmap.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/disposable.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/etc.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/etc-internal.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/eventthread.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flashable.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/font.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/input.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/iniconfig.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plane.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scene.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sprite.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/table.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/texpool.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tilequad.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/transform.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/viewport.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/window.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/serializable.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/glstate.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/quad.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tilemap.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tilemap-common.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gl-debug.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/global-ibo.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/exception.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/filesystem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/serial-util.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/intrulist.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/binding.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gl-util.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/util.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/config.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/settingsmenu.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/keybindings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tileatlas.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sharedstate.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/al-util.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/boost-hash.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/debugwriter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gl-fun.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gl-meta.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vertex.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/soundemitter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/aldatasource.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/alstream.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audiostream.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rgssad.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/windowvx.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tilemapvx.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tileatlasvx.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sharedmidistate.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/fluid-fun.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sdl-util.h
    $<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/windows/resource.h>)

set(MAIN_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audio.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bitmap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/eventthread.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/filesystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/font.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/input.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/iniconfig.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plane.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scene.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sprite.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/table.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tilequad.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/viewport.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/window.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/texpool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/glstate.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tilemap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/autotiles.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gl-debug.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/etc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/config.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/settingsmenu.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/keybindings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tileatlas.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sharedstate.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gl-fun.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gl-meta.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vertex.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/soundemitter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sdlsoundsource.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/alstream.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audiostream.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rgssad.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bundledfont.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vorbissource.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/windowvx.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tilemapvx.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tileatlasvx.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/autotilesvx.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/midisource.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/fluid-fun.cpp
    $<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/windows/resource.rc>)

source_group("MKXP Source" FILES ${MAIN_SOURCE} ${MAIN_HEADERS})

add_executable(mkxp ${EMBEDDED_SRCS} ${MAIN_HDRS} ${MAIN_SRCS})
  set_target_properties(mkxp PROPERTIES
      OUTPUT_NAME mkxp${MKXP_SUFFIX}
      CXX_VISIBILITY_PRESET hidden
      ENABLE_EXPORTS ON
      INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/mkxp)
  target_compile_features(mkxp
      PRIVATE
        cxx_std_17)
  target_compile_definitions(mkxp
      PRIVATE
        $<$<PLATFORM_ID:Linux>:_FILE_OFFSET_BITS=64>
        $<$<BOOL:${RGSS2}>:RGSS2>
        $<$<BOOL:${SHARED_FLUID}>:SHARED_FLUID>
        $<$<BOOL:${WORKDIR_CURRENT}>:WORKDIR_CURRENT>)
  target_include_directories(mkxp
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/windows
        ${CMAKE_CURRENT_BINARY_DIR}/xxd)
  target_link_libraries(mkxp
      PRIVATE
        Boost::boost
        Boost::program_options
        PkgConfig::openal
        PkgConfig::physfs
        PkgConfig::pixman
        PkgConfig::SDL2 PkgConfig::SDL2_image PkgConfig::SDL2_ttf
        PkgConfig::sigcxx
        PkgConfig::vorbisfile
        PkgConfig::zlib
        $<$<BOOL:${SHARED_FLUID}>:PkgConfig::fluidsynth>
        SDL_sound-static win32api
        mio::mio-headers
        ruby_binding)


add_subdirectory(lib/mio)
add_subdirectory(lib/SDL_Sound)
add_subdirectory(lib/win32api)


install(TARGETS mkxp win32api
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mkxp)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/lib/win32api/win32.rb
    DESTINATION ${CMAKE_INSTALL_DATADIR}/mkxp)