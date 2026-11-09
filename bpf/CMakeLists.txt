# The BPF task iterator. The kernel's BTF gives vmlinux.h at build time; the
# program is CO-RE, so the binary runs on other kernels with BTF.
#   -DWITH_BPF=ON|OFF       ON by default; OFF reads waits from /proc
#   -DWITH_LIBBPF=system|</path/to/custom/libbpf/installation>
OPTION(WITH_BPF "Read waiting threads with a BPF task iterator; OFF reads /proc at a higher CPU cost" ON)
SET(WITH_LIBBPF_DOC "system (libbpf-dev of the OS) | </path/to/custom/libbpf/installation> (include/bpf/libbpf.h and libbpf.a)")
SET(WITH_LIBBPF "system" CACHE STRING "${WITH_LIBBPF_DOC}")
SET(SSR_BPF_OFF "Or configure with -DWITH_BPF=OFF: waits are then read from /proc at a higher CPU cost.")

IF(NOT WITH_BPF)
    MESSAGE(STATUS "${PROJECT_NAME}: BPF task iterator off (WITH_BPF=OFF), waits are read from /proc")
    RETURN()
ENDIF()

FIND_PROGRAM(SSR_CLANG NAMES clang)
# Ubuntu's /usr/sbin/bpftool is a wrapper that finds nothing for a kernel without
# its linux-tools package: take the first bpftool that answers.
IF(NOT SSR_BPFTOOL)
    FILE(GLOB SSR_BPFTOOL_CANDIDATES /usr/lib/linux-tools/*/bpftool)
    FOREACH(CANDIDATE IN LISTS SSR_BPFTOOL_CANDIDATES ITEMS /usr/sbin/bpftool /usr/bin/bpftool)
        EXECUTE_PROCESS(COMMAND "${CANDIDATE}" version RESULT_VARIABLE BPFTOOL_RESULT OUTPUT_QUIET ERROR_QUIET)
        IF(BPFTOOL_RESULT EQUAL 0)
            SET(SSR_BPFTOOL "${CANDIDATE}" CACHE FILEPATH "bpftool for the BPF skeleton")
            BREAK()
        ENDIF()
    ENDFOREACH()
ENDIF()
# libbpf is linked statically, with headers and libbpf.a from one installation:
# the skeleton is built against the headers and has to run with the same libbpf.
IF(NOT "${WITH_LIBBPF}" STREQUAL "${SSR_LIBBPF_SEARCHED}")
    UNSET(SSR_LIBBPF_INCLUDE CACHE)
    UNSET(SSR_LIBBPF_ARCHIVE CACHE)
    SET(SSR_LIBBPF_SEARCHED "${WITH_LIBBPF}" CACHE INTERNAL "WITH_LIBBPF the libbpf paths were found for")
ENDIF()
IF(WITH_LIBBPF STREQUAL "system")
    FIND_PATH(SSR_LIBBPF_INCLUDE bpf/libbpf.h)
    FIND_LIBRARY(SSR_LIBBPF_ARCHIVE NAMES libbpf.a)
ELSEIF(EXISTS "${WITH_LIBBPF}/include/bpf/libbpf.h")
    FIND_PATH(SSR_LIBBPF_INCLUDE bpf/libbpf.h PATHS "${WITH_LIBBPF}/include" NO_DEFAULT_PATH)
    FIND_LIBRARY(SSR_LIBBPF_ARCHIVE NAMES libbpf.a PATHS "${WITH_LIBBPF}"
        PATH_SUFFIXES lib lib64 lib/${CMAKE_LIBRARY_ARCHITECTURE} NO_DEFAULT_PATH)
    IF(NOT SSR_LIBBPF_ARCHIVE)
        MESSAGE(FATAL_ERROR "${PROJECT_NAME}: cannot find libbpf.a in WITH_LIBBPF=${WITH_LIBBPF} "
            "(lib, lib64, lib/${CMAKE_LIBRARY_ARCHITECTURE}).\n${SSR_BPF_OFF}")
    ENDIF()
ELSE()
    MESSAGE(FATAL_ERROR "${PROJECT_NAME}: wrong option or path for WITH_LIBBPF=${WITH_LIBBPF}.\n"
        "Valid options are: ${WITH_LIBBPF_DOC}")
ENDIF()
FIND_LIBRARY(SSR_LIBELF NAMES elf)
FIND_PACKAGE(ZLIB)
SET(SSR_BTF "/sys/kernel/btf/vmlinux")

# zlib is not checked here: the profile module already requires it.
SET(SSR_BPF_MISSING)
IF(NOT SSR_CLANG)
    LIST(APPEND SSR_BPF_MISSING "clang")
ENDIF()
IF(NOT SSR_BPFTOOL)
    LIST(APPEND SSR_BPF_MISSING "bpftool")
ENDIF()
IF(NOT SSR_LIBBPF_INCLUDE OR NOT SSR_LIBBPF_ARCHIVE)
    LIST(APPEND SSR_BPF_MISSING "libbpf.a")
ENDIF()
IF(NOT SSR_LIBELF)
    LIST(APPEND SSR_BPF_MISSING "libelf")
ENDIF()
IF(SSR_BPF_MISSING)
    LIST(JOIN SSR_BPF_MISSING ", " SSR_BPF_MISSING_TEXT)
    REPORT_MISSING_DEPENDENCIES(FATAL_ERROR "${PROJECT_NAME}: cannot find ${SSR_BPF_MISSING_TEXT} for the BPF task iterator."
        "${SSR_BPF_OFF}")
ENDIF()
IF(NOT EXISTS "${SSR_BTF}")
    MESSAGE(FATAL_ERROR "${PROJECT_NAME}: cannot find ${SSR_BTF}: the kernel of this machine has no BTF "
        "(CONFIG_DEBUG_INFO_BTF), which the BPF task iterator is built from.\n${SSR_BPF_OFF}")
ENDIF()

FILE(STRINGS "${SSR_LIBBPF_INCLUDE}/bpf/libbpf_version.h" SSR_LIBBPF_VERSION_LINES REGEX "define LIBBPF_M(AJOR|INOR)_VERSION")
STRING(REGEX REPLACE ".*MAJOR_VERSION ([0-9]+).*MINOR_VERSION ([0-9]+).*" "\\1.\\2" SSR_LIBBPF_VERSION "${SSR_LIBBPF_VERSION_LINES}")
MESSAGE(STATUS "${PROJECT_NAME}: BPF task iterator on, libbpf ${SSR_LIBBPF_VERSION} ${SSR_LIBBPF_ARCHIVE}, bpftool ${SSR_BPFTOOL}")
SET(BPF_OUT "${CMAKE_CURRENT_BINARY_DIR}")
ADD_CUSTOM_COMMAND(OUTPUT "${BPF_OUT}/vmlinux.h"
    COMMAND sh -c "\"${SSR_BPFTOOL}\" btf dump file ${SSR_BTF} format c > \"${BPF_OUT}/vmlinux.h\""
    VERBATIM)
ADD_CUSTOM_COMMAND(OUTPUT "${BPF_OUT}/taskIterator.bpf.o"
    COMMAND "${SSR_CLANG}" -O2 -g -target bpf -D__TARGET_ARCH_x86 -Wno-missing-declarations
        -I "${BPF_OUT}" -I "${CMAKE_CURRENT_SOURCE_DIR}" -I "${SSR_LIBBPF_INCLUDE}"
        -c "${CMAKE_CURRENT_SOURCE_DIR}/taskIterator.bpf.c" -o "${BPF_OUT}/taskIterator.bpf.o"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/taskIterator.bpf.c" "${CMAKE_CURRENT_SOURCE_DIR}/sTaskRecord.h" "${BPF_OUT}/vmlinux.h"
    VERBATIM)
ADD_CUSTOM_COMMAND(OUTPUT "${BPF_OUT}/taskIterator.skel.h"
    COMMAND sh -c "\"${SSR_BPFTOOL}\" gen skeleton \"${BPF_OUT}/taskIterator.bpf.o\" name taskIterator_bpf > \"${BPF_OUT}/taskIterator.skel.h\""
    DEPENDS "${BPF_OUT}/taskIterator.bpf.o"
    VERBATIM)
ADD_CUSTOM_TARGET(ssr-bpf-skeleton DEPENDS "${BPF_OUT}/taskIterator.skel.h")

ADD_LIBRARY(ssr-bpf STATIC cTaskIterator.cpp)
ADD_DEPENDENCIES(ssr-bpf ssr-bpf-skeleton)
TARGET_INCLUDE_DIRECTORIES(ssr-bpf PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
# Generated by bpftool: the skeleton holds the BPF object as one string literal
# over the 64 KiB that -Wpedantic allows under clang.
TARGET_INCLUDE_DIRECTORIES(ssr-bpf SYSTEM PRIVATE "${BPF_OUT}")
# The skeleton is compiled with the headers found with libbpf.a.
TARGET_INCLUDE_DIRECTORIES(ssr-bpf SYSTEM PRIVATE "${SSR_LIBBPF_INCLUDE}")
TARGET_LINK_LIBRARIES(ssr-bpf PRIVATE ssr-warnings PUBLIC "${SSR_LIBBPF_ARCHIVE}" "${SSR_LIBELF}" ZLIB::ZLIB)
TARGET_COMPILE_DEFINITIONS(ssr-bpf PUBLIC STACK_SAMPLER_HAS_BPF=1)
