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

set (vstring "constexpr char pciex_current_version[] { \"${PCIEX_VERSION}\" }\;\n"
             "constexpr char    pciex_current_hash[] { \"${PCIEX_HASH}\" }\;\n"
)

file(WRITE pciex_version.h.tmp ${vstring})

if (NOT EXISTS "pciex_version.h")
    set (gen_ver_missing TRUE)
else ()
    set (gen_ver_missing FALSE)
endif()

if (NOT gen_ver_missing)
    file (SHA256 pciex_version.h v_hash)
    file (SHA256 pciex_version.h.tmp v_tmp_hash)
endif()

if ((gen_ver_missing) OR (NOT ${v_hash} STREQUAL ${v_tmp_hash}))
    file (COPY_FILE pciex_version.h.tmp pciex_version.h)
endif()
