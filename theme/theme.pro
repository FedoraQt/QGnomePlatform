lessThan(QT_MINOR_VERSION, 9): error("Qt 5.9 and newer is required.")

TEMPLATE = lib

QMAKE_LIBDIR += ../common
INCLUDEPATH += ../common

CONFIG += plugin \
          c++11 \
          link_pkgconfig

QT += core-private \
      dbus \
      gui-private \
      theme_support-private \
      widgets

LIBS += -lcommon

PKGCONFIG += gtk+-3.0 \
             gtk+-x11-3.0

TARGET = qgnomeplatform
target.path += $$[QT_INSTALL_PLUGINS]/platformthemes
INSTALLS += target

SOURCES += platformplugin.cpp \
           qgnomeplatformtheme.cpp

HEADERS += platformplugin.h \
           qgnomeplatformtheme.h
