cmake_minimum_required(VERSION 3.15)
project(zohd VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(BUILD_STATIC "Build static binary" OFF)
option(BUILD_TESTS "Build tests" OFF)

# Platform detection
if(WIN32)
    add_definitions(-DPLATFORM_WINDOWS)
    set(PLATFORM_SOURCES
        src/platform/windows_impl.cpp
    )
    set(PLATFORM_LIBS ws2_32 iphlpapi psapi)
elseif(APPLE)
    add_definitions(-DPLATFORM_MACOS)
    set(PLATFORM_SOURCES
        src/platform/macos_impl.cpp
    )
    set(PLATFORM_LIBS)
elseif(UNIX)
    add_definitions(-DPLATFORM_LINUX)
    set(PLATFORM_SOURCES
        src/platform/linux_impl.cpp
    )
    set(PLATFORM_LIBS)
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/core/port_scanner.cpp
    src/core/process_manager.cpp
    src/core/port_info.cpp
    src/cli/output_formatter.cpp
    ${PLATFORM_SOURCES}
)

# Main executable
add_executable(zohd ${SOURCES})

# Include directories
target_include_directories(zohd PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link libraries
target_link_libraries(zohd PRIVATE ${PLATFORM_LIBS})

# Static linking if requested
if(BUILD_STATIC)
    if(UNIX AND NOT APPLE)
        target_link_options(zohd PRIVATE -static-libgcc -static-libstdc++)
    endif()
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(zohd PRIVATE /W4 /WX-)
else()
    target_compile_options(zohd PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Install
install(TARGETS zohd DESTINATION bin)
