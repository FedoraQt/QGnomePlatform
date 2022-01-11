project(QGnomePlatform)

cmake_minimum_required(VERSION 3.0)

set(QGNOMEPLATFORM_VERSION "0.8.4")

option(USE_QT6 "Use Qt6 instead of Qt5" OFF)

if (USE_QT6)
    set(QT_MIN_VERSION "6.2.0")
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(LIBQGNOMEPLATFORM_SUFFIX "6")
else()
    set(QT_MIN_VERSION "5.15.2")
    set(CMAKE_CXX_STANDARD 14)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(LIBQGNOMEPLATFORM_SUFFIX "")
endif()

set(CMAKE_AUTOMOC ON)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules" ${CMAKE_MODULE_PATH})

include(GNUInstallDirs)
include(FeatureSummary)

if (USE_QT6)
    find_package(QT NAMES Qt6 COMPONENTS Core DBus Gui Widgets REQUIRED)
else()
    find_package(QT NAMES Qt5 COMPONENTS Core DBus Gui Widgets REQUIRED)
endif()

find_package(Qt${QT_VERSION_MAJOR} ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    DBus
    Gui
    Widgets
)

find_package(Qt${QT_VERSION_MAJOR}Gui ${QT_MIN_VERSION} CONFIG REQUIRED Private)
if (NOT USE_QT6)
    find_package(Qt${QT_VERSION_MAJOR}ThemeSupport REQUIRED)
endif()

find_package(GSettingSchemas REQUIRED)
if (USE_QT6)
    find_package(AdwaitaQt6 "1.4.1" REQUIRED)
else()
    find_package(AdwaitaQt "1.4.1" REQUIRED)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK+3 REQUIRED IMPORTED_TARGET gtk+-3.0)

# NOTE: there is no reason to disable any of the following options, but
# it is useful when building Flatpak extensions
if (DISABLE_DECORATION_SUPPORT)
    message(STATUS "Disabling Qt Wayland decoration support")
else()
    find_package(Qt${QT_VERSION_MAJOR}WaylandClient ${QT_MIN_VERSION} CONFIG REQUIRED)
    set_package_properties(Qt${QT_VERSION_MAJOR}WaylandClient PROPERTIES
        DESCRIPTION "Qt Wayland decoration support"
        PURPOSE "Required for QGnomePlatform decoration plugin"
        TYPE REQUIRED
    )
    # NOTE: I don't know how to do this only in case of qt_config(xkbcommon).
    # We would miss an include in QWaylandDisplay header file.
    if (NOT USE_QT6)
        find_package(Qt${QT_VERSION_MAJOR}XkbCommonSupport ${QT_MIN_VERSION})
    endif()

    if (DECORATION_SHADOWS_SUPPORT OR USE_QT6)
        message(STATUS "Enabling Qt Wayland decoration shadows support")
        message(STATUS "NOTE: This support requires changes in Qt Wayland from Qt 6.2")
        add_definitions(-DDECORATION_SHADOWS_SUPPORT)
    endif()
endif()

if (DISABLE_THEME_SUPPORT)
    message(STATUS "Disabling platform theme support")
endif()

if (NOT USE_QT6)
    get_target_property(REAL_QMAKE_EXECUTABLE ${Qt5Core_QMAKE_EXECUTABLE}
                        IMPORTED_LOCATION)

    if (NOT QT_PLUGINS_DIR)
        execute_process(COMMAND "${REAL_QMAKE_EXECUTABLE}" -query QT_INSTALL_PLUGINS
                        OUTPUT_VARIABLE QT_PLUGINS_DIR
                        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif()
endif()

add_subdirectory(src)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

