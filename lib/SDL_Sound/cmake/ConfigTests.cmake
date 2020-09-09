include(CheckIncludeFile)
include(CheckSymbolExists)

check_include_file("assert.h"               HAVE_ASSERT_H)
check_include_file("dlfcn.h"                HAVE_DLFCN_H)
check_include_file("inttypes.h"             HAVE_INTTYPES_H)
check_include_file("memory.h"               HAVE_MEMORY_H)
check_symbol_exists("memset" "memory.h"     HAVE_MEMSET)
check_symbol_exists("setbuf" "memory.h"     HAVE_SETBUF)
check_include_file("strings.h"              HAVE_STRINGS_H)
check_include_file("string.h"               HAVE_STRING_H)
check_symbol_exists("strrchr" "string.h"    HAVE_STRRCHR)
check_include_file("signal.h"               HAVE_SIGNAL_H)
check_include_file("stdint.h"               HAVE_STDINT_H)
check_include_file("stdlib.h"               HAVE_STDLIB_H)
check_include_file("sys/stat.h"             HAVE_SYS_STAT_H)
check_include_file("sys/types.h"            HAVE_SYS_TYPES_H)
check_include_file("unistd.h"               HAVE_UNISTD_H)

configure_file(CMake_config.h.in config.h @ONLY)
