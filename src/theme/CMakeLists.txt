
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
    qgnomeplatform
    Qt::DBus
    Qt::Gui
    Qt::ThemeSupportPrivate
    ${ADWAITAQT_LIBRARIES}
    PkgConfig::GTK+3
)

install(TARGETS qgnomeplatformtheme RENAME qgnomeplatform DESTINATION ${QT_PLUGINS_DIR}/platformthemes)


