set(SOURCES
)

add_library(CommonLib OBJECT ${SOURCES})
file(GLOB_RECURSE COMMONLIB_MODULE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cppm"
)
target_sources(CommonLib
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
            ${COMMONLIB_MODULE_SOURCES}
)
set(COMMONLIB_CXX_FLAGS -DKERNEL -ffreestanding -fno-stack-protector -fno-pic -mabi=sysv -mno-80387 -mno-mmx -mno-3dnow -mno-sse -mno-sse2 -mno-red-zone -mcmodel=kernel)
target_compile_options(CommonLib PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:${CXX_COMPILER_FLAGS} ${COMMONLIB_CXX_FLAGS}>)
