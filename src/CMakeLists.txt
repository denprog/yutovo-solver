cmake_minimum_required(VERSION 3.17)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(yutovo_calculator REQUIRED PATHS ${INSTALL_PATH}/cmake/)

add_executable(${PROJECT_NAME}
    main.cpp
    calc_solver.cpp
    logger.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    stdc++
    stdc++fs
    zmq
    yutovo_calculator
    mpfr
    gmp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ../third_party
)

install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}
    DESTINATION ${INSTALL_PATH}/bin
    COMPONENT ${PROJECT_NAME}
)
