file(GLOB_RECURSE BOOTSTRAP_SOURCES CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(Bootstrap ${BOOTSTRAP_SOURCES})
set_target_properties(Bootstrap PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(Bootstrap PRIVATE CommonLib KatlineAPI KatlineAPIStatic)

set(BOOTSTRAP_CXX_FLAGS -ffreestanding -fPIC -fno-stack-protector -mabi=sysv -mno-80387 -mno-mmx -mno-3dnow -mno-sse -mno-sse2 -mno-red-zone)

target_compile_options(Bootstrap PRIVATE
	$<$<COMPILE_LANGUAGE:CXX>:${CXX_COMPILER_FLAGS} ${BOOTSTRAP_CXX_FLAGS}>
)

set_property(TARGET Bootstrap PROPERTY LINK_FLAGS "-nostdlib -static -T ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld")

set(BOOTSTRAP_BIN "${CMAKE_CURRENT_BINARY_DIR}/Bootstrap.bin")

add_custom_command(
	OUTPUT ${BOOTSTRAP_BIN}
	COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:Bootstrap> ${BOOTSTRAP_BIN}
	DEPENDS Bootstrap
	VERBATIM
)

add_custom_target(BootstrapImage ALL DEPENDS ${BOOTSTRAP_BIN})

set(BOOTSTRAP_BIN_PATH ${BOOTSTRAP_BIN} CACHE INTERNAL "Bootstrap binary output path")
