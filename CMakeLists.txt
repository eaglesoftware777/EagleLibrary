#[[
  Eagle Library -- CMake Build Configuration
  Copyright (c) 2026 Eagle Software. All rights reserved.
]]

cmake_minimum_required(VERSION 3.20)

project(EagleLibrary
    VERSION 1.0.0
    DESCRIPTION "Eagle Software - Professional eBook & Document Library Manager"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# ── Locate Qt6 ───────────────────────────────────────────────────────────────
# Priority: CMAKE_PREFIX_PATH (already set) > env vars > auto-scan C:/Qt
# CI sets QT_DIR via the ci-release preset; local VS/Ninja builds auto-detect.
if(WIN32 AND NOT CMAKE_PREFIX_PATH)
    foreach(_env_var QT_DIR Qt6_DIR QTDIR)
        if(DEFINED ENV{${_env_var}} AND NOT "$ENV{${_env_var}}" STREQUAL "")
            set(CMAKE_PREFIX_PATH "$ENV{${_env_var}}" CACHE PATH "Qt6 prefix (from env ${_env_var})" FORCE)
            message(STATUS "Qt6: using ${_env_var} = ${CMAKE_PREFIX_PATH}")
            break()
        endif()
    endforeach()
endif()

if(WIN32 AND NOT CMAKE_PREFIX_PATH)
    file(GLOB _qt6_candidates
        "C:/Qt/6.*/msvc2022_64"
        "C:/Qt/6.*/msvc2019_64"
        "$ENV{USERPROFILE}/Qt/6.*/msvc2022_64"
        "$ENV{USERPROFILE}/Qt/6.*/msvc2019_64"
    )
    if(_qt6_candidates)
        list(SORT _qt6_candidates ORDER DESCENDING)
        list(GET _qt6_candidates 0 _qt6_path)
        set(CMAKE_PREFIX_PATH "${_qt6_path}" CACHE PATH "Auto-detected Qt6 path" FORCE)
        message(STATUS "Qt6: auto-detected ${CMAKE_PREFIX_PATH}")
        unset(_qt6_path)
    else()
        message(WARNING "Qt6 not found automatically. Set QT_DIR env var or CMAKE_PREFIX_PATH.")
    endif()
    unset(_qt6_candidates)
endif()

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    Sql
    Concurrent
    Xml
    Svg
)
qt_standard_project_setup()
include(CTest)

# ── Source files ─────────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/SplashScreen.cpp
    src/MainWindow.cpp
    src/LibraryScanner.cpp
    src/BookModel.cpp
    src/BookDelegate.cpp
    src/MetadataFetcher.cpp
    src/ThumbnailCache.cpp
    src/BookDetailDialog.cpp
    src/SettingsDialog.cpp
    src/Database.cpp
    src/CoverDownloader.cpp
    src/LanguageManager.cpp
    src/ThemeManager.cpp
    src/PluginManager.cpp
    src/CommandPalette.cpp
)

set(HEADERS
    include/SplashScreen.h
    include/MainWindow.h
    include/LibraryScanner.h
    include/BookModel.h
    include/BookDelegate.h
    include/MetadataFetcher.h
    include/ThumbnailCache.h
    include/BookDetailDialog.h
    include/SettingsDialog.h
    include/Database.h
    include/CoverDownloader.h
    include/Book.h
    include/AppConfig.h
    include/LanguageManager.h
    include/ThemeManager.h
    include/PluginInterface.h
    include/PluginManager.h
    include/CommandPalette.h
)

# ── Resource file (icons, QSS, images) ───────────────────────────────────────
set(RESOURCES
    resources/resources.qrc
)

# ── Windows RC (version info + application icon) ─────────────────────────────
if(WIN32)
    list(APPEND SOURCES resources/eagle_library.rc)
endif()

qt_add_executable(EagleLibrary
    WIN32               # no console window
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

target_include_directories(EagleLibrary PRIVATE include)

if(WIN32)
    target_link_options(EagleLibrary PRIVATE /SUBSYSTEM:WINDOWS /MANIFEST:NO)
endif()

target_link_libraries(EagleLibrary PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    Qt6::Concurrent
    Qt6::Xml
    Qt6::Svg
)

# ── Link standard Windows libs ───────────────────────────────────────────────
if(WIN32)
    target_link_libraries(EagleLibrary PRIVATE
        shell32
        ole32
        user32
        gdi32
        comctl32
    )
endif()

# ── Definitions ──────────────────────────────────────────────────────────────
target_compile_definitions(EagleLibrary PRIVATE
    APP_VERSION="1.0.0"
    APP_COMPANY="Eagle Software"
    APP_NAME="Eagle Library"
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

# ── Deploy Qt DLLs automatically (windeployqt) ───────────────────────────────
if(WIN32)
    add_custom_command(TARGET EagleLibrary POST_BUILD
        COMMAND "${Qt6_DIR}/../../../bin/windeployqt.exe"
                --no-translations
                --compiler-runtime
                "$<TARGET_FILE:EagleLibrary>"
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:EagleLibrary>/resources"
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:EagleLibrary>/translations"
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:EagleLibrary>/themes"
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:EagleLibrary>/hooks"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/resources/eagle_logo.png"
                "$<TARGET_FILE_DIR:EagleLibrary>/resources/eagle_logo.png"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/resources/eagle_mark.svg"
                "$<TARGET_FILE_DIR:EagleLibrary>/resources/eagle_mark.svg"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/resources/eagle_256.png"
                "$<TARGET_FILE_DIR:EagleLibrary>/resources/eagle_256.png"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/resources/translations"
                "$<TARGET_FILE_DIR:EagleLibrary>/translations"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/resources/themes"
                "$<TARGET_FILE_DIR:EagleLibrary>/themes"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/resources/hooks"
                "$<TARGET_FILE_DIR:EagleLibrary>/hooks"
        COMMENT "Running windeployqt..."
    )
endif()

# ── Install rules (used by CPack / NSIS) ─────────────────────────────────────
install(TARGETS EagleLibrary
    RUNTIME DESTINATION "."
)
install(DIRECTORY "${CMAKE_BINARY_DIR}/platforms"    DESTINATION "." OPTIONAL)
install(DIRECTORY "${CMAKE_BINARY_DIR}/styles"       DESTINATION "." OPTIONAL)
install(DIRECTORY "${CMAKE_BINARY_DIR}/imageformats" DESTINATION "." OPTIONAL)
install(DIRECTORY "${CMAKE_BINARY_DIR}/translations" DESTINATION "." OPTIONAL)
install(DIRECTORY "${CMAKE_BINARY_DIR}/themes"       DESTINATION "." OPTIONAL)
install(DIRECTORY "${CMAKE_BINARY_DIR}/resources"    DESTINATION "." OPTIONAL)
install(DIRECTORY "${CMAKE_BINARY_DIR}/hooks"        DESTINATION "." OPTIONAL)
install(FILES     "${CMAKE_SOURCE_DIR}/installer/LICENSE.txt"
                  "${CMAKE_SOURCE_DIR}/installer/README.txt"
    DESTINATION "."
)

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)

    qt_add_executable(EagleLibraryRuntimeTests
        tests/RuntimeLayoutTests.cpp
        src/ThemeManager.cpp
        src/LanguageManager.cpp
        include/AppConfig.h
        include/ThemeManager.h
        include/LanguageManager.h
    )

    target_include_directories(EagleLibraryRuntimeTests PRIVATE include)
    target_link_libraries(EagleLibraryRuntimeTests PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Test
    )

    add_test(NAME EagleLibraryRuntimeTests COMMAND EagleLibraryRuntimeTests)
endif()

# ── CPack / NSIS installer ───────────────────────────────────────────────────
set(CPACK_GENERATOR "NSIS")
set(CPACK_PACKAGE_NAME "EagleLibrary")
set(CPACK_PACKAGE_VENDOR "Eagle Software")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Eagle Library - Professional eBook & Document Manager")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Eagle Library")
set(CPACK_NSIS_DISPLAY_NAME "Eagle Library")
set(CPACK_NSIS_PACKAGE_NAME "Eagle Library 1.0.0")
set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/resources/eagle.ico")
set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/resources/eagle.ico")
set(CPACK_NSIS_INSTALLED_ICON_NAME "EagleLibrary.exe")
set(CPACK_NSIS_URL_INFO_ABOUT "https://eaglesoftware.biz/")
set(CPACK_NSIS_HELP_LINK "https://eaglesoftware.biz/support")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_CREATE_ICONS_EXTRA
    "CreateShortcut '$SMPROGRAMS\\\\Eagle Library\\\\Eagle Library.lnk' '$INSTDIR\\\\EagleLibrary.exe'"
    "CreateShortcut '$DESKTOP\\\\Eagle Library.lnk' '$INSTDIR\\\\EagleLibrary.exe'"
)
set(CPACK_NSIS_DELETE_ICONS_EXTRA
    "Delete '$SMPROGRAMS\\\\Eagle Library\\\\Eagle Library.lnk'"
    "Delete '$DESKTOP\\\\Eagle Library.lnk'"
)
include(CPack)
