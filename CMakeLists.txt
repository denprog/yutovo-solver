cmake_minimum_required(VERSION 3.10)

project(yutovo_solver
    VERSION 0.1.0 
    LANGUAGES CXX
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG)
endif()

if(WIN32)
    set(INSTALL_PATH "c:/Lang/vcpkg/installed/x64-windows")
else()
    set(INSTALL_PATH "$ENV{YUTOVO_DEPLOY}")
    set(THIRD_PARTY_PATH "$ENV{YUTOVO_DEPLOY}/../third_party/")
endif()

# set(REMOTE_MODE true)
# add_compile_definitions(REMOTE_MODE)

add_subdirectory(src)
add_subdirectory(test)
