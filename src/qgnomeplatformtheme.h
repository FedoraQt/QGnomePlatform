/*
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
#ifndef QGNOMEPLATFORMTHEME_H
#define QGNOMEPLATFORMTHEME_H

#include <QVariant>
#include <QFont>
#include <QPalette>
#include <qpa/qplatformtheme.h>
#include <private/qgenericunixthemes_p.h>

#include <gio/gio.h>

class QGnomePlatformTheme : public QGnomeTheme
{
    //Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QStyleFactoryInterface" FILE "adwaita.json");
public:
    QGnomePlatformTheme();

    virtual QVariant themeHint(ThemeHint hint) const;
    virtual const QFont *font(Font type) const;
    virtual QPalette *palette(Palette type) const;
    virtual bool usePlatformNativeDialog(DialogType type) const;

protected:
    void getFont();
    void getIconTheme();
    void getGtkTheme();

    QFont *m_font { nullptr };
    QString m_themeName { "Adwaita" };
    QString m_iconThemeName { "Adwaita" };
    GSettings *m_settings { nullptr };

    const QStringList m_fallbackThemeNames { "adwaita", "gtk+", "oxygen", "breeze" };
};

#endif // STYLEPLUGIN_H
