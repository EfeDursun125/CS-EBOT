cmake_minimum_required(VERSION 3.22.1)

project(EBOT)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_LIBRARIES "-static-libgcc -static-libstdc++ ${CMAKE_CSS_STANDARD_LIBRARIES}")
set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "")
set(CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES "")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -std=c++11 -w -fvisibility=hidden -fno-exceptions -fno-rtti -Wno-parentheses -pipe -mtune=generic -fno-exceptions -fno-rtti -DPOSIX -DNDEBUG=1 -pipe -O2 -march=native -funroll-loops -fomit-frame-pointer -fno-stack-protector -fvisibility=hidden -fvisibility-inlines-hidden -nostdinc++")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_CXX_FLAGS}")
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -m32 -lm -ldl -Wl,-Bstatic,--whole-archive -Wl,--no-whole-archive")

set(EBOT ebot)

file(GLOB EBOT_SRC
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
target_link_libraries(${EBOT} ${CMAKE_DL_LIBS})