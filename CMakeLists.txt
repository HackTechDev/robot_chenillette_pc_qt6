cmake_minimum_required(VERSION 3.19)
project(batmobile VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets DBus)
qt_standard_project_setup()

# BlueZ via D-Bus : liste des appareils appairés + appairage automatique (partagé GUI / CLI)
qt_add_library(batmobile_bluez STATIC
    src/config.h
    src/bluez.h src/bluez.cpp
    src/pairing.h src/pairing.cpp
)
target_link_libraries(batmobile_bluez PUBLIC Qt6::Core Qt6::DBus)

# Télécommande (GUI)
qt_add_executable(batmobile
    src/main.cpp
    src/window.h src/window.cpp
    src/link.h src/link.cpp
)
target_link_libraries(batmobile PRIVATE batmobile_bluez Qt6::Widgets)

# Appairage en ligne de commande (équivalent de bt_pairing.py)
qt_add_executable(bt_pairing src/bt_pairing.cpp)
target_link_libraries(bt_pairing PRIVATE batmobile_bluez)
