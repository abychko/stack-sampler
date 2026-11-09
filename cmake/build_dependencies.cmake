INCLUDE_GUARD(GLOBAL)

# Build dependencies as Ubuntu packages. What the build cannot find is reported
# with the command that installs them all.
SET(SSR_BUILD_PACKAGES build-essential cmake git zlib1g-dev clang libbpf-dev libelf-dev linux-tools-generic)

# cmake -P cmake/build_dependencies.cmake prints the list.
IF(CMAKE_SCRIPT_MODE_FILE STREQUAL CMAKE_CURRENT_LIST_FILE)
    EXECUTE_PROCESS(COMMAND "${CMAKE_COMMAND}" -E echo ${SSR_BUILD_PACKAGES})
ENDIF()

# REPORT_MISSING_DEPENDENCIES(<WARNING|FATAL_ERROR> <what is missing> [<other way out>])
FUNCTION(REPORT_MISSING_DEPENDENCIES MODE TEXT)
    LIST(JOIN SSR_BUILD_PACKAGES " " PACKAGES)
    SET(OTHER "")
    IF(ARGC GREATER 2)
        SET(OTHER "${ARGV2}")
    ENDIF()
    MESSAGE(${MODE} "${TEXT} Install the build dependencies:\n"
        "  Ubuntu: apt install ${PACKAGES}\n${OTHER}")
ENDFUNCTION()
