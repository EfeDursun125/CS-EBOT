cmake_minimum_required(VERSION 3.5.1)

project(EBOT)

# Set clang++ as the default C++ compiler if available
find_program(CMAKE_CXX_COMPILER clang++)
if(CMAKE_CXX_COMPILER)
    message(STATUS "Found Clang++: ${CMAKE_CXX_COMPILER} and using it as default C++ compiler")
    set(CMAKE_CXX_COMPILER clang++)
else()
    message(STATUS "Clang++ not found using default C++ compiler, this might cause E-Bot crashes")
endif()

# Set clang as the default C compiler if available
find_program(CMAKE_C_COMPILER clang)
if(CMAKE_C_COMPILER)
    message(STATUS "Found Clang: ${CMAKE_C_COMPILER} and using it as default C compiler")
    set(CMAKE_C_COMPILER clang)
else()
    message(STATUS "Clang not found using default C compiler, this might cause E-Bot crashes")
endif()

# Set build type and library suffix
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_SHARED_LIBRARY_SUFFIX ".so")

# Set compiler flags
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -Ofast")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -m32 -msse3 -pthread -Ofast -fvisibility=hidden -fno-exceptions -fno-rtti -Wno-parentheses -pipe -mtune=generic -funroll-loops -fomit-frame-pointer -fno-stack-protector -fvisibility-inlines-hidden -fexceptions")
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -m32")

# Set output directory for the library
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

# Source files
file(GLOB_RECURSE EBOT_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/goap/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../source/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../source/goap/*.cpp
)

# Create the library
add_library(${PROJECT_NAME} SHARED ${EBOT_SRC})

find_package(CURL REQUIRED)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE CURL::libcurl
    ${CMAKE_DL_LIBS}
    ${CMAKE_THREAD_LIBS_INIT}
)

# Set library properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

# Set installation directories
install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../include/ DESTINATION include)

