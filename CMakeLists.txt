cmake_minimum_required(VERSION 3.5)
project(OpcUaTestClient)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

set(SOURCE_FILES
    OpcUaTestClient.cpp
        )


include_directories(.)
include_directories(../1WaOpcUaSdk/include)
include_directories(../build/botan/include/botan-2)
link_directories(/usr/local/lib)
link_directories(/home/osboxes/WorkSpace/botan/lib)

#PI include_directories("D:/SysGCC/raspberry/arm-linux-gnueabihf/sysroot/usr/local/include")
#PI link_directories("D:/SysGCC/raspberry/arm-linux-gnueabihf/sysroot/usr/lib")
#include_directories(../../botan)

#link_directories(./Debug)
#link_directories(/home/osboxes/Downloads/boost_1_61_0/stage/lib)

add_executable(OpcUaTestClient ${SOURCE_FILES})
target_link_libraries (OpcUaTestClient OpcUaSdk botan-2 boost_filesystem boost_system)
