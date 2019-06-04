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
#include <QVariant>

#include <memory>

#undef signals
#include <gio/gio.h>
#include <gtk-3.0/gtk/gtk.h>
#include <gtk-3.0/gtk/gtksettings.h>
#define signals Q_SIGNALS

#include <qpa/qplatformtheme.h>

class QPalette;

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
        MaximizeButton = 0x04,
        AllButtons = 0x8
    };
    Q_DECLARE_FLAGS(TitlebarButtons, TitlebarButton);

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
    void fontChanged();
    void iconsChanged();
    void themeChanged();

private Q_SLOTS:
    void loadFonts();
    void loadTheme();
    void loadTitlebar();
    void loadPalette();
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

        if (m_usePortal) {
            QVariant value = m_portalSettings.value(QStringLiteral("org.gnome.desktop.interface")).value(property);
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
    QPalette *m_palette = nullptr;
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
