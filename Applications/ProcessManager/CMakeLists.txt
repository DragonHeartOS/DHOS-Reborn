file(GLOB_RECURSE PROCESS_MANAGER_SOURCES CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(ProcessManager ${PROCESS_MANAGER_SOURCES})
set_target_properties(ProcessManager PROPERTIES POSITION_INDEPENDENT_CODE OFF OUTPUT_NAME "ProcessManager.exe")
target_include_directories(ProcessManager PRIVATE "${CMAKE_SOURCE_DIR}/cxxstubs/include")

target_link_libraries(ProcessManager PRIVATE LibC CommonLib KatlineAPI KatlineAPIStatic LibRuntime)

set(PROCESS_MANAGER_CXX_FLAGS -ffreestanding -fno-pic -fno-pie -fno-stack-protector -mabi=sysv -mno-80387 -mno-mmx -mno-3dnow -mno-sse -mno-sse2 -mno-red-zone)

target_compile_options(ProcessManager PRIVATE
	$<$<COMPILE_LANGUAGE:CXX>:${CXX_COMPILER_FLAGS} ${PROCESS_MANAGER_CXX_FLAGS}>
)

set_property(TARGET ProcessManager PROPERTY LINK_FLAGS "-nostdlib -static -no-pie -T ${CMAKE_SOURCE_DIR}/Applications/Bootstrap/linker.ld")

add_custom_command(TARGET ProcessManager POST_BUILD
	COMMAND ${CMAKE_STRIP} --strip-all $<TARGET_FILE:ProcessManager>
	VERBATIM
)
