cmake_minimum_required(VERSION 3.22)
project(AxisWelcome VERSION 2.1.0 LANGUAGES CXX)

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

qt_add_executable(axis-welcome
    main.cpp
    WelcomeBackend.h
    WelcomeBackend.cpp
)

set_source_files_properties(Theme.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(axis-welcome
    URI AxisWelcome
    VERSION 1.0
    QML_FILES
        Theme.qml
        WelcomeWindow.qml
)

target_link_libraries(axis-welcome
    PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
)

install(TARGETS axis-welcome DESTINATION /usr/local/bin)
