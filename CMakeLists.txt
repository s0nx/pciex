cmake_minimum_required(VERSION 3.20)

project(
    pciex
    LANGUAGES CXX
)

# fmtlib
find_package(fmt REQUIRED)

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

add_executable(pciex)

# includes
target_include_directories(pciex PRIVATE src)

# include output dir, where pciex_version.h is generated
target_include_directories(pciex PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

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

# creates pciex_version.h using cmake script
add_custom_command(
    OUTPUT git_verhdr_gen
    COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
                         -P ${CMAKE_CURRENT_SOURCE_DIR}/gitver.cmake
)

# custom target, which is always considered out ouf date
add_custom_target(
    gitverhdr ALL
    DEPENDS git_verhdr_gen
)

# explicitly say that the executable depends on the gitverhdr
add_dependencies(pciex gitverhdr)
