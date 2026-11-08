file(GLOB_RECURSE PROCESS_MANAGER_SOURCES CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

dhos_add_executable(ProcessManager
    SOURCES ${PROCESS_MANAGER_SOURCES}
    OUTPUT_NAME "ProcessManager.exe"
    INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/cxxstubs/include"
    LINK_LIBRARIES LibC CommonLib KatlineAPI KatlineAPIStatic LibRuntime
    LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/Applications/Bootstrap/linker.ld"
    STRIP
)
