#
# Project Definition
#
cmake_minimum_required(VERSION 3.8.2)
project(CPPWebSocketResponseRequest)

#
# Find dependencies
#
find_package(ThreadComms 0.1 REQUIRED)
find_package(FixedJSON 0.1 REQUIRED)
find_package(DevToolsLog 0.1 REQUIRED)
find_package(OSDevTools 0.1 REQUIRED)
find_package(UtilTime 0.1 REQUIRED)
find_package(Boost 0.1 REQUIRED COMPONENTS system thread)
find_package(WEBSOCKETPP 0.8.2 REQUIRED)
find_package(OpenSSL REQUIRED)

add_library(CPPWebSocketResponseRequest STATIC
    include/ReqFileList.h
    include/ReqServer.h
    include/ReqSvrRequest.h
    include/WebPPSingleThreadOneShotClient.h
    include/io_thread.h
    include/stream_client.h
    include/IOneShotConnectionConsumer.h
    include/OpenConnections.h
    include/ClientUtils.h
    src/io_thread.cpp
    src/ReqFileList.cpp
    src/ReqServer.cpp
    src/ReqSvrRequest.cpp
    src/WebPPSingleThreadOneShotClient.cpp
    src/stream_client.cpp
    src/OpenConnections.cpp
    src/ClientUtils.cpp)

target_link_libraries(CPPWebSocketResponseRequest PUBLIC
    FixedJSON::FixedJSON
    ThreadComms::ThreadComms
    DevToolsLog::Log
    OSDevTools::OSDevTools
    Boost::system
    Boost::thread
    Boost::boost
    OpenSSL::SSL
    OpenSSL::Crypto
)
target_include_directories(CPPWebSocketResponseRequest PUBLIC
    $<BUILD_INTERFACE:${CPPWebSocketResponseRequest_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

set_property(TARGET CPPWebSocketResponseRequest PROPERTY PUBLIC_HEADER
    ${CPPWebSocketResponseRequest_SOURCE_DIR}/include/ReqFileList.h
    ${CPPWebSocketResponseRequest_SOURCE_DIR}/include/ReqServer.h
    ${CPPWebSocketResponseRequest_SOURCE_DIR}/include/ReqSvrRequest.h
    ${CPPWebSocketResponseRequest_SOURCE_DIR}/include/WebPPSingleThreadOneShotClient.h
    ${CPPWebSocketResponseRequest_SOURCE_DIR}/include/io_thread.h
    ${CPPWebSocketResponseRequest_SOURCE_DIR}/include/stream_client.h
    ${CPPWebSocketResponseRequest_SOURCE_DIR}/include/IOneShotConnectionConsumer.h
    ${CPPWebSocketResponseRequest_SOURCE_DIR}/include/OpenConnections.h
)

add_executable(req src/req.cpp)
target_link_libraries(req CPPWebSocketResponseRequest)

#
# Test Configuration
#
find_package(GTest REQUIRED)

add_executable(requestReply test/requestReply.cpp)
target_link_libraries(requestReply CPPWebSocketResponseRequest GTest::GTest)

add_executable(sub test/sub.cpp)
target_link_libraries(sub CPPWebSocketResponseRequest GTest::GTest)

enable_testing()
add_test(requestReply requestReply)
add_test(sub sub)

file(COPY ${CPPWebSocketResponseRequest_SOURCE_DIR}/test/msgs/helloWorld.msg DESTINATION ${CPPWebSocketResponseRequest_BINARY_DIR})
#
# NOTE: Valgrind must be configured *before* testing is imported
#
set(MEMORYCHECK_COMMAND_OPTIONS "--gen-suppressions=all --suppressions=${CPPWebSocketResponseRequest_SOURCE_DIR}/valgrind_suppress.txt --track-origins=yes --freelist-vol=2000000000 --error-exitcode=1 --track-fds=yes --num-callers=50 --fullpath-after= --trace-children=yes --leak-check=full" )
find_program(MEMORYCHECK_COMMAND valgrind )
include (CTest)

#
# Installation instructions
#
install(TARGETS CPPWebSocketResponseRequest req EXPORT CPPWebSocketResponseRequestTargets
    RUNTIME  DESTINATION bin
    ARCHIVE  DESTINATION lib
    INCLUDES DESTINATION include
    PUBLIC_HEADER DESTINATION include
    )

install (EXPORT CPPWebSocketResponseRequestTargets
    FILE         CPPWebSocketResponseRequestTargets.cmake
    NAMESPACE    CPPWebSocketResponseRequest::
    DESTINATION  lib/cmake/CPPWebSocketResponseRequest
    )

#
# Define our package version
#
include(CMakePackageConfigHelpers)
write_basic_package_version_file("CPPWebSocketResponseRequestConfigVersion.cmake"
    VERSION       0.1
    COMPATIBILITY SameMajorVersion
)
install (
    FILES       ${CPPWebSocketResponseRequest_BINARY_DIR}/CPPWebSocketResponseRequestConfigVersion.cmake
    DESTINATION lib/cmake/CPPWebSocketResponseRequest
)

#
# Finally export the Config which CMake will use
# when looking for matching libraries to link against.
#
install (
    FILES       CPPWebSocketResponseRequestConfig.cmake
    DESTINATION lib/cmake/CPPWebSocketResponseRequest
)

#
# Configure Coverage Build
#
SET(CMAKE_CXX_FLAGS_COVERAGE
    "${GCC_DEBUG_FLAGS} -fprofile-arcs -ftest-coverage"
    CACHE STRING "Flags used by the C++ compiler during coverage builds."
    FORCE )
SET(CMAKE_C_FLAGS_COVERAGE
    "${GCC_DEBUG_FLAGS} -fprofile-arcs -ftest-coverage"
    CACHE STRING "Flags used by the C compiler during coverage builds."
    FORCE )
MARK_AS_ADVANCED(
    CMAKE_CXX_FLAGS_COVERAGE
    CMAKE_C_FLAGS_COVERAGE)
