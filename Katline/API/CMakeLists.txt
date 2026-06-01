add_library(KatlineAPI OBJECT)

file(GLOB_RECURSE KATLINE_API_MODULE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cppm"
)

target_sources(KatlineAPI
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
            ${KATLINE_API_MODULE_SOURCES}
)

set(KATLINE_API_CXX_FLAGS -ffreestanding -fno-stack-protector -fno-pic -mabi=sysv -mno-80387 -mno-mmx -mno-3dnow -mno-sse -mno-sse2 -mno-red-zone)

target_compile_options(KatlineAPI PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:${CXX_COMPILER_FLAGS} ${KATLINE_API_CXX_FLAGS}>
)

target_link_libraries(KatlineAPI PUBLIC CommonLib)

add_library(KatlineAPIStatic STATIC $<TARGET_OBJECTS:KatlineAPI>)
target_link_libraries(KatlineAPIStatic PUBLIC CommonLib)
