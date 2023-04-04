/*
 * Copyright (C) 2017-2022 Jan Grulich <jgrulich@redhat.com>
 * Copyright (C) 2015 Martin Bříza <mbriza@redhat.com>
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
#include <QLoggingCategory>
#include <QQuickStyle>
#include <QStyleFactory>

#undef signals
#include <gtk-3.0/gtk/gtk.h>
#define signals Q_SIGNALS

#include <X11/Xlib.h>

#if QT_VERSION < 0x060000
#ifndef QT_NO_SYSTEMTRAYICON
#include <private/qdbustrayicon_p.h>
#endif
#endif

#if QT_VERSION > 0x060000
#include <QtGui/private/qgenericunixthemes_p.h>
#endif

Q_LOGGING_CATEGORY(QGnomePlatformThemeLog, "qt.qpa.qgnomeplatform.theme")

void gtkMessageHandler(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer unused_data)
{
    /* Silence false-positive Gtk warnings (we are using Xlib to set
     * the WM_TRANSIENT_FOR hint).
     */
    if (g_strcmp0(message,
                  "GtkDialog mapped without a transient parent. "
                  "This is discouraged.")
        != 0) {
        /* For other messages, call the default handler. */
        g_log_default_handler(log_domain, log_level, message, unused_data);
    }
}

QGnomePlatformTheme::QGnomePlatformTheme()
{
    if (QGuiApplication::platformName() != QStringLiteral("xcb")) {
        if (!qEnvironmentVariableIsSet("QT_WAYLAND_DECORATION")) {
            qputenv("QT_WAYLAND_DECORATION", "gnome");
        }
    }

    // Ensure gtk uses the same windowing system, but let it
    // fallback in case GDK_BACKEND environment variable
    // filters the preferred one out
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"))) {
        gdk_set_allowed_backends("wayland,x11");
    } else if (QGuiApplication::platformName() == QLatin1String("xcb")) {
        gdk_set_allowed_backends("x11,wayland");
    }

    // Set log handler to suppress false GtkDialog warnings
    g_log_set_handler("Gtk", G_LOG_LEVEL_MESSAGE, gtkMessageHandler, nullptr);

    /* Initialize some types here so that Gtk+ does not crash when reading
     * the treemodel for GtkFontChooser.
     */
    g_type_ensure(PANGO_TYPE_FONT_FAMILY);
    g_type_ensure(PANGO_TYPE_FONT_FACE);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Load QGnomeTheme
    m_platformTheme = QGenericUnixTheme::createUnixTheme(QLatin1String("gnome"));
#endif

    // Configure the Qt Quick Controls 2 style to the KDE desktop style,
    // Which passes the QtWidgets theme through to Qt Quick Controls.
    // From https://invent.kde.org/plasma/plasma-integration/-/blob/02fe12a55522a43de3efa6de2185a695ff2a576a/src/platformtheme/kdeplatformtheme.cpp#L582

    // if the user has explicitly set something else, don't meddle
    // Also ignore the default Fusion style
    if (!QQuickStyle::name().isEmpty() && QQuickStyle::name() != QLatin1String("Fusion")) {
        return;
    }

    // Unfortunately we only have a way to check this on Qt5
    // On Qt6 this should just fall back to the Fusion style automatically.
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (!QQuickStyle::availableStyles().contains(QStringLiteral("org.kde.desktop"))) {
        qCWarning(QGnomePlatformThemeLog) << "The desktop style for QtQuick Controls 2 applications"
                                          << "is not available on the system (qqc2-desktop-style)."
                                          << "The application may look broken.";
        return;
    }
#endif

    QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
}

QGnomePlatformTheme::~QGnomePlatformTheme()
{
}

QVariant QGnomePlatformTheme::themeHint(QPlatformTheme::ThemeHint hintType) const
{
    QVariant hint = GnomeSettings::getInstance().hint(hintType);
    if (hint.isValid()) {
        return hint;
    } else {
        return QPlatformTheme::themeHint(hintType);
    }
}

const QFont *QGnomePlatformTheme::font(Font type) const
{
    return GnomeSettings::getInstance().font(type);
}

const QPalette *QGnomePlatformTheme::palette(Palette type) const
{
    Q_UNUSED(type)

    return GnomeSettings::getInstance().palette();
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
        if (GnomeSettings::getInstance().canUseFileChooserPortal()) {
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

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#ifndef QT_NO_SYSTEMTRAYICON
static bool isDBusTrayAvailable()
{
    static bool dbusTrayAvailable = false;
    static bool dbusTrayAvailableKnown = false;
    if (!dbusTrayAvailableKnown) {
        QDBusMenuConnection conn;
        if (conn.isStatusNotifierHostRegistered()) {
            dbusTrayAvailable = true;
        }
        dbusTrayAvailableKnown = true;
    }
    return dbusTrayAvailable;
}
#endif
#endif

#ifndef QT_NO_SYSTEMTRAYICON
QPlatformSystemTrayIcon *QGnomePlatformTheme::createPlatformSystemTrayIcon() const
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (isDBusTrayAvailable()) {
        return new QDBusTrayIcon();
    }
#else
    if (m_platformTheme) {
        return m_platformTheme->createPlatformSystemTrayIcon();
    }
#endif
    return Q_NULLPTR;
}
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
Qt::ColorScheme QGnomePlatformTheme::colorScheme() const
{
    return GnomeSettings::getInstance().useGtkThemeDarkVariant() ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light;
}
#elif QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
QPlatformTheme::Appearance QGnomePlatformTheme::appearance() const
{
    return GnomeSettings::getInstance().useGtkThemeDarkVariant() ? Appearance::Dark : Appearance::Light;
}
#endif
