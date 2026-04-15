cmake_minimum_required(VERSION 3.16)
project(raid-manager VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core)

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/MdadmBackend.cpp
    src/MdadmBackend.h
    src/DiskVisualWidget.cpp
    src/DiskVisualWidget.h
    src/RaidTableWidget.cpp
    src/RaidTableWidget.h
    src/QueuePanel.cpp
    src/QueuePanel.h
    src/CreateRaidDialog.cpp
    src/CreateRaidDialog.h
    src/Models.h
)

qt_add_executable(raid-manager ${SOURCES})

target_link_libraries(raid-manager PRIVATE Qt6::Widgets Qt6::Core)

install(TARGETS raid-manager DESTINATION bin)
