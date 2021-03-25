project(QGnomePlatform)

cmake_minimum_required(VERSION 3.0)

set(QGNOMEPLATFORM_VERSION "0.7.50")

set(QT_MIN_VERSION "5.12.0")
set(CMAKE_AUTOMOC ON)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules" ${CMAKE_MODULE_PATH})

include(GNUInstallDirs)
include(FeatureSummary)

add_definitions(-std=c++11)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED Core DBus Gui Widgets)
find_package(Qt5Gui ${QT_MIN_VERSION} CONFIG REQUIRED Private)

find_package(Qt5ThemeSupport REQUIRED)

find_package(GSettingSchemas REQUIRED)
find_package(AdwaitaQt "1.2.50" REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK+3 REQUIRED IMPORTED_TARGET gtk+-3.0)

# NOTE: there is no reason to disable any of the following options, but
# it is useful when building Flatpak extensions
if (DISABLE_DECORATION_SUPPORT)
    message(STATUS "Disabling Qt Wayland decoration support")
else()
    find_package(Qt5WaylandClient ${QT_MIN_VERSION} CONFIG REQUIRED)
    set_package_properties(Qt5WaylandClient PROPERTIES
        DESCRIPTION "Qt Wayland decoration support"
        PURPOSE "Required for QGnomePlatform decoration plugin"
        TYPE REQUIRED
    )
    # NOTE: I don't know how to do this only in case of qt_config(xkbcommon).
    # We would miss an include in QWaylandDisplay header file.
    find_package(Qt5XkbCommonSupport ${QT_MIN_VERSION})

    if (DECORATION_SHADOWS_SUPPORT)
        message(STATUS "Enabling Qt Wayland decoration shadows support")
        message(STATUS "NOTE: This support requires changes in Qt Wayland from Qt 6.2")
        add_definitions(-DDECORATION_SHADOWS_SUPPORT)
    endif()
endif()

if (DISABLE_THEME_SUPPORT)
    message(STATUS "Disabling platform theme support")
endif()

get_target_property(REAL_QMAKE_EXECUTABLE ${Qt5Core_QMAKE_EXECUTABLE}
                    IMPORTED_LOCATION)

if (NOT QT_PLUGINS_DIR)
    execute_process(COMMAND "${REAL_QMAKE_EXECUTABLE}" -query QT_INSTALL_PLUGINS
                    OUTPUT_VARIABLE QT_PLUGINS_DIR
                    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

add_subdirectory(src)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

