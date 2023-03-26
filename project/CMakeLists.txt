cmake_minimum_required(VERSION 3.22.1)

project(EBOT)

# Set compiler flags
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-w -m32 -O2 -fvisibility=hidden -fno-exceptions -fno-rtti -Wno-parentheses -pipe -mtune=generic -funroll-loops -fomit-frame-pointer -fno-stack-protector -fvisibility-inlines-hidden")
set(CMAKE_CXX_LINK_FLAGS "-m32")

# Set build type and library suffix
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_SHARED_LIBRARY_SUFFIX ".so")

# Set linker flags for static linking
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")

# Set output directory for the library
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Source files
file(GLOB_RECURSE EBOT_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../source/*.cpp
)

# Create the library
add_library(${PROJECT_NAME} SHARED ${EBOT_SRC})

# Link libraries
target_link_libraries(${PROJECT_NAME}
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
