# {{{ CMake stuff
cmake_minimum_required(VERSION 3.8)
cmake_policy(VERSION 3.8)
# }}}

#{{{ Basic App setup
set(PROJECT_NAME tagutil)
add_definitions(-DT_TAGUTIL_VERSION="3.1")
project(${PROJECT_NAME} C)

# CFLAGS
add_compile_options(-D_GNU_SOURCE -D_DEFAULT_SOURCE -D_BSD_SOURCE) # make GNU libc happy
add_compile_options(-std=c11 -Wall -Wextra -fno-strict-aliasing)
if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
add_compile_options(-fstack-protector-strong -o aslr -fPIE)
else()
add_compile_options(-fstack-protector-strong -fPIE)
endif()
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")
# Per build type flags.
set(CMAKE_C_FLAGS_DEBUG    "${CMAKE_C_FLAGS} -O0 -g -fno-omit-frame-pointer")
set(CMAKE_C_FLAGS_ASAN     "${CMAKE_C_FLAGS} -O1 -g -fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize=address -fsanitize=undefined")
set(CMAKE_C_FLAGS_VALGRIND "${CMAKE_C_FLAGS_RELEASE} -g -fno-omit-frame-pointer")
set(CMAKE_C_FLAGS_COVERAGE "${CMAKE_C_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage")
if(CMAKE_BUILD_TYPE MATCHES COVERAGE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()
# CMAKE_BUILD_TYPE=GRIM: *very* strict compiler options
set(CMAKE_C_FLAGS_GRIM "${CMAKE_C_FLAGS_DEBUG} -Wextra -pedantic -Wconversion -Wstrict-prototypes -Wcast-qual -Wcast-align -Wshadow -Wredundant-decls -Wundef -Wfloat-equal -Wmissing-include-dirs -Wswitch-default -Wpointer-arith -Wbad-function-cast -Wnested-externs -Wold-style-definition -Wformat=2 -Winit-self -Wwrite-strings -Wmissing-prototypes")
include(CheckCCompilerFlag)
check_c_compiler_flag(-Wsign-conversion HAS_WSIGN_CONVERSION)
check_c_compiler_flag(-Wdouble-promotion HAS_WDOUBLE_PROMOTION)
check_c_compiler_flag(-Wlogical-op HAS_WLOGICAL_OP)
check_c_compiler_flag(-Wjump-misses-init HAS_WJUMP_MISSES_INIT)
if(HAS_WSIGN_CONVERSION)
    set(CMAKE_C_FLAGS_GRIM "${CMAKE_C_FLAGS_GRIM} -Wsign-conversion")
endif()
if(HAS_WDOUBLE_PROMOTION)
    set(CMAKE_C_FLAGS_GRIM "${CMAKE_C_FLAGS_GRIM} -Wdouble-promotion")
endif()
if(HAS_WLOGICAL_OP)
    set(CMAKE_C_FLAGS_GRIM "${CMAKE_C_FLAGS_GRIM} -Wlogical-op")
endif()
if(HAS_WJUMP_MISSES_INIT)
    set(CMAKE_C_FLAGS_GRIM "${CMAKE_C_FLAGS_GRIM} -Wjump-misses-init")
endif()

set(SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/tagutil.c
    ${CMAKE_CURRENT_SOURCE_DIR}/t_action.c
    ${CMAKE_CURRENT_SOURCE_DIR}/t_renamer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/t_editor.c
    ${CMAKE_CURRENT_SOURCE_DIR}/t_loader.c
    ${CMAKE_CURRENT_SOURCE_DIR}/t_tune.c
    ${CMAKE_CURRENT_SOURCE_DIR}/t_taglist.c
    ${CMAKE_CURRENT_SOURCE_DIR}/t_tag.c
    ${CMAKE_CURRENT_SOURCE_DIR}/t_backend.c
    ${CMAKE_CURRENT_SOURCE_DIR}/t_format.c
    ${CMAKE_CURRENT_SOURCE_DIR}/t_toolkit.c
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)
#}}}

#{{{ portability tests
macro(t_try_compile variable test_file compat_file)
    try_compile(${variable}
        ${CMAKE_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/compat/tests/${test_file}
        ${ARGN}
    )
    if(${variable})
        add_definitions(-D${variable})
    else()
        message(STATUS ${compat_file} " needed")
        set(SRCS ${SRCS} ${compat_file})
    endif()
endmacro()

t_try_compile(HAS_GETPROGNAME i_can_haz_getprogname.c  compat/getprogname.c)
t_try_compile(HAS_STRLCPY     i_can_haz_strlcpy.c      compat/strlcpy.c)
t_try_compile(HAS_STRLCAT     i_can_haz_strlcat.c      compat/strlcat.c)
t_try_compile(HAS_STRDUP      i_can_haz_strdup.c       compat/strdup.c)
t_try_compile(HAS_SBUF        i_can_haz_sbuf.c         compat/subr_sbuf.c CMAKE_FLAGS "-DLINK_LIBRARIES=-lsbuf")
t_try_compile(HAS_EACCESS     i_can_haz_eaccess.c      compat/eaccess.c)

#}}}

#{{{ Libraries
include(FindPkgConfig)

# {{{ Required libraries
#
# this sets up:
# REQUIRED_LIBRARIES
# REQUIRED_INCLUDE_DIRS

include("./FindIconv.cmake")
if (ICONV_FOUND)
    set(REQUIRED_LIBRARIES ${REQUIRED_LIBRARIES} ${ICONV_LIBRARIES})
    set(REQUIRED_INCLUDE_DIRS ${REQUIRED_INCLUDE_DIRS} ${ICONV_INCLUDE_DIR})
else()
    message(FATAL_ERROR "iconv not found.")
endif()
if (${ICONV_SECOND_ARGUMENT_IS_CONST})
    add_definitions(-DICONV_SECOND_ARGUMENT_IS_CONST)
endif()

include_directories(${REQUIRED_INCLUDE_DIRS})
# }}}

# {{{ Optional libraries
#
# this sets up:
# OPTIONAL_LIBRARIES
# OPTIONAL_INCLUDE_DIRS
if (HAS_SBUF)
    set(REQUIRED_LIBRARIES ${REQUIRED_LIBRARIES} "-lsbuf")
endif()

# Backends
set(BACKEND_COUNT 0)
set(WITH_TAGLIB NO)
if(NOT DEFINED WITHOUT_TAGLIB)
    pkg_check_modules(TAGLIB taglib_c>=1.4)
    if(TAGLIB_FOUND)
        set(WITH_TAGLIB YES)
        math(EXPR BACKEND_COUNT "${BACKEND_COUNT} + 1")
        add_definitions(-DWITH_TAGLIB)
        set(SRCS ${SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/t_fttaglib.c)
        set(OPTIONAL_LIBRARIES ${OPTIONAL_LIBRARIES} ${TAGLIB_LDFLAGS})
        set(OPTIONAL_INCLUDE_DIRS ${OPTIONAL_INCLUDE_DIRS} ${TAGLIB_INCLUDE_DIRS})
    else()
        message(STATUS "TagLib not found. Disabled.")
    endif()
endif()

set(WITH_FLAC NO)
if(NOT DEFINED WITHOUT_FLAC)
    pkg_check_modules(FLAC flac)
    if(FLAC_FOUND)
        set(WITH_FLAC YES)
        math(EXPR BACKEND_COUNT "${BACKEND_COUNT} + 1")
        add_definitions(-DWITH_FLAC)
        set(SRCS ${SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/t_ftflac.c)
        set(OPTIONAL_LIBRARIES ${OPTIONAL_LIBRARIES} ${FLAC_LDFLAGS})
        set(OPTIONAL_INCLUDE_DIRS ${OPTIONAL_INCLUDE_DIRS} ${FLAC_INCLUDE_DIRS})
    else()
        message(STATUS "libFLAC not found. Disabled.")
    endif()
endif()

set(WITH_OGGVORBIS NO)
if(NOT DEFINED WITHOUT_OGGVORBIS)
    pkg_check_modules(OGG ogg)
    pkg_check_modules(VORBIS vorbis)
    pkg_check_modules(VORBISFILE vorbisfile)
    if(OGG_FOUND AND VORBIS_FOUND AND VORBISFILE_FOUND)
        set(WITH_OGGVORBIS YES)
        math(EXPR BACKEND_COUNT "${BACKEND_COUNT} + 1")
        add_definitions(-DWITH_OGGVORBIS)
        set(SRCS ${SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/t_ftoggvorbis.c)
        set(OPTIONAL_LIBRARIES ${OPTIONAL_LIBRARIES}
            ${OGG_LDFLAGS} ${VORBIS_LDFLAGS} ${VORBISFILE_LDFLAGS})
        set(OPTIONAL_INCLUDE_DIRS ${OPTIONAL_INCLUDE_DIRS}
            ${OGG_INCLUDE_DIRS} ${VORBIS_INCLUDE_DIRS} ${VORBISFILE_INCLUDE_DIRS})
    else()
        message(STATUS "libogg/libvorbis not found. Disabled.")
    endif()
endif()

# disabled by default
if(DEFINED WITH_ID3V1)
    set(WITH_ID3V1 YES)
    math(EXPR BACKEND_COUNT "${BACKEND_COUNT} + 1")
    add_definitions(-DWITH_ID3V1)
    set(SRCS ${SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/t_ftid3v1.c)
else()
    set(WITH_ID3V1 NO)
endif()

# Formats
set(FORMAT_COUNT 0)
set(WITH_JSON NO)

# no way to disable YAML for now
pkg_check_modules(YAML yaml-0.1)
if(YAML_FOUND)
    set(WITH_YAML YES)
    math(EXPR FORMAT_COUNT "${FORMAT_COUNT} + 1")
    set(SRCS ${SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/t_yaml.c)
    set(OPTIONAL_LIBRARIES ${OPTIONAL_LIBRARIES} ${YAML_LDFLAGS})
    set(OPTIONAL_INCLUDE_DIRS ${OPTIONAL_INCLUDE_DIRS} ${YAML_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "libyaml not found.")
endif()


if(NOT DEFINED WITHOUT_JSON)
    pkg_check_modules(JSON jansson)
    if(JSON_FOUND)
        set(WITH_JSON YES)
        math(EXPR FORMAT_COUNT "${FORMAT_COUNT} + 1")
        add_definitions(-DWITH_JSON)
        set(SRCS ${SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/t_json.c)
        set(OPTIONAL_LIBRARIES ${OPTIONAL_LIBRARIES} ${JSON_LDFLAGS})
        set(OPTIONAL_INCLUDE_DIRS ${OPTIONAL_INCLUDE_DIRS} ${JSON_INCLUDE_DIRS})
    else()
        message(STATUS "jansson not found. Disabled.")
    endif()
endif()

include_directories(${OPTIONAL_INCLUDE_DIRS})
#}}}
#}}}

# {{{ Install path and configuration variables
if(DEFINED PREFIX)
    set(PREFIX ${PREFIX} CACHE PATH "install prefix")
    set(CMAKE_INSTALL_PREFIX ${PREFIX})
else()
    set(PREFIX ${CMAKE_INSTALL_PREFIX} CACHE PATH "install prefix")
endif()

# Hide to avoid confusion
mark_as_advanced(CMAKE_INSTALL_PREFIX)
# }}}

#{{{ Link stuff
add_executable(${PROJECT_NAME} ${SRCS})
target_link_libraries(${PROJECT_NAME}
    ${REQUIRED_LIBRARIES}
    ${OPTIONAL_LIBRARIES}
)
#}}}

#{{{ man
set(MAN_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.1)

# set man path
if(DEFINED MAN_PATH)
   set(MAN_PATH ${MAN_PATH} CACHE PATH "manpage directory")
else()
   set(MAN_PATH ${PREFIX}/share/man CACHE PATH "manpage directory")
endif()
#}}}

# {{{ Installation
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
install(FILES ${MAN_SRCS} DESTINATION ${MAN_PATH}/man1)
# }}}

message(STATUS "************ ${PROJECT_NAME} build options ************")
message(STATUS "Backends:")
message(STATUS "  TagLib support:                  ${WITH_TAGLIB}")
message(STATUS "  FLAC (libflac) support:          ${WITH_FLAC}")
message(STATUS "  Ogg/Vorbis (vorbisfile) support: ${WITH_OGGVORBIS}")
message(STATUS "  ID3v1.1 support:                 ${WITH_ID3V1}")
message(STATUS "Formats:")
message(STATUS "   YAML (libyaml) support:         ${WITH_YAML}")
message(STATUS "   JSON (jansson) support:         ${WITH_JSON}")
message(STATUS "***********************************************")

if (NOT BACKEND_COUNT)
    message(FATAL_ERROR "no backend available.")
endif()
if (NOT FORMAT_COUNT)
    message(FATAL_ERROR "no format available.")
endif()
