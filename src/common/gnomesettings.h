/*
 * Copyright (C) 2016-2021 Jan Grulich
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

#ifndef GNOME_SETTINGS_H
#define GNOME_SETTINGS_H

#include <QFlags>
#include <QObject>

#include <qpa/qplatformtheme.h>

class QFont;
class QString;
class QVariant;
class QPalette;

class GnomeSettings : public QObject
{
    Q_OBJECT
public:
    enum TitlebarButtonsPlacement {
        LeftPlacement = 0,
        RightPlacement = 1
    };

    enum TitlebarButton {
        CloseButton = 0x1,
        MinimizeButton = 0x02,
        MaximizeButton = 0x04
    };
    Q_DECLARE_FLAGS(TitlebarButtons, TitlebarButton);

    explicit GnomeSettings(QObject *parent = nullptr);
    virtual ~GnomeSettings() = default;

    static QFont * font(QPlatformTheme::Font type);
    static QPalette * palette();
    static QVariant hint(QPlatformTheme::ThemeHint hint);
    static bool canUseFileChooserPortal();
    static bool isGtkThemeDarkVariant();
    static bool isGtkThemeHighContrastVariant();
    static QString gtkTheme();
    static TitlebarButtons titlebarButtons();
    static TitlebarButtonsPlacement titlebarButtonPlacement();
};


Q_DECLARE_OPERATORS_FOR_FLAGS(GnomeSettings::TitlebarButtons)

#endif // GNOME_SETTINGS_H
