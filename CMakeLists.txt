# CMake 最低版本号要求
cmake_minimum_required(VERSION 3.0)
project(ljy)
include_directories(
${PROJECT_SOURCE_DIR}/src/include
)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")
set(CMAKE_C_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
message(${PROJECT_SOURCE_DIR})

file(GLOB_RECURSE LIB_SRC ./src/source/*.cpp)

#set(LIB_SRC
#    src/source/log.cpp
#    src/source/util.cpp
#    src/source/config.cpp
#    src/source/env.cpp
#)

#message(${LIB_SRC})

add_library(ljy SHARED ${LIB_SRC})

set(LIBS
        ljy
        pthread
        yaml-cpp
)

set(DEPEND_LIBS
        ljy
)

add_executable(test_log test/test_log.cpp)
add_dependencies(test_log ${DEPEND_LIBS})
target_link_libraries(test_log ${LIBS})

add_executable(test_config test/test_config.cpp)
add_dependencies(test_config ${DEPEND_LIBS})
target_link_libraries(test_config ${LIBS})

add_executable(test_thread test/test_thread.cpp)
add_dependencies(test_thread ${DEPEND_LIBS})
target_link_libraries(test_thread ${LIBS})