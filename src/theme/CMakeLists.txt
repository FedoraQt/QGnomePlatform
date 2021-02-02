
include_directories(
    ${CMAKE_SOURCE_DIR}/src/common
)

set(theme_SRCS
    platformplugin.cpp
    qgnomeplatformtheme.cpp
)

add_library(qgnomeplatform MODULE ${theme_SRCS})
target_link_libraries(qgnomeplatform
    qgnomeplatform-common
    Qt::Gui
    Qt::ThemeSupportPrivate
    ${ADWAITAQT_LIBRARIES}
)

install(TARGETS qgnomeplatform DESTINATION ${KDE_INSTALL_QTPLUGINDIR}/platformthemes)


