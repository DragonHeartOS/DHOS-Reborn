set(SOURCES
    CommonLibRuntime.cpp
    CxxAbiStubs.cpp
    Memory/NewDelete.cpp
)

add_executable(${KERNEL_TARGET} ${SOURCES})
file(GLOB_RECURSE KATLINE_MODULE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cppm"
)
target_sources(${KERNEL_TARGET}
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
            ${KATLINE_MODULE_SOURCES}
)
target_sources(${KERNEL_TARGET} PRIVATE $<TARGET_OBJECTS:CommonLib>)
target_link_libraries(${KERNEL_TARGET} PRIVATE CommonLib)
target_include_directories(${KERNEL_TARGET} PUBLIC .)
set_property(TARGET ${KERNEL_TARGET} PROPERTY OUTPUT_NAME Katline)
set_property(TARGET ${KERNEL_TARGET} PROPERTY LINK_FLAGS "-T ${CMAKE_SOURCE_DIR}/Katline/linker.ld -nostdlib -zmax-page-size=0x1000 -static")
set(KERNEL_CXX_FLAGS -DKERNEL -ffreestanding -fno-stack-protector -fno-pic -mabi=sysv -mno-80387 -mno-mmx -mno-3dnow -mno-sse -mno-sse2 -mno-red-zone -mcmodel=kernel)

target_compile_options(${KERNEL_TARGET} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:${CXX_COMPILER_FLAGS} ${KERNEL_CXX_FLAGS}>)
