cmake_minimum_required(VERSION 3.19)

project(imtube-qt VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Network)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED IMPORTED_TARGET
    gstreamer-1.0
    gstreamer-app-1.0
    gstreamer-base-1.0
)

qt_add_executable(imtube-qt
    src/main.cpp
    src/core/Settings.cpp
    src/core/VideoItem.cpp
    src/core/VideoListModel.cpp
    src/core/VideoSinkFactory.cpp
    src/core/YtDlpHelper.cpp
    src/core/GStreamerPlayer.cpp
    src/core/LibraryStore.cpp
    src/core/ThumbnailService.cpp
)

qt_add_resources(imtube-qt "qml"
    PREFIX "/qml"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/src/qml"
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/qml/Main.qml"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/qml/Theme.qml"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/qml/SearchPage.qml"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/qml/PlayerPage.qml"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/qml/LibraryPage.qml"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/qml/SettingsPage.qml"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/qml/VideoCard.qml"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/qml/TransportBar.qml"
)

target_include_directories(imtube-qt PRIVATE src)

target_link_libraries(imtube-qt PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
    PkgConfig::GST
)

set_target_properties(imtube-qt PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

install(TARGETS imtube-qt
    RUNTIME DESTINATION bin
)