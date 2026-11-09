add_subdirectory(Bootstrap)
add_subdirectory(Loader)
add_subdirectory(ProcessManager)

set(DHOS_BOOT_IMAGE_TARGETS Loader ProcessManager)
set(DHOS_BOOT_IMAGE_TARGETS "${DHOS_BOOT_IMAGE_TARGETS}" PARENT_SCOPE)

set(DHOS_BOOT_IMAGE_ARCHIVE "${CMAKE_BINARY_DIR}/boot.tar")
set(DHOS_BOOT_IMAGE_ARGS)
set(DHOS_BOOT_IMAGE_DEPENDS)
foreach(target IN LISTS DHOS_BOOT_IMAGE_TARGETS)
    if(NOT TARGET ${target})
        message(FATAL_ERROR "Boot image target does not exist: ${target}")
    endif()
    list(APPEND DHOS_BOOT_IMAGE_ARGS "${target}=$<TARGET_FILE:${target}>")
    list(APPEND DHOS_BOOT_IMAGE_DEPENDS ${target})
endforeach()

add_custom_command(
    OUTPUT "${DHOS_BOOT_IMAGE_ARCHIVE}"
    COMMAND python3 "${CMAKE_SOURCE_DIR}/Util/BuildBootImage.py"
        "${DHOS_BOOT_IMAGE_ARCHIVE}" ${DHOS_BOOT_IMAGE_ARGS}
    DEPENDS ${DHOS_BOOT_IMAGE_DEPENDS}
        "${CMAKE_CURRENT_LIST_FILE}"
        "${CMAKE_SOURCE_DIR}/Util/BuildBootImage.py"
    VERBATIM
)

add_custom_target(BootImage ALL
    DEPENDS "${DHOS_BOOT_IMAGE_ARCHIVE}"
)
