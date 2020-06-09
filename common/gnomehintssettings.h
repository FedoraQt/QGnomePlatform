/*
 * Copyright (C) 2016-2019 Jan Grulich
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

#include <QDBusVariant>
#include <QFont>
#include <QFlags>
#include <QObject>
#include <QPalette>
#include <QVariant>

#include <cmath>
#include <memory>

#undef signals
#include <gio/gio.h>
#include <gtk-3.0/gtk/gtk.h>
#include <gtk-3.0/gtk/gtksettings.h>
#define signals Q_SIGNALS

#include <qpa/qplatformtheme.h>

class GnomeHintsSettings : public QObject
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

    explicit GnomeHintsSettings();
    virtual ~GnomeHintsSettings();

    // Borrowed from the KColorUtils code
    static QColor mix(const QColor &c1, const QColor &c2, qreal bias = 0.5)
    {
        auto mixQreal = [](qreal a, qreal b, qreal bias) {
            return a + (b - a) * bias;
        };

        if (bias <= 0.0)
            return c1;
        if (bias >= 1.0)
            return c2;
        if (std::isnan(bias))
            return c1;

        qreal r = mixQreal(c1.redF(),   c2.redF(),   bias);
        qreal g = mixQreal(c1.greenF(), c2.greenF(), bias);
        qreal b = mixQreal(c1.blueF(),  c2.blueF(),  bias);
        qreal a = mixQreal(c1.alphaF(), c2.alphaF(), bias);

        return QColor::fromRgbF(r, g, b, a);
    }

    static QColor lighten(const QColor &color, qreal amount = 0.1)
    {
        qreal h, s, l, a;
        color.getHslF(&h, &s, &l, &a);

        qreal lightness = l + amount;
        if (lightness > 1)
            lightness = 1;
        return QColor::fromHslF(h, s, lightness, a);
    }

    static QColor darken(const QColor &color, qreal amount = 0.1)
    {
        qreal h, s, l, a;
        color.getHslF(&h, &s, &l, &a);

        qreal lightness = l - amount;
        if (lightness < 0)
            lightness = 0;

        return QColor::fromHslF(h, s, lightness, a);
    }

    static QColor desaturate(const QColor &color, qreal amount = 0.1)
    {
        qreal h, s, l, a;
        color.getHslF(&h, &s, &l, &a);

        qreal saturation = s - amount;
        if (saturation < 0)
            saturation = 0;
        return QColor::fromHslF(h, saturation, l, a);
    }

    static QColor transparentize(const QColor &color, qreal amount = 0.1)
    {
        qreal h, s, l, a;
        color.getHslF(&h, &s, &l, &a);

        qreal alpha = a - amount;
        if (alpha < 0)
            alpha = 0;
        return QColor::fromHslF(h, s, l, alpha);
    }

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

    inline QPalette * palette() const
    {
        QPalette *palette = new QPalette;

        if (m_gtkThemeDarkVariant) {
            // Colors defined in GTK adwaita style in _colors.scss
            QColor base_color = lighten(desaturate(QColor("#241f31"), 1.0), 0.02);
            QColor text_color = QColor("white");
            QColor bg_color = darken(desaturate(QColor("#3d3846"), 1.0), 0.04);
            QColor fg_color = QColor("#eeeeec");
            QColor selected_bg_color = darken(QColor("#3584e4"), 0.2);
            QColor selected_fg_color = QColor("white");
            QColor osd_text_color = QColor("white");
            QColor osd_bg_color = QColor("black");
            QColor shadow = transparentize(QColor("black"), 0.9);

            QColor backdrop_fg_color = mix(fg_color, bg_color);
            QColor backdrop_base_color = lighten(base_color, 0.01);
            QColor backdrop_selected_fg_color = mix(text_color, backdrop_base_color, 0.2);

            // This is the color we use as initial color for the gradient in normal state
            // Defined in _drawing.scss button(normal)
            QColor button_base_color = darken(bg_color, 0.01);

            QColor link_color = lighten(selected_bg_color, 0.2);
            QColor link_visited_color = lighten(selected_bg_color, 0.1);

            palette->setColor(QPalette::All,      QPalette::Window,          bg_color);
            palette->setColor(QPalette::All,      QPalette::WindowText,      fg_color);
            palette->setColor(QPalette::All,      QPalette::Base,            base_color);
            palette->setColor(QPalette::All,      QPalette::AlternateBase,   base_color);
            palette->setColor(QPalette::All,      QPalette::ToolTipBase,     osd_bg_color);
            palette->setColor(QPalette::All,      QPalette::ToolTipText,     osd_text_color);
            palette->setColor(QPalette::All,      QPalette::Text,            fg_color);
            palette->setColor(QPalette::All,      QPalette::Button,          button_base_color);
            palette->setColor(QPalette::All,      QPalette::ButtonText,      fg_color);
            palette->setColor(QPalette::All,      QPalette::BrightText,      text_color);

            palette->setColor(QPalette::All,      QPalette::Light,           lighten(button_base_color));
            palette->setColor(QPalette::All,      QPalette::Midlight,        mix(lighten(button_base_color), button_base_color));
            palette->setColor(QPalette::All,      QPalette::Mid,             mix(darken(button_base_color), button_base_color));
            palette->setColor(QPalette::All,      QPalette::Dark,            darken(button_base_color));
            palette->setColor(QPalette::All,      QPalette::Shadow,          shadow);

            palette->setColor(QPalette::All,      QPalette::Highlight,       selected_bg_color);
            palette->setColor(QPalette::All,      QPalette::HighlightedText, selected_fg_color);

            palette->setColor(QPalette::All,      QPalette::Link,            link_color);
            palette->setColor(QPalette::All,      QPalette::LinkVisited,     link_visited_color);


            QColor insensitive_fg_color = mix(fg_color, bg_color);
            QColor insensitive_bg_color = mix(bg_color, base_color, 0.4);

            palette->setColor(QPalette::Disabled, QPalette::Window,          insensitive_bg_color);
            palette->setColor(QPalette::Disabled, QPalette::WindowText,      insensitive_fg_color);
            palette->setColor(QPalette::Disabled, QPalette::Base,            base_color);
            palette->setColor(QPalette::Disabled, QPalette::AlternateBase,   base_color);
            palette->setColor(QPalette::Disabled, QPalette::Text,            insensitive_fg_color);
            palette->setColor(QPalette::Disabled, QPalette::Button,          insensitive_bg_color);
            palette->setColor(QPalette::Disabled, QPalette::ButtonText,      insensitive_fg_color);
            palette->setColor(QPalette::Disabled, QPalette::BrightText,      text_color);

            palette->setColor(QPalette::Disabled, QPalette::Light,           lighten(insensitive_bg_color));
            palette->setColor(QPalette::Disabled, QPalette::Midlight,        mix(lighten(insensitive_bg_color), insensitive_bg_color));
            palette->setColor(QPalette::Disabled, QPalette::Mid,             mix(darken(insensitive_bg_color), insensitive_bg_color));
            palette->setColor(QPalette::Disabled, QPalette::Dark,            darken(insensitive_bg_color));
            palette->setColor(QPalette::Disabled, QPalette::Shadow,          shadow);

            palette->setColor(QPalette::Disabled, QPalette::Highlight,       selected_bg_color);
            palette->setColor(QPalette::Disabled, QPalette::HighlightedText, selected_fg_color);

            palette->setColor(QPalette::Disabled, QPalette::Link,            link_color);
            palette->setColor(QPalette::Disabled, QPalette::LinkVisited,     link_visited_color);


            palette->setColor(QPalette::Inactive, QPalette::Window,          bg_color);
            palette->setColor(QPalette::Inactive, QPalette::WindowText,      backdrop_fg_color);
            palette->setColor(QPalette::Inactive, QPalette::Base,            backdrop_base_color);
            palette->setColor(QPalette::Inactive, QPalette::AlternateBase,   backdrop_base_color);
            palette->setColor(QPalette::Inactive, QPalette::Text,            backdrop_fg_color);
            palette->setColor(QPalette::Inactive, QPalette::Button,          button_base_color);
            palette->setColor(QPalette::Inactive, QPalette::ButtonText,      backdrop_fg_color);
            palette->setColor(QPalette::Inactive, QPalette::BrightText,      text_color);

            palette->setColor(QPalette::Inactive, QPalette::Light,           lighten(insensitive_bg_color));
            palette->setColor(QPalette::Inactive, QPalette::Midlight,        mix(lighten(insensitive_bg_color), insensitive_bg_color));
            palette->setColor(QPalette::Inactive, QPalette::Mid,             mix(darken(insensitive_bg_color), insensitive_bg_color));
            palette->setColor(QPalette::Inactive, QPalette::Dark,            darken(insensitive_bg_color));
            palette->setColor(QPalette::Inactive, QPalette::Shadow,          shadow);

            palette->setColor(QPalette::Inactive, QPalette::Highlight,       selected_bg_color);
            palette->setColor(QPalette::Inactive, QPalette::HighlightedText, backdrop_selected_fg_color);

            palette->setColor(QPalette::Inactive, QPalette::Link,            link_color);
            palette->setColor(QPalette::Inactive, QPalette::LinkVisited,     link_visited_color);
        } else {
            // Colors defined in GTK adwaita style in _colors.scss
            QColor base_color = QColor("white");
            QColor text_color = QColor("black");
            QColor bg_color = QColor("#f6f5f4");
            QColor fg_color = QColor("#2e3436");
            QColor selected_bg_color = QColor("#3584e4");
            QColor selected_fg_color = QColor("white");
            QColor osd_text_color = QColor("white");
            QColor osd_bg_color = QColor("black");
            QColor shadow = transparentize(QColor("black"), 0.9);

            QColor backdrop_fg_color = mix(fg_color, bg_color);
            QColor backdrop_base_color = darken(base_color, 0.01);
            QColor backdrop_selected_fg_color = backdrop_base_color;

            // This is the color we use as initial color for the gradient in normal state
            // Defined in _drawing.scss button(normal)
            QColor button_base_color = darken(bg_color, 0.04);

            QColor link_color = darken(selected_bg_color, 0.1);
            QColor link_visited_color = darken(selected_bg_color, 0.2);

            palette->setColor(QPalette::All,      QPalette::Window,          bg_color);
            palette->setColor(QPalette::All,      QPalette::WindowText,      fg_color);
            palette->setColor(QPalette::All,      QPalette::Base,            base_color);
            palette->setColor(QPalette::All,      QPalette::AlternateBase,   base_color);
            palette->setColor(QPalette::All,      QPalette::ToolTipBase,     osd_bg_color);
            palette->setColor(QPalette::All,      QPalette::ToolTipText,     osd_text_color);
            palette->setColor(QPalette::All,      QPalette::Text,            fg_color);
            palette->setColor(QPalette::All,      QPalette::Button,          button_base_color);
            palette->setColor(QPalette::All,      QPalette::ButtonText,      fg_color);
            palette->setColor(QPalette::All,      QPalette::BrightText,      text_color);

            palette->setColor(QPalette::All,      QPalette::Light,           lighten(button_base_color));
            palette->setColor(QPalette::All,      QPalette::Midlight,        mix(lighten(button_base_color), button_base_color));
            palette->setColor(QPalette::All,      QPalette::Mid,             mix(darken(button_base_color), button_base_color));
            palette->setColor(QPalette::All,      QPalette::Dark,            darken(button_base_color));
            palette->setColor(QPalette::All,      QPalette::Shadow,          shadow);

            palette->setColor(QPalette::All,      QPalette::Highlight,       selected_bg_color);
            palette->setColor(QPalette::All,      QPalette::HighlightedText, selected_fg_color);

            palette->setColor(QPalette::All,      QPalette::Link,            link_color);
            palette->setColor(QPalette::All,      QPalette::LinkVisited,     link_visited_color);

            QColor insensitive_fg_color = mix(fg_color, bg_color);
            QColor insensitive_bg_color = mix(bg_color, base_color, 0.4);

            palette->setColor(QPalette::Disabled, QPalette::Window,          insensitive_bg_color);
            palette->setColor(QPalette::Disabled, QPalette::WindowText,      insensitive_fg_color);
            palette->setColor(QPalette::Disabled, QPalette::Base,            base_color);
            palette->setColor(QPalette::Disabled, QPalette::AlternateBase,   base_color);
            palette->setColor(QPalette::Disabled, QPalette::Text,            insensitive_fg_color);
            palette->setColor(QPalette::Disabled, QPalette::Button,          insensitive_bg_color);
            palette->setColor(QPalette::Disabled, QPalette::ButtonText,      insensitive_fg_color);
            palette->setColor(QPalette::Disabled, QPalette::BrightText,      text_color);

            palette->setColor(QPalette::Disabled, QPalette::Light,           lighten(insensitive_bg_color));
            palette->setColor(QPalette::Disabled, QPalette::Midlight,        mix(lighten(insensitive_bg_color), insensitive_bg_color));
            palette->setColor(QPalette::Disabled, QPalette::Mid,             mix(darken(insensitive_bg_color), insensitive_bg_color));
            palette->setColor(QPalette::Disabled, QPalette::Dark,            darken(insensitive_bg_color));
            palette->setColor(QPalette::Disabled, QPalette::Shadow,          shadow);

            palette->setColor(QPalette::Disabled, QPalette::Highlight,       selected_bg_color);
            palette->setColor(QPalette::Disabled, QPalette::HighlightedText, selected_fg_color);

            palette->setColor(QPalette::Disabled, QPalette::Link,            link_color);
            palette->setColor(QPalette::Disabled, QPalette::LinkVisited,     link_visited_color);


            palette->setColor(QPalette::Inactive, QPalette::Window,          bg_color);
            palette->setColor(QPalette::Inactive, QPalette::WindowText,      backdrop_fg_color);
            palette->setColor(QPalette::Inactive, QPalette::Base,            backdrop_base_color);
            palette->setColor(QPalette::Inactive, QPalette::AlternateBase,   backdrop_base_color);
            palette->setColor(QPalette::Inactive, QPalette::Text,            backdrop_fg_color);
            palette->setColor(QPalette::Inactive, QPalette::Button,          button_base_color);
            palette->setColor(QPalette::Inactive, QPalette::ButtonText,      backdrop_fg_color);
            palette->setColor(QPalette::Inactive, QPalette::BrightText,      text_color);

            palette->setColor(QPalette::Inactive, QPalette::Light,           lighten(insensitive_bg_color));
            palette->setColor(QPalette::Inactive, QPalette::Midlight,        mix(lighten(insensitive_bg_color), insensitive_bg_color));
            palette->setColor(QPalette::Inactive, QPalette::Mid,             mix(darken(insensitive_bg_color), insensitive_bg_color));
            palette->setColor(QPalette::Inactive, QPalette::Dark,            darken(insensitive_bg_color));
            palette->setColor(QPalette::Inactive, QPalette::Shadow,          shadow);

            palette->setColor(QPalette::Inactive, QPalette::Highlight,       selected_bg_color);
            palette->setColor(QPalette::Inactive, QPalette::HighlightedText, backdrop_selected_fg_color);

            palette->setColor(QPalette::Inactive, QPalette::Link,            link_color);
            palette->setColor(QPalette::Inactive, QPalette::LinkVisited,     link_visited_color);
        }

        return palette;
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

    inline TitlebarButtons titlebarButtons() const
    {
        return m_titlebarButtons;
    }

    inline TitlebarButtonsPlacement titlebarButtonPlacement() const
    {
        return m_titlebarButtonPlacement;
    }

public Q_SLOTS:
    void cursorBlinkTimeChanged();
    void cursorSizeChanged();
    void fontChanged();
    void iconsChanged();
    void themeChanged();

private Q_SLOTS:
    void loadFonts();
    void loadTheme();
    void loadTitlebar();
    void loadStaticHints();
    void portalSettingChanged(const QString &group, const QString &key, const QDBusVariant &value);

protected:
    static void gsettingPropertyChanged(GSettings *settings, gchar *key, GnomeHintsSettings *gnomeHintsSettings);

private:
    template <typename T> T getSettingsProperty(GSettings *settings, const QString &property, bool *ok = nullptr) {
        Q_UNUSED(settings); Q_UNUSED(property); Q_UNUSED(ok);
        return {};
    }
    template <typename T>
    T getSettingsProperty(const QString &property, bool *ok = nullptr) {
        GSettings *settings = m_settings;

        // In case of Cinnamon session, we most probably want to return the value from here if possible
        if (m_cinnamonSettings) {
            GSettingsSchema *schema;
            g_object_get(G_OBJECT(m_cinnamonSettings), "settings-schema", &schema, NULL);

            if (schema) {
                if (g_settings_schema_has_key(schema, property.toStdString().c_str())) {
                    settings = m_cinnamonSettings;
                }
            }
        }

        // Use org.gnome.desktop.wm.preferences if the property is there, otherwise it would bail on
        // non-existent property
        GSettingsSchema *schema;
        g_object_get(G_OBJECT(m_gnomeDesktopSettings), "settings-schema", &schema, NULL);

        if (schema) {
            if (g_settings_schema_has_key(schema, property.toStdString().c_str())) {
                settings = m_gnomeDesktopSettings;
            }
        }

        if (m_usePortal) {
            QVariant value = m_portalSettings.value(QStringLiteral("org.gnome.desktop.interface")).value(property);
            if (!value.isNull() && value.canConvert<T>())
                return value.value<T>();
            value = m_portalSettings.value(QStringLiteral("org.gnome.desktop.wm.preferences")).value(property);
            if (!value.isNull() && value.canConvert<T>())
                return value.value<T>();
        }

        return getSettingsProperty<T>(settings, property, ok);
    }
    QStringList xdgIconThemePaths() const;
    QString kvantumThemeForGtkTheme() const;
    void configureKvantum(const QString &theme) const;

    bool m_usePortal;
    bool m_gtkThemeDarkVariant = false;
    TitlebarButtons m_titlebarButtons = TitlebarButton::CloseButton;
    TitlebarButtonsPlacement m_titlebarButtonPlacement = TitlebarButtonsPlacement::RightPlacement;
    QString m_gtkTheme = nullptr;
    GSettings *m_cinnamonSettings = nullptr;
    GSettings *m_gnomeDesktopSettings = nullptr;
    GSettings *m_settings = nullptr;
    QHash<QPlatformTheme::Font, QFont*> m_fonts;
    QHash<QPlatformTheme::ThemeHint, QVariant> m_hints;
    QMap<QString, QVariantMap> m_portalSettings;
};

template <> inline int GnomeHintsSettings::getSettingsProperty(GSettings *settings, const QString &property, bool *ok) {
    if (ok)
        *ok = true;
    return g_settings_get_int(settings, property.toStdString().c_str());
}

template <> inline QString GnomeHintsSettings::getSettingsProperty(GSettings *settings, const QString &property, bool *ok) {
    // be exception and resources safe
    std::unique_ptr<gchar, void(*)(gpointer)> raw {g_settings_get_string(settings, property.toStdString().c_str()), g_free};
    if (ok)
        *ok = !!raw;
    return QString{raw.get()};
}

template <> inline qreal GnomeHintsSettings::getSettingsProperty(GSettings *settings, const QString &property, bool *ok) {
    if (ok)
        *ok = true;
    return g_settings_get_double(settings, property.toStdString().c_str());
}

Q_DECLARE_OPERATORS_FOR_FLAGS(GnomeHintsSettings::TitlebarButtons)

#endif // GNOME_HINTS_SETTINGS_H
