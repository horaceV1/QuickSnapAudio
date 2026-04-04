cmake_minimum_required(VERSION 3.16)
project(QuickSnapAudio VERSION 1.0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui Multimedia Network)

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/audiodevicemanager.cpp
    src/audiodevicemanager.h
    src/hotkeymanager.cpp
    src/hotkeymanager.h
    src/configmanager.cpp
    src/configmanager.h
    src/trayicon.cpp
    src/trayicon.h
    src/updatechecker.cpp
    src/updatechecker.h
    src/deviceentry.cpp
    src/deviceentry.h
    resources/resources.qrc
)

if(WIN32)
    list(APPEND SOURCES src/platform/windows_audio.cpp src/platform/windows_audio.h)
    list(APPEND SOURCES src/platform/windows_hotkey.cpp src/platform/windows_hotkey.h)
    list(APPEND SOURCES resources/app.rc)
elseif(UNIX AND NOT APPLE)
    list(APPEND SOURCES src/platform/linux_audio.cpp src/platform/linux_audio.h)
    list(APPEND SOURCES src/platform/linux_hotkey.cpp src/platform/linux_hotkey.h)
endif()

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE src)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
    Qt6::Multimedia
    Qt6::Network
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ole32 oleaut32 mmdevapi)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
elseif(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PULSE REQUIRED libpulse)
    pkg_check_modules(X11 REQUIRED x11)
    pkg_check_modules(XTST REQUIRED xtst)
    target_include_directories(${PROJECT_NAME} PRIVATE ${PULSE_INCLUDE_DIRS} ${X11_INCLUDE_DIRS} ${XTST_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${PULSE_LIBRARIES} ${X11_LIBRARIES} ${XTST_LIBRARIES})
endif()

# Installation
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

if(UNIX AND NOT APPLE)
    install(FILES resources/quicksnapaudio.desktop DESTINATION share/applications)
    install(FILES resources/icon.png DESTINATION share/icons/hicolor/256x256/apps RENAME quicksnapaudio.png)
endif()

# CPack for .deb
if(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Daniel Filipe Leonardo Pessoa")
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Quick audio device switcher with hotkeys")
    set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libpulse0, libqt6widgets6, libqt6core6, libqt6gui6, libqt6network6, libx11-6, libxtst6")
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_PACKAGE_NAME "quicksnapaudio")
    include(CPack)
endif()
