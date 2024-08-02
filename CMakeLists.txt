cmake_minimum_required(VERSION 3.10)

project(yutovo_service
    VERSION 0.1.0 
    LANGUAGES CXX
)

add_compile_definitions(DEBUG)

set(INSTALL_PATH "$ENV{YUTOVO_DEPLOY}")
set(THIRD_PARTY_PATH "$ENV{YUTOVO_DEPLOY}/../third_party/")

# set(REMOTE_MODE true)
# add_compile_definitions(REMOTE_MODE)

add_subdirectory(src)
add_subdirectory(test)
