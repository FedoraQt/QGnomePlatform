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
#include "gnomehintssettings.h"
#include "qgtk3dialoghelpers.h"
#include <QApplication>
#include <QStyleFactory>

QGnomePlatformTheme::QGnomePlatformTheme()
{
    loadSettings();
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

const QPalette *QGnomePlatformTheme::palette(Palette type) const
{
    QPalette *palette = m_hints->palette();
    if (palette && type == QPlatformTheme::SystemPalette) {
        return palette;
    } else {
        return QPlatformTheme::palette(type);
    }
}

bool QGnomePlatformTheme::usePlatformNativeDialog(QPlatformTheme::DialogType type) const
{
    switch (type) {
    case QPlatformTheme::FileDialog:
        return true;
    case QPlatformTheme::FontDialog:
    case QPlatformTheme::ColorDialog:
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
    case QPlatformTheme::ColorDialog:
    case QPlatformTheme::MessageDialog:
    default:
        return 0;
    }
}

const QFont *QGnomePlatformTheme::font(Font type) const
{
    return m_hints->font(type);
}

void QGnomePlatformTheme::loadSettings()
{
    m_hints = new GnomeHintsSettings;
}
