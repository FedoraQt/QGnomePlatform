
include_directories(
    ${CMAKE_SOURCE_DIR}/src/common
)

set(decoration_SRCS
    decorationplugin.cpp
    qgnomeplatformdecoration.cpp
)

add_library(qgnomeplatformdecoration MODULE ${decoration_SRCS})
target_link_libraries(qgnomeplatformdecoration
    qgnomeplatform-common
    Qt::Gui
    Qt::GuiPrivate
    Qt::WaylandClientPrivate
    ${ADWAITAQT_LIBRARIES}
    PkgConfig::GTK+3
)

if (${Qt5XkbCommonSupport_FOUND})
target_link_libraries(qgnomeplatformdecoration
    Qt::XkbCommonSupportPrivate
)
endif()

install(TARGETS qgnomeplatformdecoration DESTINATION ${KDE_INSTALL_QTPLUGINDIR}/wayland-decoration-client)

