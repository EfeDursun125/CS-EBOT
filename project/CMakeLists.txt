# Set minimum cmake version required for compiling
cmake_minimum_required(VERSION 3.9)

# Set project
project(EBOT)

# Set platform name
set(CMAKE_SYSTEM_NAME Linux)

# Set build type and library suffix
set(CMAKE_BUILD_TYPE Release)

# Set build type to shared library (.so file)
set(CMAKE_SHARED_LIBRARY_SUFFIX ".so")

# Set clang++ as the default C++ compiler if available
find_program(CMAKE_CXX_COMPILER clang++)
if(CMAKE_CXX_COMPILER)
find_program(CMAKE_C_COMPILER clang)
if(CMAKE_C_COMPILER)
message(STATUS "Found Clang++: ${CMAKE_CXX_COMPILER} and using it as default C++ compiler")
set(CMAKE_CXX_COMPILER clang++)
message(STATUS "Found Clang: ${CMAKE_C_COMPILER} and using it as default C compiler")
set(CMAKE_C_COMPILER clang)
else()
message(STATUS "Clang not found using default C compiler, this might cause E-Bot crashes")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libstdc++ -static-libgcc -flive-range-shrinkage -fgraphite-identity -floop-nest-optimize")
endif()
else()
message(STATUS "Clang++ not found using default C++ compiler, this might cause E-Bot crashes")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libstdc++ -static-libgcc -flive-range-shrinkage -fgraphite-identity -floop-nest-optimize")
endif()

# Set compiler flags
set(CMAKE_CXX_STANDARD 11)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Og")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Og")
else()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Ofast -flto")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -flto")
endif()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -w -mssse3 -msse3 -msse2 -msse -mmmx -mfpmath=sse -ldl -lm -fvisibility=hidden -fno-exceptions -fno-rtti -Wno-parentheses -fno-pie -pipe -mtune=generic -march=x86-64-v2 -fomit-frame-pointer -fvisibility-inlines-hidden -funroll-loops -fno-stack-protector -ffunction-sections -fdata-sections -fcf-protection=none -fno-finite-math-only -ffast-math -ffp-contract=fast")

# Set output directory for the library
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/ebot)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

# Source files
file(GLOB_RECURSE EBOT_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/../source/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../source/ssm/*.cpp
)

# Create the library
add_library(${PROJECT_NAME} SHARED ${EBOT_SRC})

# Set shared library to off
set(BUILD_SHARED_LIBS OFF)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${CMAKE_DL_LIBS}
)

# Set library properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
    PREFIX ""
    OUTPUT_NAME "ebot"
    LINK_SEARCH_START_STATIC ON
    LINK_SEARCH_END_STATIC ON
    INTERPROCEDURAL_OPTIMIZATION ON
)

# Set installation directories
install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../include/ DESTINATION include)

# Give some detail about the build
message(STATUS "Current Build Flags:${CMAKE_CXX_FLAGS}")
