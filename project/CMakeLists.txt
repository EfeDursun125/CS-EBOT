cmake_minimum_required(VERSION 3.16)

# Set clang++ as the default C++ compiler if available
find_program(CLANG_CXX clang++)
find_program(CLANG_C clang)

if(CLANG_CXX)
    message(STATUS "Found clang++: ${CLANG_CXX}; configuring to use it")
    # Set as cache so CMake uses it when detecting compilers
    set(CMAKE_CXX_COMPILER ${CLANG_CXX} CACHE STRING "C++ compiler" FORCE)
endif()

if(CLANG_C)
    message(STATUS "Found clang: ${CLANG_C}; configuring to use it")
    set(CMAKE_C_COMPILER ${CLANG_C} CACHE STRING "C compiler" FORCE)
endif()

project(EBOT)

# Compiler flags
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_SHARED_LIBRARY_SUFFIX ".so")

# Force 32-bit compilation (we assume toolchain supports it, GitHub test covers this)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3 -Og")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -Og -fno-inline -fno-omit-frame-pointer -fno-threadsafe-statics -fstack-protector-all -pthread")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Ofast")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -mmmx -msse -msse2 -mfpmath=sse -flto -Ofast -fvisibility=hidden -fno-rtti -pipe -mtune=generic -fomit-frame-pointer -fvisibility-inlines-hidden -fno-stack-protector -no-pie -fno-threadsafe-statics -pthread")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
 set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -m32")

# Output dir
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/ebot)

# Include & source
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
file(GLOB_RECURSE EBOT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../source/*.cpp)

# Build shared library
add_library(${PROJECT_NAME} SHARED ${EBOT_SRC})

# Threads
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

# Install paths
install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../include/ DESTINATION include)
