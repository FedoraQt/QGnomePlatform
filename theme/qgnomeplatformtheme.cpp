/*
 * Copyright (C) 2015 Martin Bříza <mbriza@redhat.com>
 * Copyright (C) 2017-2019 Jan Grulich <jgrulich@redhat.com>
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
#include "gnomehintssettings.h"
#include "qgtk3dialoghelpers.h"

#include <QApplication>
#include <QStyleFactory>
#include <QX11Info>

#if !defined(QT_NO_DBUS) && !defined(QT_NO_SYSTEMTRAYICON)
#include <private/qdbustrayicon_p.h>
#endif

QGnomePlatformTheme::QGnomePlatformTheme()
{
    if (!QX11Info::isPlatformX11()) {
        if (!qEnvironmentVariableIsSet("QT_WAYLAND_DECORATION"))
            qputenv("QT_WAYLAND_DECORATION", "gnome");
    }

    loadSettings();

    /* Initialize some types here so that Gtk+ does not crash when reading
     * the treemodel for GtkFontChooser.
     */
    g_type_ensure(PANGO_TYPE_FONT_FAMILY);
    g_type_ensure(PANGO_TYPE_FONT_FACE);
}

QGnomePlatformTheme::~QGnomePlatformTheme()
{
    delete m_hints;
}

QVariant QGnomePlatformTheme::themeHint(QPlatformTheme::ThemeHint hintType) const
{
    QVariant hint = m_hints->hint(hintType);
    if (hint.isValid()) {
        return hint;
    } else {
        return QPlatformTheme::themeHint(hintType);
    }
}

const QFont *QGnomePlatformTheme::font(Font type) const
{
    return m_hints->font(type);
}

const QPalette *QGnomePlatformTheme::palette(Palette type) const
{
    Q_UNUSED(type);

    return m_hints->palette();
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
    case QPlatformTheme::MessageDialog:
    default:
        return false;
    }
}

QPlatformDialogHelper *QGnomePlatformTheme::createPlatformDialogHelper(QPlatformTheme::DialogType type) const
{
    switch (type) {
    case QPlatformTheme::FileDialog:
        return new QGtk3FileDialogHelper();
    case QPlatformTheme::FontDialog:
        return new QGtk3FontDialogHelper();
    case QPlatformTheme::ColorDialog:
        return new QGtk3ColorDialogHelper();
    case QPlatformTheme::MessageDialog:
    default:
        return 0;
    }
}

#if !defined(QT_NO_DBUS) && !defined(QT_NO_SYSTEMTRAYICON)
static bool isDBusTrayAvailable() {
    static bool dbusTrayAvailable = false;
    static bool dbusTrayAvailableKnown = false;
    if (!dbusTrayAvailableKnown) {
        QDBusMenuConnection conn;
        if (conn.isStatusNotifierHostRegistered())
            dbusTrayAvailable = true;
        dbusTrayAvailableKnown = true;
    }
    return dbusTrayAvailable;
}
#endif

#if !defined(QT_NO_DBUS) && !defined(QT_NO_SYSTEMTRAYICON)
QPlatformSystemTrayIcon * QGnomePlatformTheme::createPlatformSystemTrayIcon() const
{
    if (isDBusTrayAvailable())
        return new QDBusTrayIcon();
    return Q_NULLPTR;
}
#endif

void QGnomePlatformTheme::loadSettings()
{
    m_hints = new GnomeHintsSettings;
}
