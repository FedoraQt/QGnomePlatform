lessThan(QT_MINOR_VERSION, 9): error("Qt 5.9 and newer is required.")

TEMPLATE = lib

QMAKE_LIBDIR += ../common
INCLUDEPATH += ../common

CONFIG += plugin \
          c++11 \
          link_pkgconfig

QT += core \
      gui \
      waylandclient-private \
      widgets

LIBS += -lcommon

QMAKE_USE += wayland-client

PKGCONFIG += adwaita-qt gtk+-3.0

TARGET = qgnomeplatformdecoration
target.path += $$[QT_INSTALL_PLUGINS]/wayland-decoration-client
INSTALLS += target

SOURCES += decorationplugin.cpp \
           qgnomeplatformdecoration.cpp

HEADERS += decorationplugin.h \
           qgnomeplatformdecoration.h

