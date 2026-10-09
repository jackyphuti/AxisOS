cmake_minimum_required(VERSION 3.16)
project(LiquidNodesOS VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(QT_SKIP_AUTO_PLUGIN_INCLUSION ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick WaylandCompositor)

qt_add_executable(LiquidNodesOS main.cpp)

# Registers the QML module and bakes Main.qml into the binary
qt_add_qml_module(LiquidNodesOS
    URI LiquidOS
    VERSION 1.0
    QML_FILES Main.qml
)

target_link_libraries(LiquidNodesOS
    PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::WaylandCompositor
)

install(TARGETS LiquidNodesOS DESTINATION /usr/local/bin)
