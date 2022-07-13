/*
 * Copyright (C) 2016-2022 Jan Grulich
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

#ifndef HINT_PROVIDER_H
#define HINT_PROVIDER_H

#include "gnomesettings.h"

#include <QObject>
#include <QHash>
#include <QVariant>

#include <qpa/qplatformtheme.h>

class QFont;
class QString;

class HintProvider : public QObject
{
    Q_OBJECT
public:
    explicit HintProvider(QObject *parent = nullptr);
    virtual ~HintProvider();

    inline QHash<QPlatformTheme::ThemeHint, QVariant> hints() const { return m_hints; }
    inline QHash<QPlatformTheme::Font, QFont *> fonts() const { return m_fonts; }

    // Theme
    inline QString gtkTheme() const { return m_gtkTheme; }
    inline GnomeSettings::Appearance appearance() const { return m_appearance; }

    // Cursor
    inline int cursorSize() const { return m_cursorSize; }
    inline QString cursorTheme() const { return m_cursorTheme; }

    // Window decorations
    inline GnomeSettings::TitlebarButtons titlebarButtons() const { return m_titlebarButtons; }
    inline GnomeSettings::TitlebarButtonsPlacement titlebarButtonPlacement() const { return m_titlebarButtonPlacement; }

Q_SIGNALS:
    void cursorBlinkTimeChanged();
    void cursorSizeChanged();
    void cursorThemeChanged();
    void fontChanged();
    void iconThemeChanged();
    void titlebarChanged();
    void themeChanged();

protected:
    // Theme
    QString m_gtkTheme;
    GnomeSettings::Appearance m_appearance = GnomeSettings::PreferLight;

    // Cursor
    int m_cursorSize = 0;
    QString m_cursorTheme;

    // Window decorations
    GnomeSettings::TitlebarButtons m_titlebarButtons = GnomeSettings::TitlebarButton::CloseButton;
    GnomeSettings::TitlebarButtonsPlacement m_titlebarButtonPlacement = GnomeSettings::TitlebarButtonsPlacement::RightPlacement;

    QHash<QPlatformTheme::Font, QFont *> m_fonts;
    QHash<QPlatformTheme::ThemeHint, QVariant> m_hints;
};

#endif // GNOME_SETTINGS_P_H

