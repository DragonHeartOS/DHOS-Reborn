file(GLOB_RECURSE LIBC_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_library(LibC STATIC ${LIBC_SOURCES})

target_include_directories(LibC PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(LibC PUBLIC CommonLib)

set(LIBC_CXX_FLAGS -ffreestanding -fno-stack-protector -fno-pic -fno-omit-frame-pointer -mabi=sysv -mno-80387 -mno-mmx -mno-3dnow -mno-sse -mno-sse2 -mno-red-zone)

target_compile_options(LibC PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:${CXX_COMPILER_FLAGS} ${LIBC_CXX_FLAGS}>
)
