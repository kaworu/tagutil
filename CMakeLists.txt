# {{{ CMake stuff
cmake_minimum_required(VERSION 2.6.0)
if(COMMAND cmake_policy)
    cmake_policy(VERSION 2.6)
endif()
# }}}

#{{{ Basic App setup
set(PROJECT_NAME tagutil)
set(CMAKE_BUILD_TYPE DEBUG)
add_definitions(-DT_TAGUTIL_VERSION="3.0rc0")
project(${PROJECT_NAME} C)

set(SRCS
    tagutil.c
    t_toolkit.c
    t_tag.c
    t_taglist.c
    t_tune.c
    t_yaml.c
    t_json.c
    t_renamer.c
    t_editor.c
    t_loader.c
    t_backend.c
    t_action.c
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

# CFLAGS
add_definitions(-std=c99 -g -Wall)
#}}}

#{{{ portability tests
macro(t_try_compile variable test_file compat_file)
    try_compile(${variable}
        ${CMAKE_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/compat/tests/${test_file}
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

# make GNU libc happy
add_definitions(-D_GNU_SOURCE -D_BSD_SOURCE)
#}}}

#{{{ Libraries
include(FindPkgConfig)

# {{{ Required libraries
#
# this sets up:
# REQUIRED_LIBRARIES
# REQUIRED_INCLUDE_DIR
pkg_check_modules(YAML yaml-0.1)
if(YAML_FOUND)
    set(REQUIRED_LIBRARIES ${REQUIRED_LIBRARIES} ${YAML_LDFLAGS})
    set(REQUIRED_INCLUDE_DIR ${REQUIRED_INCLUDE_DIR} ${YAML_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "libyaml not found.")
endif()

pkg_check_modules(JSON jansson)
if(JSON_FOUND)
    set(REQUIRED_LIBRARIES ${REQUIRED_LIBRARIES} ${JSON_LDFLAGS})
    set(REQUIRED_INCLUDE_DIR ${REQUIRED_INCLUDE_DIR} ${JSON_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "jansson not found.")
endif()

if (HAS_SBUF)
    set(REQUIRED_LIBRARIES ${REQUIRED_LIBRARIES} "-lsbuf")
endif()

include_directories(${REQUIRED_INCLUDE_DIR})
# }}}

# {{{ Optional libraries
#
# this sets up:
# OPTIONAL_LIBRARIES
# OPTIONAL_INCLUDE_DIRS
if(NOT DEFINED WITHOUT_TAGLIB)
    pkg_check_modules(TAGLIB taglib_c>=1.4)
    if(TAGLIB_FOUND)
        add_definitions(-DWITH_TAGLIB)
        set(SRCS ${SRCS} t_fttaglib.c)
        set(OPTIONAL_LIBRARIES ${OPTIONAL_LIBRARIES} ${TAGLIB_LDFLAGS})
        set(OPTIONAL_INCLUDE_DIRS ${OPTIONAL_INCLUDE_DIRS} ${TAGLIB_INCLUDE_DIRS})
    else()
        message(STATUS "TagLib not found. Disabled.")
    endif()
endif()

if(NOT DEFINED WITHOUT_FLAC)
    pkg_check_modules(FLAC flac)
    if(FLAC_FOUND)
        add_definitions(-DWITH_FLAC)
        set(SRCS ${SRCS} t_ftflac.c)
        set(OPTIONAL_LIBRARIES ${OPTIONAL_LIBRARIES} ${FLAC_LDFLAGS})
        set(OPTIONAL_INCLUDE_DIRS ${OPTIONAL_INCLUDE_DIRS} ${FLAC_INCLUDE_DIRS})
        # test if libFLAC assert.h does clash with the system one. FLAC >= 1.3
        # did solve this issue.
        try_compile(FLAC_CLASH_WORKAROUND
            ${CMAKE_BINARY_DIR}
            ${CMAKE_SOURCE_DIR}/compat/tests/libFLAC_assert_clash.c
            CMAKE_FLAGS "-DCOMPILE_DEFINITIONS=${FLAC_LDFLAGS} -DINCLUDE_DIRECTORIES=${FLAC_INCLUDE_DIRS}"
        )
        if(FLAC_CLASH_WORKAROUND)
            # success, no need for workaround
        else()
            message(STATUS "using libFLAC workaround for <assert.h>")
            add_definitions(-DFLAC_CLASH_WORKAROUND)
        endif()
    else()
        message(STATUS "libFLAC not found. Disabled.")
    endif()
endif()

if(NOT DEFINED WITHOUT_OGGVORBIS)
    pkg_check_modules(OGGVORBIS vorbisfile)
    if(OGGVORBIS_FOUND)
        add_definitions(-DWITH_OGGVORBIS)
        set(SRCS ${SRCS} t_ftoggvorbis.c)
        set(OPTIONAL_LIBRARIES ${OPTIONAL_LIBRARIES} ${OGGVORBIS_LDFLAGS})
        set(OPTIONAL_INCLUDE_DIRS ${OPTIONAL_INCLUDE_DIRS} ${OGGVORBIS_INCLUDE_DIRS})
    else()
        message(STATUS "libogg/libvorbis not found. Disabled.")
    endif()
endif()

if(NOT DEFINED WITHOUT_ID3V1)
    add_definitions(-DWITH_ID3V1)
    set(SRCS ${SRCS} t_ftid3v1.c)
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

# set man path
if(DEFINED MAN_PATH)
   set(MAN_PATH ${MAN_PATH} CACHE PATH "manpage directory")
else()
   set(MAN_PATH ${PREFIX}/share/man CACHE PATH "manpage directory")
endif()

# Hide to avoid confusion
mark_as_advanced(CMAKE_INSTALL_PREFIX)
# }}}

#{{{ Link stuff
link_directories(/usr/local/lib)
add_executable(${PROJECT_NAME} ${SRCS})
target_link_libraries(${PROJECT_NAME}
    ${REQUIRED_LIBRARIES}
    ${OPTIONAL_LIBRARIES}
)
#}}}

#{{{ man
set(MAN_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.1)
#}}}

# {{{ Installation
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
install(FILES ${MAN_SRCS} DESTINATION ${MAN_PATH}/man1)
# }}}

# vim: filetype=cmake:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
