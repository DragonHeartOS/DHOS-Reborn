cmake_minimum_required(VERSION 3.28)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

project(dhos VERSION 0.1 LANGUAGES CXX ASM_NASM)

set(WARNING_FLAGS -Wall -Wextra -Wconversion -Wsign-conversion -Wno-varargs)
set(COMMON_COMPILER_FLAGS ${WARNING_FLAGS})
set(CXX_COMPILER_FLAGS ${COMMON_COMPILER_FLAGS} -fno-exceptions -fno-rtti)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_PREFIX "")

set(KERNEL_TARGET katline)

include_directories(${CMAKE_CURRENT_LIST_DIR})

add_subdirectory(CommonLib)
add_subdirectory(Katline/API)
add_subdirectory(Libraries)
add_subdirectory(Applications)
add_subdirectory(Katline)

add_custom_target(image
    COMMAND "${CMAKE_SOURCE_DIR}/Util/BuildDiskImage.sh"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    DEPENDS ${KERNEL_TARGET}
    USES_TERMINAL
)

add_custom_target(cdrom
    COMMAND "${CMAKE_SOURCE_DIR}/Util/BuildCDROMImage.sh"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    DEPENDS ${KERNEL_TARGET}
    USES_TERMINAL
)

add_custom_target(run
    DEPENDS run-cdrom
)

add_custom_target(run-cdrom
    COMMAND "${CMAKE_SOURCE_DIR}/Util/Run.sh"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    DEPENDS cdrom
    USES_TERMINAL
)

add_custom_target(run-image
    COMMAND "${CMAKE_SOURCE_DIR}/Util/Run.sh"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    DEPENDS image
    USES_TERMINAL
)
