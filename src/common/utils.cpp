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

#include "utils.h"

#include <QFont>

#include <pango/pango.h>

namespace Utils {

// FIXME: duplicate
QFont* qt_fontFromString(const QString &name)
{
    QFont *font = new QFont(QLatin1String("Sans"), 10);

    PangoFontDescription *desc = pango_font_description_from_string(name.toUtf8());
    font->setPointSizeF(static_cast<float>(pango_font_description_get_size(desc)) / PANGO_SCALE);

    QString family = QString::fromUtf8(pango_font_description_get_family(desc));
    if (!family.isEmpty()) {
        font->setFamily(family);
    }

    const int weight = pango_font_description_get_weight(desc);
    if (weight >= PANGO_WEIGHT_HEAVY) {
        font->setWeight(QFont::Black);
    } else if (weight >= PANGO_WEIGHT_ULTRABOLD) {
        font->setWeight(QFont::ExtraBold);
    } else if (weight >= PANGO_WEIGHT_BOLD) {
        font->setWeight(QFont::Bold);
    } else if (weight >= PANGO_WEIGHT_SEMIBOLD) {
        font->setWeight(QFont::DemiBold);
    } else if (weight >= PANGO_WEIGHT_MEDIUM) {
        font->setWeight(QFont::Medium);
    } else if (weight >= PANGO_WEIGHT_NORMAL) {
        font->setWeight(QFont::Normal);
    } else if (weight >= PANGO_WEIGHT_LIGHT) {
        font->setWeight(QFont::Light);
    } else if (weight >= PANGO_WEIGHT_ULTRALIGHT) {
        font->setWeight(QFont::ExtraLight);
    } else {
        font->setWeight(QFont::Thin);
    }

    PangoStyle style = pango_font_description_get_style(desc);
    if (style == PANGO_STYLE_ITALIC) {
        font->setStyle(QFont::StyleItalic);
    } else if (style == PANGO_STYLE_OBLIQUE) {
        font->setStyle(QFont::StyleOblique);
    } else {
        font->setStyle(QFont::StyleNormal);
    }

    pango_font_description_free(desc);
    return font;
}

GnomeSettings::TitlebarButtons titlebarButtonsFromString(const QString &layout)
{
    const QStringList btnList = layout.split(QLatin1Char(':'));
    if (btnList.count() == 2) {
        const QString &leftButtons = btnList.first();
        const QString &rightButtons = btnList.last();

        // TODO support button order
        GnomeSettings::TitlebarButtons buttons;
        if (leftButtons.contains(QStringLiteral("close")) || rightButtons.contains("close")) {
            buttons = buttons | GnomeSettings::CloseButton;
        }

        if (leftButtons.contains(QStringLiteral("maximize")) || rightButtons.contains("maximize")) {
            buttons = buttons | GnomeSettings::MaximizeButton;
        }

        if (leftButtons.contains(QStringLiteral("minimize")) || rightButtons.contains("minimize")) {
            buttons = buttons | GnomeSettings::MinimizeButton;
        }

        return buttons;
    }

    return GnomeSettings::CloseButton;
}

GnomeSettings::TitlebarButtonsPlacement titlebarButtonPlacementFromString(const QString &layout)
{
    const QStringList btnList = layout.split(QLatin1Char(':'));
    if (btnList.count() == 2) {
        const QString &leftButtons = btnList.first();
        return leftButtons.contains(QStringLiteral("close")) ? GnomeSettings::LeftPlacement : GnomeSettings::RightPlacement;
    }

    return GnomeSettings::RightPlacement;
}

}
