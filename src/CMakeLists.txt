cmake_minimum_required(VERSION 3.17)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
if(WIN32)
    set(SPDLOG_FMT_EXTERNAL ON)
endif()

if(EMSCRIPTEN)
    set(CMAKE_FIND_ROOT_PATH ${INSTALL_PATH}/wasm/cmake/)
elseif(NOT WIN32)
    set(CMAKE_FIND_ROOT_PATH ${INSTALL_PATH}/lib/cmake/)
endif()

if(WIN32)
	set(CMAKE_PREFIX_PATH "${INSTALL_PATH}")
endif()

find_package(PkgConfig REQUIRED)
find_package(yutovo-calculator REQUIRED)
find_package(yutovo-logger REQUIRED)
if(WIN32)
    pkg_check_modules(mpfr REQUIRED IMPORTED_TARGET mpfr)
    find_package(RapidJSON CONFIG REQUIRED)
endif()

if(REMOTE_MODE)
    add_executable(${PROJECT_NAME}
        main.cpp
        session.cpp
        service_solver.cpp
        service_config.cpp
    )
else()
    add_library(${PROJECT_NAME}
        session.cpp
        service_solver.cpp
        service_config.cpp
    )
endif()

if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8 /bigobj")
else()
    target_compile_options(${PROJECT_NAME} PUBLIC -Wno-deprecated-declarations)
endif()

set(CMAKE_DEBUG_POSTFIX d)

if(EMSCRIPTEN)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -s USE_PTHREADS=1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s DISABLE_EXCEPTION_THROWING=0")
    target_link_options(${PROJECT_NAME} PUBLIC "-fexceptions -s DISABLE_EXCEPTION_THROWING=0")
    target_compile_options(${PROJECT_NAME} PUBLIC -Wno-ambiguous-reversed-operator)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES 
    DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

if(REMOTE_MODE)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        stdc++
        stdc++fs
        pthread
        ssl
        crypto
        yutovo-calculator
        yutovo-logger
    )
else()
    if(WIN32)
        target_link_libraries(${PROJECT_NAME} PRIVATE
            yutovo-calculator
            yutovo-logger
        )
        target_link_libraries(${PROJECT_NAME} PRIVATE RapidJSON rapidjson)
    else()
        target_link_libraries(${PROJECT_NAME} PRIVATE
            stdc++
            pthread
            yutovo-calculator
            yutovo-logger
        )
    endif()
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
    $<$<CONFIG:Debug>:${INSTALL_PATH}/include>
    $<$<CONFIG:Release>:${INSTALL_PATH}/include>
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${THIRD_PARTY_PATH}
)

if(EMSCRIPTEN)
    install(
        TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}
        DESTINATION ${INSTALL_PATH}/wasm
        COMPONENT ${PROJECT_NAME}
    )
elseif(REMOTE_MODE)
    install(
        TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}
        DESTINATION ${INSTALL_PATH}/bin
        COMPONENT ${PROJECT_NAME}
    )
else()
    install(
        TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}
        DESTINATION ${INSTALL_PATH}/lib
        COMPONENT ${PROJECT_NAME}
    )
endif()

install(
    FILES 
        service_context.h
        session.h
        service_config.h
        types.h
        service_solver.h
    DESTINATION ${INSTALL_PATH}/include/${PROJECT_NAME}
    COMPONENT ${PROJECT_NAME}
)

if(EMSCRIPTEN)
    install(
        EXPORT ${PROJECT_NAME}
        DESTINATION ${INSTALL_PATH}/wasm/cmake/${PROJECT_NAME}
        COMPONENT ${PROJECT_NAME}
        FILE ${PROJECT_NAME}.cmake
    )
else()
    install(
        EXPORT ${PROJECT_NAME}
        DESTINATION ${INSTALL_PATH}/lib/cmake/${PROJECT_NAME}
        COMPONENT ${PROJECT_NAME}
        FILE ${PROJECT_NAME}.cmake
    )
endif()

if(EMSCRIPTEN)
    install(
        FILES ${PROJECT_NAME}-config.cmake
        DESTINATION ${INSTALL_PATH}/wasm/cmake/${PROJECT_NAME}
        COMPONENT ${PROJECT_NAME}
    )
else()
    install(
        FILES ${PROJECT_NAME}-config.cmake
        DESTINATION ${INSTALL_PATH}/lib/cmake/${PROJECT_NAME}
        COMPONENT ${PROJECT_NAME}
    )
endif()

if(NOT WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../keys/yutovo-solver.crt ${CMAKE_CURRENT_BINARY_DIR}/../
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../keys/yutovo-solver.key ${CMAKE_CURRENT_BINARY_DIR}/../
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../keys/yutovo-solver.pub ${CMAKE_CURRENT_BINARY_DIR}/../
    )
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/yutovo-solver.ini ${CMAKE_CURRENT_BINARY_DIR}/../
    )
endif()
