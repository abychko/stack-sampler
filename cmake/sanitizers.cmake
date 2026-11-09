INCLUDE_GUARD(GLOBAL)

SET(SANITIZER "<NONE>" CACHE STRING "Sanitizer to build with")
SET_PROPERTY(CACHE SANITIZER PROPERTY STRINGS <NONE> ASAN MSAN TSAN UBSAN)

INCLUDE(CheckCXXCompilerFlag)

IF(SANITIZER STREQUAL "<NONE>")
    SET(STACK_SAMPLER_SANITIZER_FLAG "")
ELSEIF(SANITIZER STREQUAL "ASAN")
    SET(STACK_SAMPLER_SANITIZER_FLAG "-fsanitize=address")
ELSEIF(SANITIZER STREQUAL "MSAN")
    SET(STACK_SAMPLER_SANITIZER_FLAG "-fsanitize=memory")
ELSEIF(SANITIZER STREQUAL "TSAN")
    # On a kernel with 32-bit mmap randomization (vm.mmap_rnd_bits=32,
    # default from 6.x) a TSAN binary dies with "unexpected memory mapping".
    # Run it under `setarch x86_64 -R` (ASLR off).
    SET(STACK_SAMPLER_SANITIZER_FLAG "-fsanitize=thread")
ELSEIF(SANITIZER STREQUAL "UBSAN")
    SET(STACK_SAMPLER_SANITIZER_FLAG "-fsanitize=undefined")
ELSE()
    MESSAGE(FATAL_ERROR "Unknown SANITIZER value '${SANITIZER}': use one of <NONE> ASAN MSAN TSAN UBSAN")
ENDIF()

IF(NOT SANITIZER STREQUAL "<NONE>")
    # CHECK_CXX_COMPILER_FLAG only tests the compile step; a sanitizer flag
    # also needs its runtime library at link time (undefined ___asan_init
    # otherwise), so it's repeated via CMAKE_REQUIRED_LINK_OPTIONS.
    SET(CMAKE_REQUIRED_LINK_OPTIONS "${STACK_SAMPLER_SANITIZER_FLAG}")
    CHECK_CXX_COMPILER_FLAG("${STACK_SAMPLER_SANITIZER_FLAG}" STACK_SAMPLER_SANITIZER_SUPPORTED)
    UNSET(CMAKE_REQUIRED_LINK_OPTIONS)
    IF(NOT STACK_SAMPLER_SANITIZER_SUPPORTED)
        MESSAGE(FATAL_ERROR "${SANITIZER} (${STACK_SAMPLER_SANITIZER_FLAG}) is not supported by ${CMAKE_CXX_COMPILER}")
    ENDIF()

    # Directory-scoped, so every target of the project gets it: a sanitizer build
    # needs every dependency instrumented the same way, or the boundary
    # with uninstrumented code produces false reports.
    ADD_COMPILE_OPTIONS("${STACK_SAMPLER_SANITIZER_FLAG}")
    ADD_LINK_OPTIONS("${STACK_SAMPLER_SANITIZER_FLAG}")

    IF(SANITIZER STREQUAL "ASAN")
        # Recommended alongside ASan: without a frame pointer its stack
        # unwinder can't reliably produce backtraces.
        ADD_COMPILE_OPTIONS(-fno-omit-frame-pointer)
    ENDIF()
ENDIF()

IF(DEVELOPER_MODE)
    MESSAGE(STATUS "<DEVELOPER MODE> -> SANITIZER=${SANITIZER}")
ENDIF()
