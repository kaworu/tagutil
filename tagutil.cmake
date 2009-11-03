set(PROJECT_NAME tagutil)
project(${PROJECT_NAME} C)

set(CMAKE_BUILD_TYPE DEBUG)

link_directories(/usr/local/lib)

# {{{ CFLAGS
add_definitions(-std=c99 -g -Wall)
# }}}

# pkg-config
include(FindPkgConfig)

# {{{ Required libraries
macro(a_find_library variable library)
    find_library(${variable} ${library})
    if(NOT ${variable})
        message(FATAL_ERROR ${library} " library not found.")
    endif()
endmacro()

# Check for libYAML
a_find_library(YAML yaml)

set(REQUIRED_LIBRARIES ${YAML})
# }}}

# {{{ Optional libraries
#
# this sets up:
# OPTIONAL_LIBRARIES
# OPTIONAL_INCLUDE_DIRS
pkg_check_modules(TAGLIB taglib_c>=1.4)
if(TAGLIB_FOUND)
    add_definitions(-DWITH_TAGLIB)
    set(WITH_TAGLIB ON)
    set(OPTIONAL_LIBRARIES ${OPTIONAL_LIBRARIES} ${TAGLIB_LDFLAGS})
    set(OPTIONAL_INCLUDE_DIRS ${OPTIONAL_INCLUDE_DIRS} ${TAGLIB_INCLUDE_DIRS})
else()
    set(WITH_TAGLIB OFF)
    message(STATUS "TagLib not found. Disabled.")
endif()

pkg_check_modules(FLAC flac)
if(FLAC_FOUND)
    add_definitions(-DWITH_FLAC)
    set(WITH_FLAC ON)
    set(OPTIONAL_LIBRARIES ${OPTIONAL_LIBRARIES} ${FLAC_LDFLAGS})
    set(OPTIONAL_INCLUDE_DIRS ${OPTIONAL_INCLUDE_DIRS} ${FLAC_INCLUDE_DIRS})
else()
    set(WITH_FLAC OFF)
    message(STATUS "libflac not found. Disabled.")
endif()

pkg_check_modules(OGGVORBIS vorbisfile)
if(OGGVORBIS_FOUND)
    add_definitions(-DWITH_OGGVORBIS)
    set(WITH_OGGVORBIS ON)
    set(OPTIONAL_LIBRARIES ${OPTIONAL_LIBRARIES} ${OGGVORBIS_LDFLAGS})
    set(OPTIONAL_INCLUDE_DIRS ${OPTIONAL_INCLUDE_DIRS} ${OGGVORBIS_INCLUDE_DIRS})
else()
    set(WITH_OGGVORBIS OFF)
    message(STATUS "libogg/libvorbis not found. Disabled.")
endif()

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
# vim: filetype=cmake:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
