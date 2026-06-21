cmake_minimum_required(VERSION 3.28)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

project(dhos VERSION 0.1 LANGUAGES CXX ASM_NASM)

set(WARNING_FLAGS -Wall -Wextra -Wconversion -Wsign-conversion -Wno-varargs)
set(COMMON_COMPILER_FLAGS ${WARNING_FLAGS})
set(CXX_COMPILER_FLAGS ${COMMON_COMPILER_FLAGS} -fno-exceptions -fno-rtti)
set(COMMONLIB_BUILD_KERNEL_VARIANT ON)
set(COMMONLIB_KERNEL_STUB_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/cxxstubs/include")
set(COMMONLIB_KERNEL_CXX_FLAGS -DKERNEL -ffreestanding -fno-stack-protector -fno-pic -fno-omit-frame-pointer -mabi=sysv -mno-80387 -mno-mmx -mno-3dnow -mno-sse -mno-sse2 -mno-red-zone -mcmodel=kernel)
set(COMMONLIB_PLATFORM none CACHE STRING "" FORCE)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_PREFIX "")

set(KERNEL_TARGET katline)

include_directories(${CMAKE_CURRENT_LIST_DIR})

add_subdirectory(vendor/CommonLib)

file(GLOB_RECURSE COMMONLIB_MODULE_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_LIST_DIR}/vendor/CommonLib/src/*.cppm")

add_library(CommonLibKernel OBJECT)

target_sources(CommonLibKernel PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES
                                      ${COMMONLIB_MODULE_SOURCES})
target_sources(CommonLibKernel PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/vendor/CommonLib/src/platforms/none.cpp")

target_include_directories(CommonLib PRIVATE "${COMMONLIB_KERNEL_STUB_INCLUDE_DIR}")
target_include_directories(CommonLibKernel PRIVATE "${COMMONLIB_KERNEL_STUB_INCLUDE_DIR}")
target_compile_options(CommonLibKernel PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:${CXX_COMPILER_FLAGS} ${COMMONLIB_KERNEL_CXX_FLAGS}>)

target_compile_options(CommonLib PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-ffreestanding -fno-stack-protector -fPIC>)

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
