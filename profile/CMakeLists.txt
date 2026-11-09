FIND_PACKAGE(ZLIB)
IF(NOT ZLIB_FOUND)
    REPORT_MISSING_DEPENDENCIES(FATAL_ERROR "Cannot find zlib development libraries.")
ENDIF()

ADD_LIBRARY(ssr-profile STATIC cProtobufEncoder.cpp cProfileBuilder.cpp cGzipFile.cpp)
TARGET_INCLUDE_DIRECTORIES(ssr-profile PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
TARGET_LINK_LIBRARIES(ssr-profile PRIVATE ssr-warnings ZLIB::ZLIB)
