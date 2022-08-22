
include_directories(
    ${CMAKE_SOURCE_DIR}/src/common
)

set(decoration_SRCS
    decorationplugin.cpp
    qgnomeplatformdecoration.cpp
)

add_library(qgnomeplatformdecoration MODULE ${decoration_SRCS})
target_link_libraries(qgnomeplatformdecoration
    qgnomeplatform${LIBQGNOMEPLATFORM_SUFFIX}
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::GuiPrivate
    Qt${QT_VERSION_MAJOR}::WaylandClientPrivate
    ${ADWAITAQT_LIBRARIES}
    PkgConfig::GTK4
)

if (NOT USE_QT6)
    if (${Qt5XkbCommonSupport_FOUND})
    target_link_libraries(qgnomeplatformdecoration
        Qt${QT_VERSION_MAJOR}::XkbCommonSupportPrivate
    )
    endif()
endif()

install(TARGETS qgnomeplatformdecoration DESTINATION ${QT_PLUGINS_DIR}/wayland-decoration-client)
