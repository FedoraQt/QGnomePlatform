/*
 * Copyright (C) 2015 Martin Bříza <mbriza@redhat.com>
 * Copyright (C) 2017-2021 Jan Grulich <jgrulich@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "qgnomeplatformtheme.h"
#include "gnomesettings.h"
#include "qgtk3dialoghelpers.h"
#include "qxdgdesktopportalfiledialog_p.h"

#include <QApplication>
#include <QGuiApplication>
#include <QStyleFactory>

#undef signals
#include <gtk-3.0/gtk/gtk.h>
#define signals Q_SIGNALS

QGnomePlatformTheme::QGnomePlatformTheme()
{
    if (QGuiApplication::platformName() != QStringLiteral("xcb")) {
        if (!qEnvironmentVariableIsSet("QT_WAYLAND_DECORATION")) {
            qputenv("QT_WAYLAND_DECORATION", "gnome");
        }
    }

    /* Initialize some types here so that Gtk+ does not crash when reading
     * the treemodel for GtkFontChooser.
     */
    g_type_ensure(PANGO_TYPE_FONT_FAMILY);
    g_type_ensure(PANGO_TYPE_FONT_FACE);
}

QGnomePlatformTheme::~QGnomePlatformTheme()
{
}

QVariant QGnomePlatformTheme::themeHint(QPlatformTheme::ThemeHint hintType) const
{
    QVariant hint = GnomeSettings::hint(hintType);
    if (hint.isValid()) {
        return hint;
    } else {
        return QPlatformTheme::themeHint(hintType);
    }
}

const QFont *QGnomePlatformTheme::font(Font type) const
{
    return GnomeSettings::font(type);
}

const QPalette *QGnomePlatformTheme::palette(Palette type) const
{
    Q_UNUSED(type)

    return GnomeSettings::palette();
}

bool QGnomePlatformTheme::usePlatformNativeDialog(QPlatformTheme::DialogType type) const
{
    switch (type) {
    case QPlatformTheme::FileDialog:
        return true;
    case QPlatformTheme::FontDialog:
        return true;
    case QPlatformTheme::ColorDialog:
        return true;
    default:
        return false;
    }
}

QPlatformDialogHelper *QGnomePlatformTheme::createPlatformDialogHelper(QPlatformTheme::DialogType type) const
{
    switch (type) {
    case QPlatformTheme::FileDialog: {
        if (GnomeSettings::canUseFileChooserPortal()) {
            return new QXdgDesktopPortalFileDialog;
        } else {
            return new QGtk3FileDialogHelper;
        }
    }
    case QPlatformTheme::FontDialog:
        return new QGtk3FontDialogHelper();
    case QPlatformTheme::ColorDialog:
        return new QGtk3ColorDialogHelper();
    default:
        return nullptr;
    }
}

#ifndef QT_NO_SYSTEMTRAYICON
QPlatformSystemTrayIcon* QGnomePlatformTheme::createPlatformSystemTrayIcon() const
{
    return Q_NULLPTR;
}
#endif
