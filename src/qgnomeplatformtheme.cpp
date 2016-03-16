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

#include "qgnomeplatformtheme.h"

#include <QApplication>
#include <QStyleFactory>
#include <QDebug>
#include <QRegExp>

QGnomePlatformTheme::QGnomePlatformTheme()
        : QGnomeTheme()
        , m_settings(g_settings_new("org.gnome.desktop.interface")) {
    getFont();
    getIconTheme();
    getGtkTheme();
}

QVariant QGnomePlatformTheme::themeHint(ThemeHint hint) const {
    switch(hint) {
        case StyleNames:
            return QStringList() << m_themeName << m_fallbackThemeNames;
        case SystemIconThemeName:
            return m_iconThemeName;
        case SystemIconFallbackThemeName:
            return "oxygen";
        default:
            return QGnomeTheme::themeHint(hint);
    }
}

const QFont *QGnomePlatformTheme::font(Font type) const {
    Q_UNUSED(type)
    return m_font;
}

QPalette *QGnomePlatformTheme::palette(Palette type) const {
    Q_UNUSED(type)
    return new QPalette();
}

bool QGnomePlatformTheme::usePlatformNativeDialog(DialogType type) const {
    Q_UNUSED(type)
    return true;
}

void QGnomePlatformTheme::getFont() {
    gdouble scaling = g_settings_get_double(m_settings, "text-scaling-factor");
    gchar *name = g_settings_get_string(m_settings, "font-name");
    if (!name)
        return;
    gchar *fixed = g_settings_get_string(m_settings, "monospace-font-name");
    if (!fixed) {
        free(name);
        return;
    }

    QString rawFont(name);

    if (m_font)
        delete m_font;

    QRegExp re("(.+)[ \t]+([0-9]+)");
    int fontSize;
    if (re.indexIn(rawFont) == 0) {
        fontSize = re.cap(2).toInt();
        m_font = new QFont(re.cap(1), fontSize, QFont::Normal);
    }
    else {
        m_font = new QFont(rawFont);
        fontSize = m_font->pointSize();
    }

    m_font->setPointSizeF(fontSize * scaling);

    QGuiApplication::setFont(*m_font);

    free(name);
}

void QGnomePlatformTheme::getIconTheme() {
    gchar *data = g_settings_get_string(m_settings, "icon-theme");
    if (!data)
        return;

    m_iconThemeName = QString(data);

    free(data);
}

void QGnomePlatformTheme::getGtkTheme() {
    gchar *data = g_settings_get_string(m_settings, "gtk-theme");
    if (!data)
        return;

    m_themeName = QString(data);

    free(data);
}
