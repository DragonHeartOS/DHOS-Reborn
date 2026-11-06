file(GLOB_RECURSE LOADER_SOURCES CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(Loader ${LOADER_SOURCES})
set_target_properties(Loader PROPERTIES POSITION_INDEPENDENT_CODE OFF OUTPUT_NAME "Loader.exe")
target_include_directories(Loader PRIVATE "${CMAKE_SOURCE_DIR}/cxxstubs/include")

target_link_libraries(Loader PRIVATE LibC CommonLib KatlineAPI KatlineAPIStatic LibRuntime)

set(LOADER_CXX_FLAGS -ffreestanding -fno-pic -fno-pie -fno-stack-protector -mabi=sysv -mno-80387 -mno-mmx -mno-3dnow -mno-sse -mno-sse2 -mno-red-zone)

target_compile_options(Loader PRIVATE
	$<$<COMPILE_LANGUAGE:CXX>:${CXX_COMPILER_FLAGS} ${LOADER_CXX_FLAGS}>
)

set_property(TARGET Loader PROPERTY LINK_FLAGS "-nostdlib -static -no-pie -T ${CMAKE_SOURCE_DIR}/Applications/Bootstrap/linker.ld")

add_custom_command(TARGET Loader POST_BUILD
	COMMAND ${CMAKE_STRIP} --strip-all $<TARGET_FILE:Loader>
	VERBATIM
)
