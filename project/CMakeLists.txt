cmake_minimum_required(VERSION 3.5.1)

project(EBOT)

# Set CMP0069 policy if available
if(POLICY CMP0069)
  cmake_policy(SET CMP0069 NEW)
endif()

# Use clang++ if available
find_program(CLANGPP clang++)
if(CLANGPP)
  message(STATUS "Found Clang++: ${CLANGPP} — using as default C++ compiler")
  set(CMAKE_CXX_COMPILER ${CLANGPP})
else()
  message(STATUS "Clang++ not found — using default C++ compiler")
endif()

# Use clang if available
find_program(CLANGC clang)
if(CLANGC)
  message(STATUS "Found Clang: ${CLANGC} — using as default C compiler")
  set(CMAKE_C_COMPILER ${CLANGC})
else()
  message(STATUS "Clang not found — using default C compiler")
endif()

# Default build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# 32-bit compilation and optimization flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -g3 -Og")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -g3 -Og -fno-inline -fno-omit-frame-pointer -fno-threadsafe-statics -pthread")
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -Ofast")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -m32 -mmmx -msse -msse2 -mfpmath=sse -flto -Ofast -fvisibility=hidden -fno-exceptions -fno-rtti -Wno-parentheses -pipe -mtune=generic -fomit-frame-pointer -fvisibility-inlines-hidden -fno-stack-protector -fno-omit-frame-pointer -no-pie -fno-threadsafe-statics -pthread")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32")
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# Output path for the shared library
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/ebot)
set(CMAKE_SHARED_LIBRARY_SUFFIX ".so")

# Include paths
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

# Source files
file(GLOB_RECURSE EBOT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../source/*.cpp)

# Define library
add_library(${PROJECT_NAME} SHARED ${EBOT_SRC})

# Pthread support
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Linking
target_link_libraries(${PROJECT_NAME} PRIVATE m dl pthread)

# Library properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
    PREFIX ""
    OUTPUT_NAME "ebot"
    INTERPROCEDURAL_OPTIMIZATION ON
)

# Install targets
install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../include/ DESTINATION include)
