cmake_minimum_required(VERSION 3.20)

project(EagleLibrary
    VERSION 1.0.0
    DESCRIPTION "Eagle Software - Professional eBook Library Manager"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Windows subsystem (no console) ──────────────────────────────────────────
if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /MANIFEST:NO")
endif()

# ── Find Qt6 (bundled with MSVC 2022 toolchain) ─────────────────────────────
# Qt6 path — adjust if needed, e.g. set QT_DIR in cmake-gui
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    Sql
    Concurrent
    Xml
)
qt_standard_project_setup()

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

target_link_libraries(EagleLibrary PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    Qt6::Concurrent
    Qt6::Xml
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
install(FILES     "${CMAKE_SOURCE_DIR}/installer/LICENSE.txt"
                  "${CMAKE_SOURCE_DIR}/installer/README.txt"
    DESTINATION "."
)

# ── CPack / NSIS installer ───────────────────────────────────────────────────
include(CPack)
set(CPACK_GENERATOR "NSIS")
set(CPACK_PACKAGE_NAME "EagleLibrary")
set(CPACK_PACKAGE_VENDOR "Eagle Software")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Eagle Library - Professional eBook Manager")
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
