set(SOURCES
)

add_library(CommonLib OBJECT ${SOURCES})
target_sources(CommonLib
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
            CommonLib.cppm
            Types.cppm
            TypeTraits.cppm
            InitializerList.cppm
            StringView.cppm
            Platform.cppm
            Utility.cppm
            CString.cppm
            Option.cppm
            Iterator.cppm
            Span.cppm
            Range.cppm
            Array.cppm
            ArrayList.cppm
            LinkedList.cppm
            String.cppm
            Math.cppm
            Color.cppm
            Atomic.cppm
            Box.cppm
            Rc.cppm
            Arc.cppm
            Function.cppm
            Variant.cppm
            Result.cppm
            Error.cppm
            HashMap.cppm
)
set(COMMONLIB_CXX_FLAGS -DKERNEL -ffreestanding -fno-stack-protector -fno-pic -mabi=sysv -mno-80387 -mno-mmx -mno-3dnow -mno-sse -mno-sse2 -mno-red-zone -mcmodel=kernel)
target_compile_options(CommonLib PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:${CXX_COMPILER_FLAGS} ${COMMONLIB_CXX_FLAGS}>)
