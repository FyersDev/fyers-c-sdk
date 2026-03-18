cmake_minimum_required(VERSION 3.15)
project(fyers-api-c VERSION 3.1.8 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Generate compile_commands.json for linters/IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(BUILD_EXAMPLES "Build example programs (for testing only; excluded from package)" OFF)
option(BUILD_TESTS "Build tests" OFF)

# Find required packages
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

# Find cJSON: pkg-config on Unix, find_package on Windows (vcpkg)
# vcpkg exports cJSONConfig.cmake (capital J) and target "cjson" (no namespace)
if(WIN32)
    find_package(cJSON CONFIG QUIET)
    if(cJSON_FOUND)
        set(CJSON_TARGET cjson)
    else()
        find_package(cjson CONFIG QUIET)
        if(cjson_FOUND)
            set(CJSON_TARGET cjson::cjson)
        else()
            find_package(PkgConfig QUIET)
            if(PkgConfig_FOUND)
                pkg_search_module(CJSON cjson libcjson)
                if(CJSON_FOUND)
                    pkg_search_module(CJSON REQUIRED IMPORTED_TARGET cjson libcjson)
                    set(CJSON_TARGET PkgConfig::CJSON)
                endif()
            endif()
        endif()
    endif()
    if(NOT CJSON_TARGET)
        message(FATAL_ERROR "cJSON not found. On Windows, use vcpkg: vcpkg install cjson")
    endif()
else()
    find_package(PkgConfig REQUIRED)
    pkg_search_module(CJSON REQUIRED IMPORTED_TARGET cjson libcjson)
    set(CJSON_TARGET PkgConfig::CJSON)
endif()

# Find libwebsockets
set(LWS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../libwebsockets")
set(LWS_FOUND FALSE)
if(WIN32)
    find_package(libwebsockets CONFIG QUIET)
    if(libwebsockets_FOUND)
        set(LWS_FOUND TRUE)
        # vcpkg libwebsockets exports target "websockets_shared", not libwebsockets::libwebsockets
        set(LWS_TARGET websockets_shared)
    else()
        find_package(LibWebSockets CONFIG QUIET)
        if(LibWebSockets_FOUND)
            set(LWS_FOUND TRUE)
            set(LWS_TARGET LibWebSockets::LibWebSockets)
        endif()
    endif()
endif()

if(NOT LWS_FOUND AND EXISTS "${LWS_DIR}/CMakeLists.txt")
    # Use libwebsockets from parent directory (Unix)
    set(LWS_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/libwebsockets")
    set(LWS_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/lws_install")

    include(ExternalProject)
    ExternalProject_Add(
        libwebsockets_ext
        SOURCE_DIR "${LWS_DIR}"
        BINARY_DIR "${LWS_BUILD_DIR}"
        INSTALL_DIR "${LWS_INSTALL_DIR}"
        CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX=${LWS_INSTALL_DIR}
            -DLWS_WITHOUT_TESTAPPS=ON
            -DLWS_WITHOUT_TEST_PING=ON
            -DLWS_WITHOUT_TEST_CLIENT=ON
            -DLWS_WITHOUT_TEST_SERVER=ON
            -DLWS_WITHOUT_TEST_SERVER_EXTPOLL=ON
            -DLWS_WITHOUT_TEST_ECHO=ON
            -DLWS_WITHOUT_TEST_FRAGGLE=ON
            -DLWS_WITHOUT_TEST_ECHO_CLIENT=ON
            -DLWS_WITHOUT_TEST_ECHO_SERVER=ON
            -DLWS_WITH_SHARED=OFF
            -DLWS_WITH_STATIC=ON
            -DLWS_WITH_HTTP2=ON
            -DLWS_WITH_LWSWS=OFF
            -DLWS_IPV6=ON
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        BUILD_ALWAYS OFF
    )

    set(LWS_INCLUDE_DIR "${LWS_INSTALL_DIR}/include")
    set(LWS_LIBRARY "${LWS_INSTALL_DIR}/lib/libwebsockets.a")
    set(LWS_TARGET "")
endif()

if(NOT LWS_FOUND AND NOT LWS_INCLUDE_DIR)
    # Try system-installed libwebsockets (Unix)
    find_path(LWS_INCLUDE_DIR
        libwebsockets.h
        PATHS
            /usr/local/include
            /usr/include
            /opt/local/include
            /opt/homebrew/include
    )
    find_library(LWS_LIBRARY
        websockets
        PATHS
            /usr/local/lib
            /usr/lib
            /opt/local/lib
            /opt/homebrew/lib
    )
endif()

if(NOT LWS_TARGET AND (NOT LWS_INCLUDE_DIR OR NOT LWS_LIBRARY))
    message(FATAL_ERROR "libwebsockets not found. Install libwebsockets-dev (Linux), libwebsockets (macOS), or libwebsockets (vcpkg on Windows)")
endif()

# Source files
set(FYERS_CORE_SOURCES
    src/fyers_api.c
    src/fyers_config.c
    src/fyers_logger.c
    src/fyers_http_client.c
    src/fyers_session.c
    src/fyers_model.c
    src/fyers_data_ws.c
    src/fyers_order_ws.c
)

# Create shared library
# macOS: libfyers-api-c.dylib
# Linux: libfyers-api-c.so
# Windows: fyers-api-c.dll + fyers-api-c.lib
add_library(fyers-api-c SHARED ${FYERS_CORE_SOURCES})

# Windows: export symbols via FYERS_API macro
target_compile_definitions(fyers-api-c PRIVATE FYERS_API_EXPORTS)

# Shared library versioning
set_target_properties(fyers-api-c PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# macOS: install name for dynamic loader
if(APPLE)
    set_target_properties(fyers-api-c PROPERTIES
        INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib"
    )
endif()

if(EXISTS "${LWS_DIR}/CMakeLists.txt")
    add_dependencies(fyers-api-c libwebsockets_ext)
endif()

target_include_directories(fyers-api-c
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${LWS_INCLUDE_DIR}
)

target_link_libraries(fyers-api-c PUBLIC
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
    ${CJSON_TARGET}
)

if(LWS_TARGET)
    target_link_libraries(fyers-api-c PUBLIC ${LWS_TARGET})
else()
    target_link_libraries(fyers-api-c PRIVATE ${LWS_LIBRARY})
endif()
if(WIN32)
    target_link_libraries(fyers-api-c PRIVATE ws2_32)
endif()

# Install library and headers
# LIBRARY: .so (Linux), .dylib (macOS)
# ARCHIVE: .a (static), .lib (Windows import library)
# RUNTIME: .dll (Windows)
install(TARGETS fyers-api-c
    EXPORT fyers-api-c-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# Install CMake config for find_package(fyers-api-c)
include(CMakePackageConfigHelpers)

set(CMAKE_INSTALL_CMAKEDIR lib/cmake/fyers-api-c)

install(EXPORT fyers-api-c-targets
    FILE fyers-api-c-targets.cmake
    NAMESPACE fyers-api-c::
    DESTINATION ${CMAKE_INSTALL_CMAKEDIR}
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/fyers-api-c-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/fyers-api-c-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_CMAKEDIR}
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/fyers-api-c-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/fyers-api-c-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/fyers-api-c-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_CMAKEDIR}
)

# Examples (not installed)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
