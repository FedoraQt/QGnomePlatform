/*
 * Copyright (C) 2016 Jan Grulich
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

#include "gnomehintssettings.h"

#include <QDir>
#include <QDebug>
#include <QString>
#include <QPalette>
#include <QMainWindow>
#include <QApplication>
#include <QGuiApplication>
#include <QDialogButtonBox>

#include <gtk-3.0/gtk/gtksettings.h>

GnomeHintsSettings::GnomeHintsSettings()
    : QObject(0)
    , m_gtkThemeDarkVariant(false)
    , m_palette(nullptr)
    , m_settings(g_settings_new("org.gnome.desktop.interface"))
{
    gtk_init(nullptr, nullptr);

    // Get current theme and variant
    g_object_get(gtk_settings_get_default(), "gtk-theme-name", &m_gtkTheme, "gtk-application-prefer-dark-theme", &m_gtkThemeDarkVariant, NULL);

    if (!m_gtkTheme) {
        qWarning() << "Couldn't get current gtk theme!";
    } else {
        qDebug() << "Theme name: " << m_gtkTheme;
        qDebug() << "Dark version: " << (m_gtkThemeDarkVariant ? "yes" : "no");
    }

    gint cursorBlinkTime = g_settings_get_int(m_settings, "cursor-blink-time");
//     g_object_get(gtk_settings_get_default(), "gtk-cursor-blink-time", &cursorBlinkTime, NULL);
    if (cursorBlinkTime >= 100) {
        qDebug() << "Cursor blink time: " << cursorBlinkTime;
        m_hints[QPlatformTheme::CursorFlashTime] = cursorBlinkTime;
    } else {
        m_hints[QPlatformTheme::CursorFlashTime] = 1200;
    }

    gint doubleClickTime = 400;
    g_object_get(gtk_settings_get_default(), "gtk-double-click-time", &doubleClickTime, NULL);
    qDebug() << "Double click time: " << doubleClickTime;
    m_hints[QPlatformTheme::MouseDoubleClickInterval] = doubleClickTime;

    guint longPressTime = 500;
    g_object_get(gtk_settings_get_default(), "gtk-long-press-time", &longPressTime, NULL);
    qDebug() << "Long press time: " << longPressTime;
    m_hints[QPlatformTheme::MousePressAndHoldInterval] = longPressTime;

    gint doubleClickDistance = 5;
    g_object_get(gtk_settings_get_default(), "gtk-double-click-distance", &doubleClickDistance, NULL);
    qDebug() << "Double click distance: " << doubleClickDistance;
    m_hints[QPlatformTheme::MouseDoubleClickDistance] = doubleClickDistance;

    gint startDragDistance = 8;
    g_object_get(gtk_settings_get_default(), "gtk-dnd-drag-threshold", &startDragDistance, NULL);
    qDebug() << "Dnd drag threshold: " << startDragDistance;
    m_hints[QPlatformTheme::StartDragDistance] = startDragDistance;

    guint passwordMaskDelay = 0;
    g_object_get(gtk_settings_get_default(), "gtk-entry-password-hint-timeout", &passwordMaskDelay, NULL);
    qDebug() << "Password hint timeout: " << passwordMaskDelay;
    m_hints[QPlatformTheme::PasswordMaskDelay] = passwordMaskDelay;

    gchar *systemIconTheme = g_settings_get_string(m_settings, "icon-theme");
//     g_object_get(gtk_settings_get_default(), "gtk-icon-theme-name", &systemIconTheme, NULL);
    if (systemIconTheme) {
        qDebug() << "Icon theme: " << systemIconTheme;
        m_hints[QPlatformTheme::SystemIconThemeName] = systemIconTheme;
        free(systemIconTheme);
    } else {
        m_hints[QPlatformTheme::SystemIconThemeName] = "Adwaita";
    }
    m_hints[QPlatformTheme::SystemIconFallbackThemeName] = "Adwaita";
    m_hints[QPlatformTheme::IconThemeSearchPaths] = xdgIconThemePaths();

    QStringList styleNames;
    styleNames << QStringLiteral("adwaita")
               << QStringLiteral("gtk+")
               << QStringLiteral("fusion")
               << QStringLiteral("windows");
    m_hints[QPlatformTheme::StyleNames] = styleNames;

    m_hints[QPlatformTheme::DialogButtonBoxLayout] = QDialogButtonBox::GnomeLayout;
    m_hints[QPlatformTheme::DialogButtonBoxButtonsHaveIcons] = true;
    m_hints[QPlatformTheme::KeyboardScheme] = QPlatformTheme::GnomeKeyboardScheme;
    m_hints[QPlatformTheme::IconPixmapSizes] = QVariant::fromValue(QList<int>() << 512 << 256 << 128 << 64 << 32 << 22 << 16 << 8);
    m_hints[QPlatformTheme::PasswordMaskCharacter] = QVariant(QChar(0x2022));

                                        /* Other theme hints */
    // KeyboardInputInterval, StartDragTime, KeyboardAutoRepeatRate, StartDragVelocity, DropShadow,
    // MaximumScrollBarDragDistance, ItemViewActivateItemOnSingleClick, WindowAutoPlacement, DialogButtonBoxButtonsHaveIcons
    // UseFullScreenForPopupMenu, UiEffects, SpellCheckUnderlineStyle, TabFocusBehavior, TabAllWidgets, PasswordMaskCharacter
    // DialogSnapToDefaultButton, ContextMenuOnMouseRelease, WheelScrollLines
    //  TODO TextCursorWidth, ToolButtonStyle, ToolBarIconSize

    // Load fonts
    loadFonts();

    // Load palette
    loadPalette();
}

GnomeHintsSettings::~GnomeHintsSettings()
{
    qDeleteAll(m_fonts);
    delete m_palette;
}

void GnomeHintsSettings::loadFonts()
{
//     gdouble scaling = g_settings_get_double(m_settings, "text-scaling-factor");

    QStringList fontTypes { "font-name", "monospace-font-name" };

    Q_FOREACH (const QString fontType, fontTypes) {
        gchar *fontName = g_settings_get_string(m_settings, fontType.toStdString().c_str());
        if (!fontName) {
            qWarning() << "Couldn't get " << fontType;
        } else {
            QString fontNameString(fontName);
            QRegExp re("(.+)[ \t]+([0-9]+)");
            int fontSize;
            if (re.indexIn(fontNameString) == 0) {
                fontSize = re.cap(2).toInt();
                if (fontType == QLatin1String("font-name")) {
                    m_fonts[QPlatformTheme::SystemFont] = new QFont(re.cap(1), fontSize, QFont::Normal);
                    qDebug() << "Font name: " << re.cap(1) << " (size " << fontSize << ")";
                } else if (fontType == QLatin1String("monospace-font-name")) {
                    m_fonts[QPlatformTheme::FixedFont] = new QFont(re.cap(1), fontSize, QFont::Normal);
                    qDebug() << "Monospace font name: " << re.cap(1) << " (size " << fontSize << ")";
                }
            } else {
                if (fontType == QLatin1String("font-name")) {
                    m_fonts[QPlatformTheme::SystemFont] = new QFont(fontNameString);
                    qDebug() << "Font name: " << fontNameString;
                } else if (fontType == QLatin1String("monospace-font-name")) {
                    m_fonts[QPlatformTheme::FixedFont] = new QFont(fontNameString);
                    qDebug() << "Monospace font name: " << fontNameString;
                }
            }
            free(fontName);
        }
    }
}

void GnomeHintsSettings::loadPalette()
{
    if (m_palette) {
        delete m_palette;
        m_palette = nullptr;
    }

    m_palette = new QPalette();
//     GtkCssProvider *gtkCssProvider = gtk_css_provider_get_named(themeName, preferDark ? "dark" : NULL);
//
//     if (!gtkCssProvider) {
//         qDebug() << "Couldn't load current gtk css provider!";
//         return;
//     }

//     qDebug() << gtk_css_provider_to_string(gtkCssProvider);
}

QStringList GnomeHintsSettings::xdgIconThemePaths() const
{
    QStringList paths;

    const QFileInfo homeIconDir(QDir::homePath() + QStringLiteral("/.icons"));
    if (homeIconDir.isDir()) {
        paths << homeIconDir.absoluteFilePath();
    }

    QString xdgDirString = QFile::decodeName(qgetenv("XDG_DATA_DIRS"));

    if (xdgDirString.isEmpty()) {
        xdgDirString = QStringLiteral("/usr/local/share:/usr/share");
    }

    Q_FOREACH (const QString &xdgDir, xdgDirString.split(QLatin1Char(':'))) {
        const QFileInfo xdgIconsDir(xdgDir + QStringLiteral("/icons"));
        if (xdgIconsDir.isDir()) {
            paths << xdgIconsDir.absoluteFilePath();
        }
    }

    return paths;
}
