file(GLOB_RECURSE BOOTSTRAP_SOURCES CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(Bootstrap ${BOOTSTRAP_SOURCES})
set_target_properties(Bootstrap PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(Bootstrap PRIVATE "${CMAKE_SOURCE_DIR}/cxxstubs/include")

target_link_libraries(Bootstrap PRIVATE CommonLib KatlineAPI KatlineAPIStatic)

set(BOOTSTRAP_CXX_FLAGS -ffreestanding -fPIC -fno-stack-protector -mabi=sysv -mno-80387 -mno-mmx -mno-3dnow -mno-sse -mno-sse2 -mno-red-zone)

target_compile_options(Bootstrap PRIVATE
	$<$<COMPILE_LANGUAGE:CXX>:${CXX_COMPILER_FLAGS} ${BOOTSTRAP_CXX_FLAGS}>
)

set_property(TARGET Bootstrap PROPERTY LINK_FLAGS "-nostdlib -static -T ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld")

set(BOOTSTRAP_BIN "${CMAKE_CURRENT_BINARY_DIR}/Bootstrap.bin")
set(BOOTSTRAP_ENTRY_HEADER "${CMAKE_CURRENT_BINARY_DIR}/BootstrapEntry.h")

add_custom_command(
	OUTPUT ${BOOTSTRAP_BIN}
	COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:Bootstrap> ${BOOTSTRAP_BIN}
	DEPENDS Bootstrap
	VERBATIM
)

add_custom_command(
	OUTPUT ${BOOTSTRAP_ENTRY_HEADER}
	COMMAND python3 "${CMAKE_SOURCE_DIR}/Util/GenerateBootstrapEntry.py" "${BOOTSTRAP_ENTRY_HEADER}" "$<TARGET_FILE:Bootstrap>"
	DEPENDS Bootstrap "${CMAKE_SOURCE_DIR}/Util/GenerateBootstrapEntry.py"
	VERBATIM
)

add_custom_target(BootstrapImage ALL DEPENDS ${BOOTSTRAP_BIN} ${BOOTSTRAP_ENTRY_HEADER})

set(BOOTSTRAP_BIN_PATH ${BOOTSTRAP_BIN} CACHE INTERNAL "Bootstrap binary output path")
set(BOOTSTRAP_ENTRY_HEADER_PATH ${BOOTSTRAP_ENTRY_HEADER} CACHE INTERNAL "Bootstrap entry header path")
