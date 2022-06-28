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

#include "hintprovider.h"

#include <qpa/qplatformtheme.h>

#include <QDialogButtonBox>

HintProvider::HintProvider(QObject* parent)
    : QObject(parent)
{
    // Generic hints shared with all providers
    m_hints[QPlatformTheme::DialogButtonBoxLayout] = QDialogButtonBox::GnomeLayout;
    m_hints[QPlatformTheme::DialogButtonBoxButtonsHaveIcons] = true;
    m_hints[QPlatformTheme::KeyboardScheme] = QPlatformTheme::GnomeKeyboardScheme;
    m_hints[QPlatformTheme::IconPixmapSizes] = QVariant::fromValue(QList<int>() << 512 << 256 << 128 << 64 << 32 << 22 << 16 << 8);
    m_hints[QPlatformTheme::PasswordMaskCharacter] = QVariant(QChar(0x2022));
}

HintProvider::~HintProvider() {
    qDeleteAll(m_fonts);
}
