cmake_minimum_required(VERSION 3.10)

project(yutovo_service
    VERSION 0.1.0 
    LANGUAGES CXX
)

add_compile_definitions(DEBUG)

set(INSTALL_PATH "$ENV{YUTOVO_DEPLOY}")

add_subdirectory(src)
add_subdirectory(test)
