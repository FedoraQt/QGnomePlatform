TEMPLATE = lib

CONFIG += plugin \
          c++11 \
          link_pkgconfig

QT += core-private \
      gui-private \
      platformsupport-private \
      widgets

PKGCONFIG += gtk+-3.0 \
             gtk+-x11-3.0

TARGET = qgnomeplatform
target.path += $$[QT_INSTALL_PLUGINS]/platformthemes
INSTALLS += target

SOURCES += src/platformplugin.cpp \
           src/qgnomeplatformtheme.cpp \
           src/gnomehintssettings.cpp \
           src/qgtk3dialoghelpers.cpp \
           src/gnomeproxystyle.cpp

HEADERS += src/platformplugin.h \
           src/qgnomeplatformtheme.h \
           src/gnomehintssettings.h \
           src/qgtk3dialoghelpers.h \
           src/gnomeproxystyle.h
