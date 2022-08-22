
set(common_SRCS
    gnomesettings.cpp
    gsettingshintprovider.cpp
    hintprovider.cpp
    portalhintprovider.cpp
    utils.cpp
)

add_library(qgnomeplatform${LIBQGNOMEPLATFORM_SUFFIX} SHARED ${common_SRCS})
target_link_libraries(qgnomeplatform${LIBQGNOMEPLATFORM_SUFFIX}
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::DBus
    Qt${QT_VERSION_MAJOR}::GuiPrivate
    Qt${QT_VERSION_MAJOR}::Widgets
    ${ADWAITAQT_LIBRARIES}
    PkgConfig::GTK4
)

install(TARGETS "qgnomeplatform${LIBQGNOMEPLATFORM_SUFFIX}" RUNTIME DESTINATION bin LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
