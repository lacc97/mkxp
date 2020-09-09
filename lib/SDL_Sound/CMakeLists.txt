cmake_minimum_required(VERSION 3.15)
project(SDL_sound VERSION 1.0.1)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(ENABLE_VOC       "Enable VOC decoding"               ON)
option(ENABLE_WAV       "Enable WAV decoding"               ON)
option(ENABLE_RAW       "Enable raw audio 'decoding'"       ON)
option(ENABLE_AIFF      "Enable AIFF decoding"              ON)
option(ENABLE_AU        "Enable AU decoding"                ON)
option(ENABLE_SHN       "Enable SHN decoding"               ON)
option(ENABLE_MIDI      "Enable software MIDI music"        ON)
option(ENABLE_MPG123    "Enable MP3 decoding"               ON)
# option(ENABLE_MIKMOD    "Enable MOD decoding via mikmod"    OFF)
option(ENABLE_MODPLUG   "Enable MOD decoding via modplug"   ON)
option(ENABLE_OGG       "Enable OGG decoding via libvorbis" ON)
option(ENABLE_FLAC      "Enable FLAC decoding via libFLAC"  ON)
option(ENABLE_SPEEX     "Enable SPX decoding via libspeex"  ON)
# option(ENABLE_PHYSICSFS "Enable PhysicsFS in playsound"     OFF)

set(SOUND_SUPPORTS_VOC          ${ENABLE_VOC})
set(SOUND_SUPPORTS_WAV          ${ENABLE_WAV})
set(SOUND_SUPPORTS_RAW          ${ENABLE_RAW})
set(SOUND_SUPPORTS_AIFF         ${ENABLE_AIFF})
set(SOUND_SUPPORTS_AU           ${ENABLE_AU})
set(SOUND_SUPPORTS_SHN          ${ENABLE_SHN})
set(SOUND_SUPPORTS_MIDI         ${ENABLE_MIDI})
set(SOUND_SUPPORTS_MPG123       ${ENABLE_MPG123})
# set(SOUND_SUPPORTS_MIKMOD     ${ENABLE_MIKMOD})
set(SOUND_SUPPORTS_MODPLUG      ${ENABLE_MODPLUG})
set(SOUND_SUPPORTS_OGG          ${ENABLE_OGG})
set(SOUND_SUPPORTS_FLAC         ${ENABLE_FLAC})
set(SOUND_SUPPORTS_SPEEX        ${ENABLE_SPEEX})
# set(SOUND_SUPPORTS_PHYSICSFS ${ENABLE_PHYSICSFS})

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)

if(ENABLE_MPG123)
    pkg_check_modules(MPG123 REQUIRED IMPORTED_TARGET libmpg123)
    set(SDL_SOUND_DEC_LIBRARIES ${SDL_SOUND_DEC_LIBRARIES} PkgConfig::MPG123)
endif(ENABLE_MPG123)
if(ENABLE_MIKMOD)
    pkg_check_modules(MIKMOD REQUIRED IMPORTED_TARGET mikmod)
    set(SDL_SOUND_DEC_LIBRARIES ${SDL_SOUND_DEC_LIBRARIES} PkgConfig::MIKMOD)
endif(ENABLE_MIKMOD)
if(ENABLE_MODPLUG)
    pkg_check_modules(MODPLUG REQUIRED IMPORTED_TARGET libmodplug)
    set(SDL_SOUND_DEC_LIBRARIES ${SDL_SOUND_DEC_LIBRARIES} PkgConfig::MODPLUG)
endif(ENABLE_MODPLUG)
if(ENABLE_OGG)
    pkg_check_modules(OGG REQUIRED IMPORTED_TARGET ogg vorbis)
    set(SDL_SOUND_DEC_LIBRARIES ${SDL_SOUND_DEC_LIBRARIES} PkgConfig::OGG)
endif(ENABLE_OGG)
if(ENABLE_FLAC)
    pkg_check_modules(FLAC REQUIRED IMPORTED_TARGET flac)
    set(SDL_SOUND_DEC_LIBRARIES ${SDL_SOUND_DEC_LIBRARIES} PkgConfig::FLAC)
endif(ENABLE_FLAC)
if(ENABLE_SPEEX)
    pkg_check_modules(SPEEX REQUIRED IMPORTED_TARGET speex)
    set(SDL_SOUND_DEC_LIBRARIES ${SDL_SOUND_DEC_LIBRARIES} PkgConfig::SPEEX)
endif(ENABLE_SPEEX)

include(ConfigTests)

set(SDL_SOUND_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/audio_convert.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/extra_rwops.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/SDL_sound.c")
        
add_library(SDL_sound SHARED ${SDL_SOUND_SOURCES})
    set_target_properties(SDL_sound PROPERTIES
        C_VISIBILITY_PRESET     hidden
        UNITY_BUILD             OFF)
    target_compile_definitions(SDL_sound
        PRIVATE
            HAVE_CONFIG_H
            SOUND_MODPLUG_IN_OWN_PATH)
    target_include_directories(SDL_sound
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE
            ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(SDL_sound
        PUBLIC
            PkgConfig::SDL2
        PRIVATE
            ${SDL_SOUND_DEC_LIBRARIES})

add_library(SDL_sound-static STATIC ${SDL_SOUND_SOURCES})
    set_target_properties(SDL_sound-static PROPERTIES
        OUTPUT_NAME SDL_sound
        C_VISIBILITY_PRESET     hidden
        UNITY_BUILD             OFF)
    target_compile_definitions(SDL_sound-static
        PRIVATE
            HAVE_CONFIG_H
            SOUND_MODPLUG_IN_OWN_PATH)
    target_include_directories(SDL_sound-static
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE
            ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(SDL_sound-static
        PUBLIC
            PkgConfig::SDL2
        PRIVATE
            ${SDL_SOUND_DEC_LIBRARIES})

add_subdirectory(decoders/)
# add_subdirectory(mixer/)
# add_subdirectory(playsound/)
