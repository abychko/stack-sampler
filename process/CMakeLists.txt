ADD_LIBRARY(ssr-process STATIC cMemoryMap.cpp cBuildIdReader.cpp cRemoteMemory.cpp cThreadReader.cpp
    cFileDescriptorReader.cpp)
TARGET_INCLUDE_DIRECTORIES(ssr-process PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
TARGET_LINK_LIBRARIES(ssr-process PRIVATE ssr-warnings)
