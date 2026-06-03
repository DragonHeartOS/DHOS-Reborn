add_library(LibRuntime STATIC RT.cpp CommonLibRuntime.cpp OperatorNewDelete.cpp)

set(LIBUTIL_CXX_FLAGS -ffreestanding -fno-stack-protector -fno-pic -mabi=sysv -mno-80387 -mno-mmx -mno-3dnow -mno-sse -mno-sse2 -mno-red-zone -fPIC)

target_compile_options(LibRuntime PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:${CXX_COMPILER_FLAGS} ${LIBUTIL_CXX_FLAGS}>
)

target_link_libraries(LibRuntime PRIVATE CommonLib KatlineAPI)
