# CMake 最低版本号要求
cmake_minimum_required(VERSION 3.0)
project(ljy)
include_directories(
${PROJECT_SOURCE_DIR}/src/include
)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
message(${PROJECT_SOURCE_DIR})

set(LIB_SRC
    src/source/log.cpp
    src/source/util.cpp
)

#message(${LIB_SRC})

add_library(ljy SHARED ${LIB_SRC})

set(LIBS
        ljy
        pthread
)

add_executable(test_log test/test_log.cpp)
add_dependencies(test_log ljy)
target_link_libraries(test_log ${LIBS})