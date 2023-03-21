cmake_minimum_required(VERSION 3.22.1)

project(EBOT)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-parentheses -03 -DNDEBUG=1 -fno-exceptions -fno-rtti -funroll-loops -fomit-frame-pointer -pipe")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -fvisibility=hidden -fno-exceptions -fno-rtti")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32 -lm -ldl")

set(EBOT ebot)

FILE(GLOB EBOT_SRC
source/basecode.cpp
../source/callbacks.cpp
../source/chatlib.cpp
../source/combat.cpp
../source/control.cpp
../source/engine.cpp
../source/globals.cpp
../source/interface.cpp
../source/navigate.cpp
../source/netmsg.cpp
../source/support.cpp
../source/waypoint.cpp
)

include_directories(../include)
add_library(${EBOT} SHARED ${EBOT_SRC})