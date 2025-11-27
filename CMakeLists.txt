cmake_minimum_required(VERSION 3.15)
project(EGMEmulator VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler options
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Find required packages
find_package(Threads REQUIRED)

# Option to build tests
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_SIMULATOR "Build simulator variant" ON)

# Detect Zeus OS platform (check for characteristic device)
if(EXISTS "/dev/ttymxc4")
    set(ZEUS_OS ON)
    add_definitions(-DZEUS_OS)
    message(STATUS "Zeus OS platform detected - enabling Zeus integration")
else()
    set(ZEUS_OS OFF)
    message(STATUS "Building for standard platform - Zeus features will be stubbed")
endif()

# Source files
set(COMMON_SOURCES
    src/event/EventService.cpp
    src/simulator/Game.cpp
    src/simulator/Machine.cpp
    src/io/CommChannel.cpp
    src/io/MachineCommPort.cpp
    src/sas/SASConstants.cpp
    src/sas/CRC16.cpp
    src/sas/BCD.cpp
    src/sas/SASCommands.cpp
    src/sas/SASCommPort.cpp
    src/sas/SASDaemon.cpp
    src/sas/commands/MeterCommands.cpp
    src/sas/commands/EnableCommands.cpp
    src/sas/commands/ExceptionCommands.cpp
    src/sas/commands/DateTimeCommands.cpp
    src/sas/commands/TITOCommands.cpp
    src/sas/commands/AFTCommands.cpp
    src/sas/commands/ProgressiveCommands.cpp
)

# Platform-specific sources
if(ZEUS_OS)
    # Zeus OS platform
    list(APPEND COMMON_SOURCES
        src/io/ZeusSerialPort.cpp
        src/io/ZeusPlatform.cpp
    )
else()
    # Simulated platform (for development/testing)
    list(APPEND COMMON_SOURCES
        src/io/SimulatedPlatform.cpp
    )
endif()

# Create main library
add_library(egm_core STATIC ${COMMON_SOURCES})
target_include_directories(egm_core PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

# Link libraries
if(ZEUS_OS)
    # Link Zeus S7Lite library
    target_link_libraries(egm_core
        Threads::Threads
        s7lite
    )
else()
    target_link_libraries(egm_core
        Threads::Threads
    )
endif()

# Simulator executable
if(BUILD_SIMULATOR)
    add_executable(egm_simulator
        src/simulator/main.cpp
    )
    target_link_libraries(egm_simulator egm_core)
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Install targets
install(TARGETS egm_core
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)
