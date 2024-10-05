cmake_minimum_required(VERSION 3.20)

find_library(FMT_LIB
    NAMES fmt
    PATHS /lib /usr/lib /usr/lib64
)

if(NOT FMT_LIB)
    message(FATAL_ERROR "fmt library has not been found")
endif()

find_package(Git)

if (GIT_EXECUTABLE)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
            OUTPUT_VARIABLE GIT_DESCRIBE_VERSION
            RESULT_VARIABLE GIT_DESCRIBE_ERROR_CODE
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (NOT GIT_DESCRIBE_ERROR_CODE)
        set(PCIEX_VERSION ${GIT_DESCRIBE_VERSION})
    endif ()
    execute_process(
            COMMAND ${GIT_EXECUTABLE} log -1 --format=%h
            OUTPUT_VARIABLE GIT_CURRENT_HASH
            RESULT_VARIABLE GIT_HASH_ERROR_CODE
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (NOT GIT_HASH_ERROR_CODE)
        set(PCIEX_HASH ${GIT_CURRENT_HASH})
    endif ()
endif ()

if (NOT DEFINED PCIEX_VERSION)
    set(PCIEX_VERSION 0.0.0)
    message(WARNING "Failed to determine VERSION from Git tags. Using default version \"${PCIEX_VERSION}\".")
endif ()
if (NOT DEFINED PCIEX_HASH)
    set(PCIEX_HASH "0000000")
    message(WARNING "Failed to determine HASH from Git logs. Using default hash \"${PCIEX_HASH}\".")
endif ()

add_definitions(-DPCIEX_CURRENT_VERSION="${PCIEX_VERSION}")
add_definitions(-DPCIEX_CURRENT_HASH="${PCIEX_HASH}")

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
    VERSION ${PCIEX_VERSION}
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
