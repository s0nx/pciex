cmake_minimum_required(VERSION 3.20)

find_library(FMT_LIB
    NAMES fmt
    PATHS /lib /usr/lib /usr/lib64
)

if(NOT FMT_LIB)
    message(FATAL_ERROR "fmt library has not been found")
endif()

# FTXUI
include(FetchContent)

set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
FetchContent_Declare(
ftxui
GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
GIT_TAG        main
)

FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
    FetchContent_Populate(ftxui)
    add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()


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
    src/pci_regs.cpp
    src/topology.cpp
    src/util.cpp
    src/ui/screen.cpp
)

target_compile_features(pciex PRIVATE cxx_std_20)
target_compile_options(pciex PRIVATE -Wall -Wextra -pedantic -O3)

target_link_libraries(pciex LINK_PUBLIC fmt)
target_link_libraries(pciex
    PRIVATE ftxui::screen
    PRIVATE ftxui::dom
    PRIVATE ftxui::component
)
