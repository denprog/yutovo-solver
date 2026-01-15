cmake_minimum_required(VERSION 3.10)

project(yutovo-solver
    VERSION 0.1.0 
    LANGUAGES CXX
)

set(PROJECT_LICENSE "GPL-3.0-only")

message(STATUS "Project license: ${PROJECT_LICENSE}")
message(STATUS "CXX Compiler ID: ${CMAKE_CXX_COMPILER_ID}")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG)
endif()

if(WIN32)
    set(INSTALL_PATH "$ENV{VCPKG_ROOT}/installed/x64-windows")
else()
    set(INSTALL_PATH "$ENV{YUTOVO_DEPLOY}")
endif()

# set(REMOTE_MODE true)
# add_compile_definitions(REMOTE_MODE)

add_subdirectory(src)
add_subdirectory(test)
