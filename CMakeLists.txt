cmake_minimum_required(VERSION 3.20)

project(
    pciex
    LANGUAGES CXX
)

# fmtlib
find_package(fmt REQUIRED)

include(FetchContent)

# FTXUI
FetchContent_Declare(
    ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
    GIT_TAG        main
)
FetchContent_MakeAvailable(ftxui)

# CLI11
FetchContent_Declare(
    cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG main
)
FetchContent_MakeAvailable(cli11)

# XXX: it seems target_compile_features(... cxx_std_20) doesn't propagate
# version to cli11, so set the version explicitly
set(CMAKE_CXX_STANDARD 20)

add_executable(pciex)

# includes
target_include_directories(pciex PRIVATE src)

# include output dir, where pciex_version.h is generated
target_include_directories(pciex PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# src
target_sources(pciex PRIVATE
    src/config.cpp
    src/ids_parse.cpp
    src/linux-sysfs.cpp
    src/log.cpp
    src/main.cpp
    src/pci_topo.cpp
    src/pci_dev.cpp
    src/pci_regs.cpp
    src/snapshot.cpp
    src/util.cpp
    src/ui/common_comp.cpp
    src/ui/compat_cap_comp.cpp
    src/ui/ext_cap_comp.cpp
    src/ui/screen.cpp
)

target_compile_features(pciex PUBLIC cxx_std_20)
target_compile_options(pciex PRIVATE -Wall -Wextra -pedantic -O3)

target_link_libraries(pciex PRIVATE fmt)
target_link_libraries(pciex
    PRIVATE ftxui::screen
    PRIVATE ftxui::dom
    PRIVATE ftxui::component
)

target_link_libraries(pciex PRIVATE CLI11::CLI11)

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
