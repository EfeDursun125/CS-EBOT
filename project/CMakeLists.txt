cmake_minimum_required(VERSION 3.16)

project(EBOT)

# Check system architecture and warn if multilib is needed
include(CheckCXXCompilerFlag)
execute_process(COMMAND uname -m OUTPUT_VARIABLE ARCH OUTPUT_STRIP_TRAILING_WHITESPACE)

if(ARCH STREQUAL "x86_64")
    message(STATUS "Detected 64-bit host system: ${ARCH}")
    check_cxx_compiler_flag("-m32" COMPILER_SUPPORTS_M32)
    if(NOT COMPILER_SUPPORTS_M32)
        message(WARNING "Your system is 64-bit but does not support 32-bit (-m32) compilation.")
        message(WARNING "Make sure you have installed 'gcc-multilib' and 'g++-multilib'.")
        message(FATAL_ERROR "Cannot proceed without 32-bit compilation support.")
    endif()
endif()

# Compiler setup
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_SHARED_LIBRARY_SUFFIX ".so")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -g3 -Og")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -g3 -Og -fno-inline -fno-omit-frame-pointer -fno-threadsafe-statics -pthread")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -Ofast")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -m32 -mmmx -msse -msse2 -mfpmath=sse -flto -Ofast -fvisibility=hidden -fno-exceptions -fno-rtti -pipe -mtune=generic -fomit-frame-pointer -fvisibility-inlines-hidden -fno-stack-protector -fno-omit-frame-pointer -no-pie -fno-threadsafe-statics -pthread")
    set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -m32")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# Output directory
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/ebot)

# Includes and sources
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
file(GLOB_RECURSE EBOT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../source/*.cpp)

# Build
add_library(${PROJECT_NAME} SHARED ${EBOT_SRC})

# Dependencies
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE m dl pthread)

# Target properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
    PREFIX ""
    OUTPUT_NAME "ebot"
    INTERPROCEDURAL_OPTIMIZATION ON
)

# Install
install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../include/ DESTINATION include)
