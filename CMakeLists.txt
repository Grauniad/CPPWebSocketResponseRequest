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
find_package(WEBSOCKETPP 0.7.0 REQUIRED)
find_package(OpenSSL REQUIRED)

#
# Exported Library (libThreadComms)
#
add_library(CPPWebSocketResponseRequest STATIC
    include/AsyncReqSvrRequest.h
    include/ReqFileList.h
    include/ReqServer.h
    include/ReqSvrRequest.h
    include/http_client.h
    include/http_request.h
    include/io_thread.h
    include/stream_client.h
    src/AsyncReqSvrRequest.cpp
    src/http_client.cpp
    src/http_request.cpp
    src/io_thread.cpp
    src/ReqFileList.cpp
    src/ReqServer.cpp
    src/ReqSvrRequest.cpp
    src/stream_client.cpp
)
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
    ${CPPWebSocketResponseRequest_SOURCE_DIR}/include/AsyncReqSvrRequest.h
    ${CPPWebSocketResponseRequest_SOURCE_DIR}/include/ReqFileList.h
    ${CPPWebSocketResponseRequest_SOURCE_DIR}/include/ReqServer.h
    ${CPPWebSocketResponseRequest_SOURCE_DIR}/include/ReqSvrRequest.h
    ${CPPWebSocketResponseRequest_SOURCE_DIR}/include/http_client.h
    ${CPPWebSocketResponseRequest_SOURCE_DIR}/include/http_request.h
    ${CPPWebSocketResponseRequest_SOURCE_DIR}/include/io_thread.h
    ${CPPWebSocketResponseRequest_SOURCE_DIR}/include/stream_client.h
)

#
# Test Configuration
#
find_package(GTest REQUIRED)

add_executable(requestReply test/requestReply.cpp)
target_link_libraries(requestReply CPPWebSocketResponseRequest GTest::GTest)

enable_testing()
add_test(requestReply requestReply)
