include(FetchContent)

find_package(Threads REQUIRED)

# nlohmann/json (header-only)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(nlohmann_json)

# crow
set(CROW_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(CROW_BUILD_TESTS    OFF CACHE INTERNAL "")
set(CROW_INSTALL        OFF CACHE INTERNAL "")
FetchContent_Declare(
    crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG        v1.2.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(crow)

# crow использует asio standalone -- на ubuntu 22.04 пакет libasio-dev кладёт заголовки в /usr/include
find_path(ASIO_INCLUDE_DIR asio.hpp HINTS /usr/include /usr/local/include)
if(NOT ASIO_INCLUDE_DIR)
    message(FATAL_ERROR
        "asio standalone headers not found.\n"
        "On Ubuntu: sudo apt install -y libasio-dev")
endif()
message(STATUS "Using asio headers from: ${ASIO_INCLUDE_DIR}")

# статическая библиотека с ядром -- выносим чтобы тесты линковались без main
add_library(yadr_core STATIC
    src/proc_utils.cpp
    src/snapshot.cpp
    src/sampler.cpp
    src/server.cpp
    src/collectors/cpu.cpp
    src/collectors/memory.cpp
    src/collectors/system.cpp
    src/collectors/network.cpp
    src/collectors/disk.cpp
    src/collectors/process.cpp
)
target_include_directories(yadr_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(yadr_core SYSTEM PUBLIC ${ASIO_INCLUDE_DIR})
target_link_libraries(yadr_core PUBLIC nlohmann_json::nlohmann_json Crow::Crow Threads::Threads)
target_compile_features(yadr_core PUBLIC cxx_std_20)
target_compile_options(yadr_core PRIVATE
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wno-sign-conversion
)

add_executable(yadr-server src/main.cpp)
target_link_libraries(yadr-server PRIVATE yadr_core)

if(YADR_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
