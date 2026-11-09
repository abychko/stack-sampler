CMAKE_MINIMUM_REQUIRED(VERSION 3.20)

INCLUDE(cmake/project_version.cmake)
PROJECT(stack-sampler VERSION "${STACK_SAMPLER_VERSION}" LANGUAGES CXX)

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

SET(STACK_SAMPLER_MODULES src)
FOREACH(MODULE IN LISTS STACK_SAMPLER_MODULES)
    ADD_SUBDIRECTORY(${MODULE})
ENDFOREACH()

INCLUDE(cmake/format.cmake)
ADD_FORMAT_TARGETS(${STACK_SAMPLER_MODULES})
