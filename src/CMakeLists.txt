cmake_minimum_required(VERSION 3.17)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(PkgConfig REQUIRED)
find_package(yutovo_calculator REQUIRED PATHS ${INSTALL_PATH}/cmake/)
find_package(yutovo_logger REQUIRED PATHS ${INSTALL_PATH}/lib/cmake/)

add_executable(${PROJECT_NAME}
    main.cpp
    session.cpp
    solver.cpp
    config.cpp
)

target_compile_options(${PROJECT_NAME} PUBLIC -Wno-deprecated-declarations)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    stdc++
    stdc++fs
    pthread
    ssl
    crypto
    yutovo_calculator
    yutovo_logger
)

target_include_directories(${PROJECT_NAME} PUBLIC
    $<INSTALL_INTERFACE:${INSTALL_PATH}/include>
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${THIRD_PARTY_PATH}
)

install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}
    DESTINATION ${INSTALL_PATH}/bin
    COMPONENT ${PROJECT_NAME}
)

install(
    FILES 
        service_context.h
        types.h
    DESTINATION ${INSTALL_PATH}/include/${PROJECT_NAME}
    COMPONENT ${PROJECT_NAME}
)

install(
    EXPORT ${PROJECT_NAME}
    DESTINATION ${INSTALL_PATH}/cmake/${PROJECT_NAME}
    COMPONENT ${PROJECT_NAME}
    FILE ${PROJECT_NAME}.cmake
)

install(
    FILES ${PROJECT_NAME}-config.cmake
    DESTINATION ${INSTALL_PATH}/cmake/${PROJECT_NAME}
    COMPONENT ${PROJECT_NAME}
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../keys/yutovo_service.crt ${CMAKE_CURRENT_BINARY_DIR}/../
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../keys/yutovo_service.key ${CMAKE_CURRENT_BINARY_DIR}/../
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../keys/yutovo_service.pub ${CMAKE_CURRENT_BINARY_DIR}/../
)
