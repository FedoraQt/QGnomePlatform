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
#include "utils.h"

#include <qpa/qplatformtheme.h>

#include <QDialogButtonBox>
#include <QFont>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(QGnomePlatformHintProvider, "qt.qpa.qgnomeplatform.hintprovider")

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

void HintProvider::setCursorBlinkTime(int cursorBlinkTime)
{
    if (cursorBlinkTime >= 100) {
        qCDebug(QGnomePlatformHintProvider) << "Cursor blink time: " << cursorBlinkTime;
        m_hints[QPlatformTheme::CursorFlashTime] = cursorBlinkTime;
    } else {
        m_hints[QPlatformTheme::CursorFlashTime] = 1200;
    }
}

void HintProvider::setCursorSize(int cursorSize)
{
    m_cursorSize = cursorSize;
}

void HintProvider::setCursorTheme(const QString &cursorTheme)
{
    m_cursorTheme = cursorTheme;
}

void HintProvider::setIconTheme(const QString &iconTheme)
{
    if (!iconTheme.isEmpty()) {
        qCDebug(QGnomePlatformHintProvider) << "Icon theme: " << iconTheme;
        m_hints[QPlatformTheme::SystemIconThemeName] = iconTheme;
    } else {
        m_hints[QPlatformTheme::SystemIconThemeName] = "Adwaita";
    }
}

void HintProvider::setFonts(const QString &systemFont, const QString &monospaceFont, const QString &titlebarFont)
{
    qDeleteAll(m_fonts);
    m_fonts.clear();

    QFont *font = Utils::qt_fontFromString(systemFont);
    m_fonts[QPlatformTheme::SystemFont] = font;
    qCDebug(QGnomePlatformHintProvider) << "Font name: " << font->family() << " (size " << font->pointSize() << ")";

    QFont *fixedFont = Utils::qt_fontFromString(monospaceFont);
    m_fonts[QPlatformTheme::FixedFont] = fixedFont;
    qCDebug(QGnomePlatformHintProvider) << "Monospace font name: " << fixedFont->family() << " (size " << fixedFont->pointSize() << ")";

    QFont *tbarFont = Utils::qt_fontFromString(titlebarFont);
    m_fonts[QPlatformTheme::TitleBarFont] = tbarFont;
    qCDebug(QGnomePlatformHintProvider) << "TitleBar font name: " << tbarFont->family() << " (size " << tbarFont->pointSize() << ")";
}

void HintProvider::setTitlebar(const QString &buttonLayout)
{
    m_titlebarButtonPlacement = Utils::titlebarButtonPlacementFromString(buttonLayout);
    m_titlebarButtons = Utils::titlebarButtonsFromString(buttonLayout);
}

void HintProvider::setTheme(const QString &theme, GnomeSettings::Appearance appearance)
{
    m_gtkTheme = theme;
    qCDebug(QGnomePlatformHintProvider) << "GTK theme: " << m_gtkTheme;
    m_appearance = appearance;
    qCDebug(QGnomePlatformHintProvider) << "Prefer dark theme: " << (appearance == GnomeSettings::PreferDark ? "yes" : "no");
}

void HintProvider::setStaticHints(int doubleClickTime, int longPressTime, int doubleClickDistance, int startDragDistance, int passwordMaskDelay)
{
    qCDebug(QGnomePlatformHintProvider) << "Double click time: " << doubleClickTime;
    m_hints[QPlatformTheme::MouseDoubleClickInterval] = doubleClickTime;

    qCDebug(QGnomePlatformHintProvider) << "Long press time: " << longPressTime;
    m_hints[QPlatformTheme::MousePressAndHoldInterval] = longPressTime;

    qCDebug(QGnomePlatformHintProvider) << "Double click distance: " << doubleClickDistance;
    m_hints[QPlatformTheme::MouseDoubleClickDistance] = doubleClickDistance;

    qCDebug(QGnomePlatformHintProvider) << "Dnd drag threshold: " << startDragDistance;
    m_hints[QPlatformTheme::StartDragDistance] = startDragDistance;

    qCDebug(QGnomePlatformHintProvider) << "Password hint timeout: " << passwordMaskDelay;
    m_hints[QPlatformTheme::PasswordMaskDelay] = passwordMaskDelay;
}
