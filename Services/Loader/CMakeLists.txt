file(GLOB_RECURSE LOADER_SOURCES CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

dhos_add_executable(Loader
    SOURCES ${LOADER_SOURCES}
    OUTPUT_NAME "Loader.exe"
    INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/cxxstubs/include"
    LINK_LIBRARIES LibC CommonLib KatlineAPI KatlineAPIStatic LibRuntime
    LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/Services/Bootstrap/linker.ld"
    STRIP
)
