cmake_minimum_required(VERSION 2.6.0)

if(COMMAND cmake_policy)
    cmake_policy(VERSION 2.6)
endif()

set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})

if(NOT EXISTS ${SOURCE_DIR}/tagutil.cmake)
    message(FATAL_ERROR "Please provide tagutil.cmake")
endif()
include(tagutil.cmake)

include_directories(
    ${SOURCE_DIR}
    ${BUILD_DIR}
    ${OPTIONAL_INCLUDE_DIRS}
)

set(SRCS
    tagutil.c
    t_toolkit.c
    t_tag.c
    t_yaml.c
    t_renamer.c
    t_lexer.c
    t_parser.c
    t_filter.c
)

try_compile(HAS_GETPROGNAME
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/build-tests/i_can_haz_getprogname.c)
if(HAS_GETPROGNAME)
    add_definitions(-DHAS_GETPROGNAME)
else()
    set(SRCS ${SRCS} compat/getprogname.c)
endif()

try_compile(HAS_STRLCPY
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/build-tests/i_can_haz_strlcpy.c)
if(HAS_STRLCPY)
    add_definitions(-DHAS_STRLCPY)
else()
    set(SRCS ${SRCS} compat/strlcpy.c)
endif()

try_compile(HAS_STRLCAT
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/build-tests/i_can_haz_strlcat.c)
if(HAS_STRLCAT)
    add_definitions(-DHAS_STRLCAT)
else()
    set(SRCS ${SRCS} compat/strlcat.c)
endif()

try_compile(HAS_STRDUP
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/build-tests/i_can_haz_strdup.c)
if(HAS_STRDUP)
    add_definitions(-DHAS_STRDUP)
else()
    set(SRCS ${SRCS} compat/strdup.c)
endif()

try_compile(HAS_SYS_QUEUE_H
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/build-tests/i_can_haz_sys_queue_h.c)
if(HAS_SYS_QUEUE_H)
    add_definitions(-DHAS_SYS_QUEUE_H)
endif()


if (WITH_TAGLIB)
    set(SRCS ${SRCS} t_ftgeneric.c)
endif()
if (WITH_FLAC)
    set(SRCS ${SRCS} t_ftflac.c)
endif()
if (WITH_OGGVORBIS)
    set(SRCS ${SRCS} t_ftoggvorbis.c)
endif()

set(MAN_SRCS ${SOURCE_DIR}/${PROJECT_NAME}.1)

add_executable(${PROJECT_NAME} ${SRCS})

set_target_properties(${PROJECT_NAME}
    PROPERTIES
    LINK_FLAGS -export-dynamic)

target_link_libraries(${PROJECT_NAME}
    ${REQUIRED_LIBRARIES}
    ${OPTIONAL_LIBRARIES}
)

# {{{ Installation
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
#install(FILES ${AWE_MAN1_FILES} DESTINATION ${MAN_PATH}/man1)
# }}}

# vim: filetype=cmake:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
