# {{{ CMake stuff
cmake_minimum_required(VERSION 2.6.0)
if(COMMAND cmake_policy)
    cmake_policy(VERSION 2.6)
endif()
# }}}

#{{{ Basic App setup
set(PROJECT_NAME tagutil)
set(CMAKE_BUILD_TYPE DEBUG)
add_definitions(-DT_TAGUTIL_VERSION="3.0")
project(${PROJECT_NAME} C)

set(SRCS
    tagutil.c
    t_action.c
    t_renamer.c
    t_editor.c
    t_loader.c
    t_tune.c
    t_taglist.c
    t_tag.c
    t_backend.c
    t_format.c
    t_toolkit.c
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
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
# REQUIRED_INCLUDE_DIRS
pkg_check_modules(YAML yaml-0.1)
if(YAML_FOUND)
    set(WITH_YAML YES)
    set(SRCS ${SRCS} t_yaml.c)
    set(REQUIRED_LIBRARIES ${REQUIRED_LIBRARIES} ${YAML_LDFLAGS})
    set(REQUIRED_INCLUDE_DIRS ${REQUIRED_INCLUDE_DIRS} ${YAML_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "libyaml not found.")
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
if(NOT DEFINED WITHOUT_TAGLIB)
    pkg_check_modules(TAGLIB taglib_c>=1.4)
    if(TAGLIB_FOUND)
        set(WITH_TAGLIB YES)
        add_definitions(-DWITH_TAGLIB)
        set(SRCS ${SRCS} t_fttaglib.c)
        set(OPTIONAL_LIBRARIES ${OPTIONAL_LIBRARIES} ${TAGLIB_LDFLAGS})
        set(OPTIONAL_INCLUDE_DIRS ${OPTIONAL_INCLUDE_DIRS} ${TAGLIB_INCLUDE_DIRS})
    else()
        set(WITH_TAGLIB NO)
        message(STATUS "TagLib not found. Disabled.")
    endif()
endif()

if(NOT DEFINED WITHOUT_FLAC)
    pkg_check_modules(FLAC flac)
    if(FLAC_FOUND)
        set(WITH_FLAC YES)
        add_definitions(-DWITH_FLAC)
        set(SRCS ${SRCS} t_ftflac.c)
        set(OPTIONAL_LIBRARIES ${OPTIONAL_LIBRARIES} ${FLAC_LDFLAGS})
        set(OPTIONAL_INCLUDE_DIRS ${OPTIONAL_INCLUDE_DIRS} ${FLAC_INCLUDE_DIRS})
        # test if libFLAC assert.h does clash with the system one. FLAC >= 1.3
        # did solve this issue.
        # FIXME: not working correctly, false positive with libflac 1.3
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
        set(WITH_FLAC NO)
        message(STATUS "libFLAC not found. Disabled.")
    endif()
endif()

if(NOT DEFINED WITHOUT_OGGVORBIS)
    pkg_check_modules(OGGVORBIS vorbisfile)
    if(OGGVORBIS_FOUND)
        set(WITH_OGGVORBIS YES)
        add_definitions(-DWITH_OGGVORBIS)
        set(SRCS ${SRCS} t_ftoggvorbis.c)
        set(OPTIONAL_LIBRARIES ${OPTIONAL_LIBRARIES} ${OGGVORBIS_LDFLAGS})
        set(OPTIONAL_INCLUDE_DIRS ${OPTIONAL_INCLUDE_DIRS} ${OGGVORBIS_INCLUDE_DIRS})
    else()
        set(WITH_OGGVORBIS NO)
        message(STATUS "libogg/libvorbis not found. Disabled.")
    endif()
endif()

if(NOT DEFINED WITHOUT_ID3V1)
    set(WITH_ID3V1 YES)
    add_definitions(-DWITH_ID3V1)
    set(SRCS ${SRCS} t_ftid3v1.c)
else()
    set(WITH_ID3V1 NO)
endif()

# Output
if(NOT DEFINED WITHOUT_JSON)
    pkg_check_modules(JSON jansson)
    if(JSON_FOUND)
        set(WITH_JSON YES)
        add_definitions(-DWITH_JSON)
        set(SRCS ${SRCS} t_json.c)
        set(OPTIONAL_LIBRARIES ${OPTIONAL_LIBRARIES} ${JSON_LDFLAGS})
        set(OPTIONAL_INCLUDE_DIRS ${OPTIONAL_INCLUDE_DIRS} ${JSON_INCLUDE_DIRS})
    else()
        set(WITH_JSON NO)
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
message(STATUS "Output:")
message(STATUS "   YAML (libyaml) support:         ${WITH_YAML}")
message(STATUS "   JSON (jansson) support:         ${WITH_JSON}")
message(STATUS "***********************************************")
# vim: filetype=cmake:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
