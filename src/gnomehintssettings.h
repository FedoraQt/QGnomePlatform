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

#ifndef GNOME_HINTS_SETTINGS_H
#define GNOME_HINTS_SETTINGS_H

#include <QFont>
#include <QObject>
#include <QVariant>

#undef signals
#include <gio/gio.h>
#include <qpa/qplatformtheme.h>

#include <gtk-3.0/gtk/gtk.h>

class QPalette;

class GnomeHintsSettings : public QObject
{
    Q_OBJECT
public:
    explicit GnomeHintsSettings();
    virtual ~GnomeHintsSettings();

    inline QFont * font(QPlatformTheme::Font type) const
    {
        if (m_fonts.contains(type)) {
            return m_fonts[type];
        } else if (m_fonts.contains(QPlatformTheme::SystemFont)) {
            return m_fonts[QPlatformTheme::SystemFont];
        } else {
            // GTK default font
            return new QFont(QLatin1String("Sans"), 10);
        }
    }

    inline bool gtkThemeDarkVariant() const
    {
        return m_gtkThemeDarkVariant;
    }

    inline QString gtkTheme() const
    {
        return QString(m_gtkTheme);
    }

    inline QVariant hint(QPlatformTheme::ThemeHint hint) const
    {
        return m_hints[hint];
    }

    inline QPalette *palette() const
    {
        return m_palette;
    }

public Q_SLOTS:
    void cursorBlinkTimeChanged();
    void fontChanged();
    void iconsChanged();
    void themeChanged();

private Q_SLOTS:
    void loadTheme();
    void loadFonts();
    void loadPalette();
    void loadStaticHints();

protected:
    static void gsettingPropertyChanged(GSettings *settings, gchar *key, GnomeHintsSettings *gnomeHintsSettings);

private:
    QStringList xdgIconThemePaths() const;

    gboolean m_gtkThemeDarkVariant;
    gchar *m_gtkTheme;
    QPalette *m_palette;
    GSettings *m_settings;
    QHash<QPlatformTheme::Font, QFont*> m_fonts;
    QHash<QPlatformTheme::ThemeHint, QVariant> m_hints;
};

#endif // GNOME_HINTS_SETTINGS_H
