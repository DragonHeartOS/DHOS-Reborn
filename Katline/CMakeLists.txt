set(SOURCES
    CommonLibRuntime.cpp
    CxxAbiStubs.cpp
    Memory/NewDelete.cpp
)

file(GLOB_RECURSE KATLINE_MODULE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cppm"
)
list(FILTER KATLINE_MODULE_SOURCES EXCLUDE REGEX "/API/")

set(KERNEL_LINK_FLAGS "-T ${CMAKE_SOURCE_DIR}/Katline/linker.ld -nostdlib -zmax-page-size=0x1000 -static")
set(KERNEL_CXX_FLAGS -DKERNEL -ffreestanding -fno-stack-protector -fno-pic -fno-omit-frame-pointer -mabi=sysv -mno-80387 -mno-mmx -mno-3dnow -mno-sse -mno-sse2 -mno-red-zone -mcmodel=kernel)

add_library(${KERNEL_TARGET}_core OBJECT ${SOURCES})
target_sources(${KERNEL_TARGET}_core
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
            ${KATLINE_MODULE_SOURCES}
)
target_link_libraries(${KERNEL_TARGET}_core PRIVATE CommonLib KatlineAPI)
target_include_directories(${KERNEL_TARGET}_core PUBLIC .)
target_compile_definitions(${KERNEL_TARGET}_core PRIVATE KATLINE_BOOTSTRAP_BIN_PATH="${BOOTSTRAP_BIN_PATH}")
target_include_directories(${KERNEL_TARGET}_core PRIVATE "${CMAKE_BINARY_DIR}/Applications/Bootstrap")
target_compile_options(${KERNEL_TARGET}_core PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:${CXX_COMPILER_FLAGS} ${KERNEL_CXX_FLAGS}>)
add_dependencies(${KERNEL_TARGET}_core BootstrapImage)

add_executable(${KERNEL_TARGET}_seed
    $<TARGET_OBJECTS:${KERNEL_TARGET}_core>
    $<TARGET_OBJECTS:CommonLib>
)
target_link_libraries(${KERNEL_TARGET}_seed PRIVATE CommonLib KatlineAPI)
target_include_directories(${KERNEL_TARGET}_seed PUBLIC .)
set_property(TARGET ${KERNEL_TARGET}_seed PROPERTY OUTPUT_NAME Katline.seed)
set_property(TARGET ${KERNEL_TARGET}_seed PROPERTY LINK_FLAGS "${KERNEL_LINK_FLAGS}")

set(KERNEL_SYMBOLS_CPP "${CMAKE_CURRENT_BINARY_DIR}/GeneratedKernelSymbols.cpp")
set(KERNEL_SYMBOL_SOURCE_ARG "")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(KERNEL_SYMBOL_SOURCE_ARG "--with-source")
endif()
add_custom_command(
    OUTPUT ${KERNEL_SYMBOLS_CPP}
    COMMAND python3 "${CMAKE_SOURCE_DIR}/Util/GenerateKernelSymbols.py" ${KERNEL_SYMBOL_SOURCE_ARG} --project-root "${CMAKE_SOURCE_DIR}" "$<TARGET_FILE:${KERNEL_TARGET}_seed>" "${KERNEL_SYMBOLS_CPP}"
    DEPENDS ${KERNEL_TARGET}_seed "${CMAKE_SOURCE_DIR}/Util/GenerateKernelSymbols.py"
    VERBATIM
)

add_executable(${KERNEL_TARGET}
    ${KERNEL_SYMBOLS_CPP}
    $<TARGET_OBJECTS:${KERNEL_TARGET}_core>
    $<TARGET_OBJECTS:CommonLib>
)
target_link_libraries(${KERNEL_TARGET} PRIVATE CommonLib KatlineAPI)
target_include_directories(${KERNEL_TARGET} PUBLIC .)
set_property(TARGET ${KERNEL_TARGET} PROPERTY OUTPUT_NAME Katline)
set_property(TARGET ${KERNEL_TARGET} PROPERTY LINK_FLAGS "${KERNEL_LINK_FLAGS}")
target_compile_options(${KERNEL_TARGET} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:${CXX_COMPILER_FLAGS} ${KERNEL_CXX_FLAGS}>)
add_dependencies(${KERNEL_TARGET} BootstrapImage)
