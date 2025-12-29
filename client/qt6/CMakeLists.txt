cmake_minimum_required(VERSION 3.16)
project(MillionaireClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Disable Qt6 deployment tools that may cause appman-controller issues
set(QT_NO_APP_MAN_CONTROLLER TRUE CACHE BOOL "Disable appman-controller" FORCE)
set(QT_NO_DEPLOY_TOOL TRUE CACHE BOOL "Disable Qt deployment tool" FORCE)

# Enable automatic MOC, UIC, RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Disable Qt6 deployment
set(QT_DEPLOY_TOOL_EXECUTABLE "" CACHE STRING "Qt deployment tool" FORCE)

find_package(Qt6 REQUIRED COMPONENTS Core Network Quick Widgets)

# Main sources - QML version
set(SOURCES
    main.cpp
    NetworkClient.cpp
    FileDialogHelper.cpp
)

set(HEADERS
    NetworkClient.h
    FileDialogHelper.h
)

# QML Resource file
set(RESOURCE_FILES
    qml.qrc
)

# Use add_executable instead of qt6_add_executable to avoid conflicts
add_executable(MillionaireClient ${SOURCES} ${HEADERS} ${RESOURCE_FILES})

# Link Qt6 libraries
target_link_libraries(MillionaireClient
    Qt6::Core
    Qt6::Network
    Qt6::Quick
    Qt6::Widgets
)

# Set QML import path
set_target_properties(MillionaireClient PROPERTIES
    QT_QML_MODULES_VERSION "6.0"
)

# Add QML import path for runtime
if(WIN32)
    set(QML_IMPORT_PATH "${Qt6_DIR}/../../../qml")
    set_target_properties(MillionaireClient PROPERTIES
        QT_QML_IMPORT_PATH "${QML_IMPORT_PATH}"
    )
endif()

