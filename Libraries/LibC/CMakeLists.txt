file(GLOB_RECURSE LIBC_MODULE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cppm"
)

add_library(LibC STATIC)

target_sources(LibC
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
            ${LIBC_MODULE_SOURCES}
)

target_include_directories(LibC PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(LibC PUBLIC CommonLib)

set(LIBC_CXX_FLAGS -ffreestanding -fno-stack-protector -fno-pic -fno-omit-frame-pointer -mabi=sysv -mno-80387 -mno-mmx -mno-3dnow -mno-sse -mno-sse2 -mno-red-zone -mcmodel=kernel)

target_compile_options(LibC PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:${CXX_COMPILER_FLAGS} ${LIBC_CXX_FLAGS}>
)
