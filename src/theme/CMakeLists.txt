
include_directories(
    ${CMAKE_SOURCE_DIR}/src/common
)

set(theme_SRCS
    platformplugin.cpp
    qgnomeplatformtheme.cpp
    qgtk3dialoghelpers.cpp
    qxdgdesktopportalfiledialog.cpp
)

add_library(qgnomeplatformtheme MODULE ${theme_SRCS})
target_link_libraries(qgnomeplatformtheme
    qgnomeplatform${LIBQGNOMEPLATFORM_SUFFIX}
    Qt${QT_VERSION_MAJOR}::DBus
    Qt${QT_VERSION_MAJOR}::Gui
    ${ADWAITAQT_LIBRARIES}
    PkgConfig::GTK+3
)

if (NOT USE_QT6)
    target_link_libraries(qgnomeplatformtheme Qt5::ThemeSupportPrivate)
endif()

install(TARGETS qgnomeplatformtheme DESTINATION ${QT_PLUGINS_DIR}/platformthemes)
