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
