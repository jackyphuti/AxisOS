cmake_minimum_required(VERSION 3.22)
project(AxisTerminal VERSION 2.0.0 LANGUAGES CXX)

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
    Qml
)

find_library(VTERM_LIBRARY NAMES vterm REQUIRED)
find_library(UTIL_LIBRARY NAMES util REQUIRED)

qt_add_executable(axis-terminal
    main.cpp
    TerminalBackend.h
    TerminalBackend.cpp
    TerminalScreenItem.h
    TerminalScreenItem.cpp
)

qt_add_qml_module(axis-terminal
    URI AxisTerminal
    VERSION 1.0
    QML_FILES
        TerminalWindow.qml
)

target_link_libraries(axis-terminal
    PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    ${VTERM_LIBRARY}
    ${UTIL_LIBRARY}
)

install(TARGETS axis-terminal DESTINATION /usr/local/bin)
