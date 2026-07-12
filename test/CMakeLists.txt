cmake_minimum_required(VERSION 3.17)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(WIN32)
    set(CMAKE_PREFIX_PATH "${INSTALL_PATH}")
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(mpfr REQUIRED IMPORTED_TARGET mpfr)
    pkg_check_modules(gmp REQUIRED IMPORTED_TARGET gmp)
    pkg_check_modules(gtest REQUIRED gtest)
endif()

enable_testing()

if(EMSCRIPTEN)
    set(CMAKE_FIND_ROOT_PATH ${INSTALL_PATH}/wasm/cmake/)
else()
    set(CMAKE_FIND_ROOT_PATH ${INSTALL_PATH}/lib/cmake/)
endif()

find_package(yutovo-calculator REQUIRED)
find_package(yutovo-logger REQUIRED)

add_executable(${PROJECT_NAME}_tests
    main.cpp
    mock.cpp
    calculator.cpp
    session.cpp
    result_types.cpp
    solvers.cpp
    exceptions.cpp
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME}_tests PUBLIC
        ${PROJECT_NAME}
        yutovo-calculator
        yutovo-logger
    )
    target_link_libraries(${PROJECT_NAME}_tests PRIVATE GTest::gtest GTest::gtest_main)
    target_include_directories(${PROJECT_NAME}_tests PRIVATE
        ../src
        ${INSTALL_PATH}/include
    )
else()
    target_link_libraries(${PROJECT_NAME}_tests PUBLIC
        ${PROJECT_NAME}
        yutovo-calculator
        yutovo-logger
        gtest
        gtest_main
        pthread
        dl
    )
    target_include_directories(${PROJECT_NAME}_tests PRIVATE
        ../src
        ${gtest_INCLUDE_DIRS}
    )
endif()

if(NOT WIN32)
    include(GoogleTest)
    gtest_discover_tests(${PROJECT_NAME}_tests)
endif()

install(
    TARGETS ${PROJECT_NAME}_tests
    EXPORT ${PROJECT_NAME}
    DESTINATION ${INSTALL_PATH}/bin
    COMPONENT ${PROJECT_NAME}
)
