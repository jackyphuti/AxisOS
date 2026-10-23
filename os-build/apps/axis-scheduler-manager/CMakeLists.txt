cmake_minimum_required(VERSION 3.22)
project(AxisSchedulerManager VERSION 2.1.0 LANGUAGES CXX)

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

qt_add_executable(axis-scheduler-manager
    main.cpp
    SchedulerBackend.h
    SchedulerBackend.cpp
)

set_source_files_properties(Theme.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(axis-scheduler-manager
    URI AxisSchedulerManager
    VERSION 1.0
    QML_FILES
        Theme.qml
        SchedulerWindow.qml
)

target_link_libraries(axis-scheduler-manager
    PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
)

install(TARGETS axis-scheduler-manager DESTINATION /usr/local/bin)
