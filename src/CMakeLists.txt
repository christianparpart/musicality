set(MUSICALITY_SRCS
	main.cpp
)

add_executable(musicality ${MUSICALITY_SRCS})

target_include_directories(musicality PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_definitions(musicality PRIVATE
    MUSICALITY_VERSION_MAJOR=${CMAKE_PROJECT_VERSION_MAJOR}
    MUSICALITY_VERSION_MINOR=${CMAKE_PROJECT_VERSION_MINOR}
    MUSICALITY_VERSION_PATCH=${CMAKE_PROJECT_VERSION_PATCH}
)

if(WIN32)
    if (NOT ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug"))
        set_target_properties(musicality PROPERTIES
            # Tell MSVC to create a GUI application.
            WIN32_EXECUTABLE ON

            # Tell MSVC to use main instead of WinMain for Windows subsystem executables.
            LINK_FLAGS "/ENTRY:mainCRTStartup"
        )
    endif()
elseif(APPLE)
    set(MUSICALITY_version "0.1.0_pre")
    set_target_properties(musicality PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_BUNDLE_NAME "musicality"
        MACOSX_BUNDLE_INFO_STRING "musicality"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.christianparpart.musicality"
        MACOSX_BUNDLE_LONG_VERSION_STRING "${MUSICALITY_version}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${MUSICALITY_version}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${MUSICALITY_version}"
        # TODO: MACOSX_BUNDLE_ICON_FILE "musicality.icns"
        # TODO: RESOURCE "images/icon.icns"
    )
endif()

set(MUSICALITY_LIBRARIES )
target_link_libraries(musicality PRIVATE ${MUSICALITY_LIBRARIES})
