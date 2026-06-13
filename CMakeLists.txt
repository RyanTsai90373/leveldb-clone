cmake_minimum_required(VERSION 3.16)
project(leveldb_clone CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -g -fsanitize=address)
add_link_options(-fsanitize=address)

file(GLOB LIB_SRCS CONFIGURE_DEPENDS db/*.cc util/*.cc)
add_library(leveldb_clone STATIC ${LIB_SRCS})

target_include_directories(leveldb_clone PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

enable_testing()
foreach(test_name db_test arena_test internalkey_test encode_test)
    add_executable(${test_name} test/${test_name}.cc)
    target_link_libraries(${test_name} PRIVATE leveldb_clone)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
