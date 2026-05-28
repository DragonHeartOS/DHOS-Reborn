set(SOURCES
    Memory/Heap.cpp
    Memory/MemoryManager.cpp
    Controllers/FramebufferController.cpp
    Controllers/SerialController.cpp
    Debug.cpp
    Arch/Interrupts.cpp
    Arch/IDT.cpp
    Arch/LimineStart.cpp
    Katline.cpp)

add_executable(${KERNEL_TARGET} ${SOURCES})
target_link_libraries(${KERNEL_TARGET} PRIVATE CommonLib)
target_include_directories(${KERNEL_TARGET} PUBLIC .)
set_property(TARGET ${KERNEL_TARGET} PROPERTY OUTPUT_NAME Katline)
set_property(TARGET ${KERNEL_TARGET} PROPERTY LINK_FLAGS "-T ${CMAKE_SOURCE_DIR}/Katline/linker.ld -nostdlib -zmax-page-size=0x1000 -static")
set(KERNEL_C_CXX_FLAGS -DKERNEL -ffreestanding -fno-stack-protector -fno-pic -mabi=sysv -mno-80387 -mno-mmx -mno-3dnow -mno-sse -mno-sse2 -mno-red-zone -mcmodel=kernel -MMD)

target_compile_options(${KERNEL_TARGET} PRIVATE
    $<$<COMPILE_LANGUAGE:C>:${C_COMPILER_FLAGS} ${KERNEL_C_CXX_FLAGS}>
    $<$<COMPILE_LANGUAGE:CXX>:${CXX_COMPILER_FLAGS} ${KERNEL_C_CXX_FLAGS}>)
