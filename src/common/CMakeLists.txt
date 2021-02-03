
set(common_SRCS
    gnomehintssettings.cpp
)

add_library(qgnomeplatform SHARED ${common_SRCS})
target_link_libraries(qgnomeplatform
    Qt::Core
    Qt::DBus
    Qt::GuiPrivate
    Qt::Widgets
    ${ADWAITAQT_LIBRARIES}
    PkgConfig::GTK+3
)

install(TARGETS qgnomeplatform RUNTIME DESTINATION bin LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
