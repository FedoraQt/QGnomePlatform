
set(common_SRCS
    gnomehintssettings.cpp
    qgtk3dialoghelpers.cpp
    qxdgdesktopportalfiledialog.cpp
)

add_library(qgnomeplatform-common STATIC ${common_SRCS})
target_link_libraries(qgnomeplatform-common
    Qt::DBus
    Qt::Core
    Qt::GuiPrivate
    Qt::Widgets
    ${ADWAITAQT_LIBRARIES}
    PkgConfig::GTK+3
)
