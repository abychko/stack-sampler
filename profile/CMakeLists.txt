FIND_PACKAGE(ZLIB REQUIRED)

ADD_LIBRARY(ssr-profile STATIC cProtobufEncoder.cpp cProfileBuilder.cpp cGzipFile.cpp)
TARGET_INCLUDE_DIRECTORIES(ssr-profile PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
TARGET_LINK_LIBRARIES(ssr-profile PRIVATE ssr-warnings ZLIB::ZLIB)
