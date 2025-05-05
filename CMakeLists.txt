cmake_minimum_required(VERSION 3.13)

project(lox-interpreter)

file(GLOB_RECURSE SOURCE_FILES src/*.c src/*.h)

set(CMAKE_C_STANDARD 23) # Enable the C23 standard

add_executable(interpreter ${SOURCE_FILES})
target_link_libraries(interpreter m)
target_compile_options(interpreter PRIVATE -Werror -Wall -Wextra)
