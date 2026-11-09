CMAKE_MINIMUM_REQUIRED(VERSION 3.20)

INCLUDE(cmake/project_version.cmake)
PROJECT(stack-sampler VERSION "${STACK_SAMPLER_VERSION}" LANGUAGES CXX)

INCLUDE("${PROJECT_SOURCE_DIR}/cmake/project_commit.cmake")

SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)

OPTION(DEVELOPER_MODE "Enable developer mode" OFF)
INCLUDE("${PROJECT_SOURCE_DIR}/cmake/developer_mode.cmake")
INCLUDE("${PROJECT_SOURCE_DIR}/cmake/sanitizers.cmake")

# The repository root must not be an include directory: on a case-insensitive
# file system the VERSION file shadows the standard <version> header.
INCLUDE_DIRECTORIES("${PROJECT_SOURCE_DIR}/src")

ADD_LIBRARY(ssr-warnings INTERFACE)
TARGET_COMPILE_OPTIONS(ssr-warnings INTERFACE -Wall -Wextra -Wpedantic -Werror -Wmissing-declarations -pedantic-errors)

# Modules in dependency order, the executable last; sampling is Linux-only.
SET(STACK_SAMPLER_MODULES stack profile src)
IF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    SET(STACK_SAMPLER_MODULES process wait stack cpu profile src)
ENDIF()
FOREACH(MODULE IN LISTS STACK_SAMPLER_MODULES)
    ADD_SUBDIRECTORY(${MODULE})
ENDFOREACH()

INCLUDE(cmake/format.cmake)
ADD_FORMAT_TARGETS(${STACK_SAMPLER_MODULES})

# GPL v2 requires the license text to go with the binary.
INSTALL(FILES README.md LICENSE DESTINATION share/doc/${PROJECT_NAME})

INCLUDE(cmake/packaging.cmake)
