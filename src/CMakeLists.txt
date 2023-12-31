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

find_package(yutovo-calculator REQUIRED)
find_package(yutovo-logger REQUIRED)

if(WIN32)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(mpfr REQUIRED IMPORTED_TARGET mpfr)
    find_package(RapidJSON CONFIG REQUIRED)
elseif(NOT EMSCRIPTEN)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(mpfr REQUIRED IMPORTED_TARGET mpfr)
    pkg_check_modules(gmp REQUIRED IMPORTED_TARGET gmp)
endif()

if(NOT EMSCRIPTEN)
    find_package(Boost REQUIRED COMPONENTS process)
endif()

if(REMOTE_MODE)
    add_executable(${PROJECT_NAME}
        main.cpp
        session.cpp
        service_solver.cpp
        calculator_solver.cpp
        python_solver.cpp
        service_config.cpp
        solver_process.cpp
    )
else()
    add_library(${PROJECT_NAME}
        session.cpp
        service_solver.cpp
        calculator_solver.cpp
        python_solver.cpp
        service_config.cpp
        solver_process.cpp
    )
endif()

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:NDEBUG>)
    target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:/O2>)
endif()

target_compile_options(${PROJECT_NAME} PUBLIC -Wno-deprecated-declarations)
if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8 /bigobj /EHsc /MP /FIwin32_prelude.h")
else()
    if (NOT DEFINED EMSCRIPTEN)
        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
            if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
                add_compile_options(-Og -g -fsanitize=undefined -fsanitize=address -fno-omit-frame-pointer)
                add_definitions(-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE)
                add_definitions(-D_LIBCPP_ENABLE_ASSERTIONS)
                add_link_options(-fsanitize=undefined -fsanitize=address)
            elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -fhardened -Wimplicit-fallthrough -U_FORTIFY_SOURCE -fstrict-flex-arrays=3 -Wtrampolines \
                    -Werror=format-security -Wno-hardened")
            endif()
        else()
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
        endif()
    endif()
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

#the worker process that owns the calculator parsers, not built for emscripten
if(NOT EMSCRIPTEN AND NOT REMOTE_MODE)
    add_executable(${PROJECT_NAME}-calculator-worker
        calculator_worker.cpp
        service_solver.cpp
        calculator_solver.cpp
        python_solver.cpp
        service_config.cpp
        solver_process.cpp
    )

    set_target_properties(${PROJECT_NAME}-calculator-worker PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
    
    if(WIN32)
        target_link_libraries(${PROJECT_NAME}-calculator-worker PRIVATE
            yutovo-calculator
            yutovo-logger
            Boost::process
        )
    else()
        target_link_libraries(${PROJECT_NAME}-calculator-worker PRIVATE
            stdc++
            stdc++fs
            pthread
            yutovo-calculator
            yutovo-logger
            Boost::process
        )
    endif()
    target_include_directories(${PROJECT_NAME}-calculator-worker PUBLIC
        $<$<CONFIG:Debug>:${INSTALL_PATH}/include>
        $<$<CONFIG:Release>:${INSTALL_PATH}/include>
    )
    install(
        TARGETS ${PROJECT_NAME}-calculator-worker
        DESTINATION ${INSTALL_PATH}/bin
        COMPONENT ${PROJECT_NAME}
    )
    if(WIN32)
        target_compile_definitions(${PROJECT_NAME}-calculator-worker PRIVATE $<$<CONFIG:Debug>:NDEBUG>)
        target_compile_options(${PROJECT_NAME}-calculator-worker PRIVATE $<$<CONFIG:Debug>:/O2>)
        #the worker runs the recursive-descent grammar of yutovo-calculator that allows 512 nested levels and needs more stack
        target_link_options(${PROJECT_NAME}-calculator-worker PRIVATE "LINKER:/STACK:16777216")
    endif()
endif()

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
    if(NOT EMSCRIPTEN)
        target_link_libraries(${PROJECT_NAME} PRIVATE Boost::process)
    endif()
else()
    if(WIN32)
        target_link_libraries(${PROJECT_NAME} PRIVATE
            yutovo-calculator
            yutovo-logger
            Boost::process
        )
        target_link_libraries(${PROJECT_NAME} PRIVATE RapidJSON rapidjson)
    else()
        target_link_libraries(${PROJECT_NAME} PRIVATE
            stdc++
            pthread
            yutovo-calculator
            yutovo-logger
        )
        if(NOT EMSCRIPTEN)
            target_link_libraries(${PROJECT_NAME} PRIVATE Boost::process)
        endif()
    endif()
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
    $<$<CONFIG:Debug>:${INSTALL_PATH}/include>
    $<$<CONFIG:Release>:${INSTALL_PATH}/include>
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

if(NOT EMSCRIPTEN)
    if(WIN32)
        set(YUTOVO_DEBUG_LIBRARY "${PROJECT_NAME}d.lib")
        set(YUTOVO_RELEASE_LIBRARY "${PROJECT_NAME}.lib")
    else()
        set(YUTOVO_DEBUG_LIBRARY "lib${PROJECT_NAME}d.a")
        set(YUTOVO_RELEASE_LIBRARY "lib${PROJECT_NAME}.a")
    endif()

    file(GENERATE
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-release.cmake"
        CONTENT "set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
        set_target_properties(${PROJECT_NAME} PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE \"CXX\"
        IMPORTED_LOCATION_RELEASE \"${INSTALL_PATH}/lib/${YUTOVO_RELEASE_LIBRARY}\")"
    )

    install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-release.cmake"
        DESTINATION ${INSTALL_PATH}/lib/cmake/${PROJECT_NAME}
        COMPONENT ${PROJECT_NAME}
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR DEFINED CMAKE_CONFIGURATION_TYPES)
        install(
            FILES $<TARGET_FILE:${PROJECT_NAME}>
            DESTINATION ${INSTALL_PATH}/lib
            RENAME ${YUTOVO_RELEASE_LIBRARY}
            COMPONENT ${PROJECT_NAME}
        )
    endif()
endif()

install(
    FILES
        service_context.h
        session.h
        service_config.h
        types.h
        service_solver.h
        calculator_solver.h
        python_solver.h
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
    # the TLS keys are optional and live outside the repository, so builds without them (CI) must not fail
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../keys/yutovo-solver.crt)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../../keys/yutovo-solver.crt ${CMAKE_CURRENT_BINARY_DIR}/../
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../../keys/yutovo-solver.key ${CMAKE_CURRENT_BINARY_DIR}/../
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../../keys/yutovo-solver.pub ${CMAKE_CURRENT_BINARY_DIR}/../
        )
    endif()
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/yutovo-solver.ini ${CMAKE_CURRENT_BINARY_DIR}/../
    )
endif()
