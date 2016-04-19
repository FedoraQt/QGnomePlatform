TEMPLATE = lib

CONFIG += plugin \
          c++11 \
          link_pkgconfig \
          no_keywords

QT += core-private \
      gui-private \
      platformsupport-private \
      widgets

PKGCONFIG += gio-2.0 \
             gtk+-3.0

TARGET = qgnomeplatform
target.path += $$[QT_INSTALL_PLUGINS]/platformthemes
INSTALLS += target

SOURCES += src/platformplugin.cpp \
           src/qgnomeplatformtheme.cpp \
           src/gnomehintssettings.cpp

HEADERS += src/platformplugin.h \
           src/qgnomeplatformtheme.h \
           src/gnomehintssettings.h
