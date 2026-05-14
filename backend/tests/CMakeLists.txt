# googletest подтягивается через FetchContent -- системный gtest не нужен
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
    GIT_SHALLOW    TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(yadr_tests
    test_proc_parsing.cpp
    test_to_json.cpp
)
target_link_libraries(yadr_tests PRIVATE yadr_core GTest::gtest_main)
target_compile_features(yadr_tests PRIVATE cxx_std_20)

include(GoogleTest)
gtest_discover_tests(yadr_tests)
