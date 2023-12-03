cmake_minimum_required(VERSION 3.20)

project(
    pciex
    VERSION 0.01
    LANGUAGES CXX
)

add_executable(pciex)

# includes
target_include_directories(pciex PRIVATE src)

# src
target_sources(pciex PRIVATE
    src/ids_parse.cpp
    src/linux-sysfs.cpp
    src/main.cpp
    src/pciex.cpp
    src/topology.cpp
    src/util.cpp
)

target_compile_features(pciex PRIVATE cxx_std_20)
target_compile_options(pciex PRIVATE -Wall -Wextra -pedantic -O3)

target_link_libraries (pciex LINK_PUBLIC fmt)
