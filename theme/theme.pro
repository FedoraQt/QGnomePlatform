lessThan(QT_MINOR_VERSION, 6): error("Qt 5.6 and newer is required.")

TEMPLATE = lib

QMAKE_LIBDIR += ../common
INCLUDEPATH += ../common

CONFIG += plugin \
          c++11 \
          link_pkgconfig

QT += core-private \
      dbus \
      gui-private \
      widgets

LIBS += -lcommon

equals(QT_MAJOR_VERSION, 5): greaterThan(QT_MINOR_VERSION, 7): QT += theme_support-private
equals(QT_MAJOR_VERSION, 5): lessThan(QT_MINOR_VERSION, 8): QT += platformsupport-private

PKGCONFIG += gtk+-3.0 \
             gtk+-x11-3.0

TARGET = qgnomeplatform
target.path += $$[QT_INSTALL_PLUGINS]/platformthemes
INSTALLS += target

SOURCES += platformplugin.cpp \
           qgnomeplatformtheme.cpp

HEADERS += platformplugin.h \
           qgnomeplatformtheme.h
