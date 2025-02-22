# Set minimum cmake version required for compiling
cmake_minimum_required(VERSION 3.9)

# Set project
project(EBOT)

# Set platform name
set(CMAKE_SYSTEM_NAME Linux)

# Set build type and library suffix
set(CMAKE_BUILD_TYPE Debug)

# Set build type to shared library (.so file)
set(CMAKE_SHARED_LIBRARY_SUFFIX ".so")

# Set compiler to clang
find_program(CMAKE_CXX_COMPILER clang++)
if(CMAKE_CXX_COMPILER)
    message(STATUS "Found Clang++: ${CMAKE_CXX_COMPILER} and using it as default C++ compiler")
    set(CMAKE_CXX_COMPILER clang++)
endif()

find_program(CMAKE_C_COMPILER clang)
if(CMAKE_C_COMPILER)
    message(STATUS "Found Clang: ${CMAKE_C_COMPILER} and using it as default C compiler")
    set(CMAKE_C_COMPILER clang)
endif()

# Set compiler flags
set(CMAKE_CXX_STANDARD 17)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3 -Og")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -Og -fno-inline -fno-omit-frame-pointer")
else()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Ofast")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -flto -mssse3 -msse3 -msse2 -msse -mmmx -mfpmath=sse -fvisibility=hidden -fno-rtti -Wno-parentheses -pipe -mtune=generic -fomit-frame-pointer -fvisibility-inlines-hidden -funroll-loops -ffunction-sections -fdata-sections -fcf-protection=none -fno-common -fshort-enums -fno-math-errno -fno-plt")
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32") # -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -w -fno-threadsafe-statics") # -fPIC -Wno-write-strings -Wno-format-security -fno-stack-protector -fno-exceptions")


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

# Link options
target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++ -ldl -lm -Bstatic)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_DL_LIBS})

# Set library properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
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
