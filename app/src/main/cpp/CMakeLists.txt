cmake_minimum_required(VERSION 3.10.2)
project("libcppdumper" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(
    libcppdumper
    SHARED
    il2cpp_dumper.cpp
)

find_library(
    log-lib
    log
)

target_link_libraries(
    libcppdumper
    ${log-lib}
)

# Android NDK specific settings
if(ANDROID)
    target_compile_options(libcppdumper PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -fvisibility=hidden
        -frtti
        -fexceptions
    )
endif()

# Include directories
target_include_directories(libcppdumper PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)