cmake_minimum_required(VERSION 3.10)
project(cpp-markdown)

add_library(cppmarkdown
	src/markdown.h
	src/markdown.cpp
	src/markdown-tokens.h
	src/markdown-tokens.cpp
)
target_include_directories(cppmarkdown PUBLIC src)
target_link_libraries(cppmarkdown PUBLIC boost_regex)

add_executable(cpp-markdown src/main.cpp)
target_link_libraries(cpp-markdown PUBLIC cppmarkdown)


enable_testing()

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(markdown_tests test/markdown_tests.cpp)
target_link_libraries(markdown_tests PUBLIC cppmarkdown GTest::gtest_main)
target_include_directories(markdown_tests PUBLIC src)

include(GoogleTest)
gtest_discover_tests(markdown_tests)