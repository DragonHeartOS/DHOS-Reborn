dhos_add_static_library(LibRuntime
    SOURCES RT.cpp OperatorNewDelete.cpp
    PIC
    INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/cxxstubs/include"
    LINK_LIBRARIES CommonLib KatlineAPI
)
