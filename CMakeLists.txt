cmake_minimum_required(VERSION 3.10)
project(cpp-markdown)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

find_package(Threads REQUIRED)
find_library(RDKAFKA_LIB rdkafka++)

add_executable(conversion-service service/src/main.cpp service/src/message_handler.cpp)
target_include_directories(conversion-service PRIVATE service/include)
target_link_libraries(conversion-service PUBLIC cppmarkdown Threads::Threads ${RDKAFKA_LIB})

add_executable(message_handler_tests test/message_handler_tests.cpp service/src/message_handler.cpp)
target_include_directories(message_handler_tests PUBLIC service/src)
target_link_libraries(message_handler_tests PUBLIC GTest::gtest_main)
gtest_discover_tests(message_handler_tests)