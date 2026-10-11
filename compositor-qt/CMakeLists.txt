cmake_minimum_required(VERSION 3.22)
project(LiquidNodesOS VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(QT_SKIP_AUTO_PLUGIN_INCLUSION ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Quick
    WaylandCompositor
    DBus
)

find_library(PAM_LIBRARY NAMES pam REQUIRED)

qt_add_executable(LiquidNodesOS
    main.cpp
    systemcontroller.h
    systemcontroller.cpp
    authmanager.h
    authmanager.cpp
)

qt_add_qml_module(LiquidNodesOS
    URI LiquidNodes
    VERSION 1.0
    QML_FILES
        Main.qml
        GlassmorphicWindow.qml
        LiquidBackground.qml
        LiquidTopBar.qml
        LiquidDock.qml
        LoginScreen.qml
)

target_link_libraries(LiquidNodesOS
    PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::WaylandCompositor
    Qt6::DBus
    ${PAM_LIBRARY}
)

install(TARGETS LiquidNodesOS DESTINATION /usr/local/bin)
