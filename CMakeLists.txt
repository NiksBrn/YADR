cmake_minimum_required(VERSION 3.20)
project(yadr LANGUAGES CXX
        VERSION 1.0.0
        DESCRIPTION "Yet Another Desktop Resource monitor — C++ backend + WebUI")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(YADR_BUILD_TESTS    "Build unit tests"                     ON)
option(YADR_BUILD_FRONTEND "Build the frontend via npm + Vite"    ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(backend)

if(YADR_BUILD_FRONTEND)
    find_program(NPM_EXE npm REQUIRED)
    set(FRONTEND_SRC ${CMAKE_SOURCE_DIR}/frontend)
    set(FRONTEND_OUT ${CMAKE_BINARY_DIR}/web)
    set(FRONTEND_STAMP ${CMAKE_BINARY_DIR}/frontend.stamp)

    file(GLOB_RECURSE FRONTEND_INPUTS
        CONFIGURE_DEPENDS
        ${FRONTEND_SRC}/src/*
        ${FRONTEND_SRC}/index.html
        ${FRONTEND_SRC}/package.json
        ${FRONTEND_SRC}/tsconfig.json
        ${FRONTEND_SRC}/vite.config.ts
    )

    add_custom_command(
        OUTPUT ${FRONTEND_STAMP}
        COMMAND ${NPM_EXE} install --no-audit --no-fund --silent
        COMMAND ${NPM_EXE} run build --silent
        COMMAND ${CMAKE_COMMAND} -E rm -rf ${FRONTEND_OUT}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${FRONTEND_SRC}/dist ${FRONTEND_OUT}
        COMMAND ${CMAKE_COMMAND} -E touch ${FRONTEND_STAMP}
        WORKING_DIRECTORY ${FRONTEND_SRC}
        DEPENDS ${FRONTEND_INPUTS}
        COMMENT "Building frontend (Vite) -> ${FRONTEND_OUT}"
        VERBATIM
    )
    add_custom_target(frontend ALL DEPENDS ${FRONTEND_STAMP})
    add_dependencies(yadr-server frontend)
endif()

install(TARGETS yadr-server RUNTIME DESTINATION bin)
if(YADR_BUILD_FRONTEND)
    install(DIRECTORY ${CMAKE_BINARY_DIR}/web/ DESTINATION share/yadr/web)
endif()

message(STATUS "")
message(STATUS "YADR configuration:")
message(STATUS "  Build type     : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Frontend build : ${YADR_BUILD_FRONTEND}")
message(STATUS "  Tests          : ${YADR_BUILD_TESTS}")
message(STATUS "")
