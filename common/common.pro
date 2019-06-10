
TEMPLATE = lib

CONFIG += c++11 \
          link_pkgconfig \
          staticlib

QT += core \
      dbus \
      theme_support-private \
      widgets

PKGCONFIG += gtk+-3.0 \
             gtk+-x11-3.0

SOURCES += gnomehintssettings.cpp \
           qgtk3dialoghelpers.cpp

HEADERS += gnomehintssettings.h \
           qgtk3dialoghelpers.h
