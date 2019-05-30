
TEMPLATE = lib

CONFIG += c++11 \
          link_pkgconfig \
          staticlib

QT += core \
      dbus \
      widgets

PKGCONFIG += gtk+-3.0 \
             gtk+-x11-3.0

equals(QT_MAJOR_VERSION, 5): greaterThan(QT_MINOR_VERSION, 7): QT += theme_support-private
equals(QT_MAJOR_VERSION, 5): lessThan(QT_MINOR_VERSION, 8): QT += platformsupport-private

SOURCES += gnomehintssettings.cpp \
           qgtk3dialoghelpers.cpp

HEADERS += gnomehintssettings.h \
           qgtk3dialoghelpers.h
