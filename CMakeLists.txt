# CMake 最低版本号要求
cmake_minimum_required(VERSION 3.0)
project(ljy)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include (cmake/utils.cmake)
include_directories(
        ${PROJECT_SOURCE_DIR}/src/base/include
        ${PROJECT_SOURCE_DIR}/src/http/include
        ${PROJECT_SOURCE_DIR}/src/streams/include
)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")
set(CMAKE_C_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
#message(${PROJECT_SOURCE_DIR})

file(GLOB_RECURSE LIB_SRC src/base/source/*.cpp src/http/source/*.cpp src/streams/source/*.cpp)
#message(WARNING ${CMAKE_CURRENT_SOURCE_DIR})

ragelmaker(src/http/source/http11_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/http/source)
ragelmaker(src/http/source/httpclient_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/http/source)
#message(WARNING ${LIB_SRC})
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()
#set(LIB_SRC
#    src/source/log.cpp
#    src/source/util.cpp
#    src/source/config.cpp
#    src/source/env.cpp
#)

#message(${LIB_SRC})
find_library(PTHREAD pthread)


add_library(ljy SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(ljy)


set(LIBS
        ljy
        dl
        pthread
        yaml-cpp
        ${OPENSSL_LIBRARIES}
        
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

add_executable(test_fiber test/test_fiber.cpp)
add_dependencies(test_fiber ${DEPEND_LIBS})
target_link_libraries(test_fiber ${LIBS})

add_executable(test_scheduler test/test_scheduler.cpp)
add_dependencies(test_scheduler ${DEPEND_LIBS})
target_link_libraries(test_scheduler ${LIBS})

add_executable(test_iomanager test/test_iomanager.cpp)
add_dependencies(test_iomanager ${DEPEND_LIBS})
target_link_libraries(test_iomanager ${LIBS})

add_executable(test_hook test/test_hook.cpp)
add_dependencies(test_hook ${DEPEND_LIBS})
target_link_libraries(test_hook ${LIBS})

add_executable(test_address test/test_address.cpp)
add_dependencies(test_address ${DEPEND_LIBS})
target_link_libraries(test_address ${LIBS})

add_executable(test_socket test/test_socket.cpp)
add_dependencies(test_socket ${DEPEND_LIBS})
target_link_libraries(test_socket ${LIBS})

add_executable(test_bytearray test/test_bytearray.cpp)
add_dependencies(test_bytearray ${DEPEND_LIBS})
target_link_libraries(test_bytearray ${LIBS})

add_executable(test_http test/test_http.cpp)
add_dependencies(test_http ${DEPEND_LIBS})
target_link_libraries(test_http ${LIBS})

add_executable(test_http_paraser test/test_http_paraser.cpp)
add_dependencies(test_http_paraser ${DEPEND_LIBS})
target_link_libraries(test_http_paraser ${LIBS})

add_executable(test_tcp_server test/test_tcp_server.cpp)
add_dependencies(test_tcp_server ${DEPEND_LIBS})
target_link_libraries(test_tcp_server ${LIBS})

add_executable(test_http_server test/test_http_server.cpp)
add_dependencies(test_http_server ${DEPEND_LIBS})
target_link_libraries(test_http_server ${LIBS})

add_executable(test_http_client test/test_http_client.cpp)
add_dependencies(test_http_client ${DEPEND_LIBS})
target_link_libraries(test_http_client ${LIBS})