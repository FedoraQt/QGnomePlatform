/*
 * Copyright (C) 2016-2021 Jan Grulich
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

#ifndef GNOME_SETTINGS_P_H
#define GNOME_SETTINGS_P_H

#include "gnomesettings.h"

#undef signals
#include <gio/gio.h>
#include <gtk-3.0/gtk/gtk.h>
#include <gtk-3.0/gtk/gtksettings.h>
#define signals Q_SIGNALS

#include <QHash>
#include <QMap>

#include <qpa/qplatformtheme.h>

class QDBusVariant;
class QFont;
class QPalette;
class QString;
class QVariant;

class GnomeSettingsPrivate : public GnomeSettings
{
    Q_OBJECT
public:
    GnomeSettingsPrivate(QObject *parent = nullptr);
    virtual ~GnomeSettingsPrivate();

    QFont *font(QPlatformTheme::Font type) const;
    QPalette *palette() const;
    bool canUseFileChooserPortal() const;
    bool isGtkThemeDarkVariant() const;
    bool isGtkThemeHighContrastVariant() const;
    QString gtkTheme() const;
    QVariant hint(QPlatformTheme::ThemeHint hint) const;
    TitlebarButtons titlebarButtons() const;
    TitlebarButtonsPlacement titlebarButtonPlacement() const;
    void setOnThemeChanged(std::function<void()> callback);

private Q_SLOTS:
    void cursorBlinkTimeChanged();
    void cursorSizeChanged();
    void cursorThemeChanged();
    void fontChanged();
    void iconsChanged();
    void themeChanged();
    void loadFonts();
    void loadTheme();
    void loadTitlebar();
    void loadStaticHints();
    void portalSettingChanged(const QString &group, const QString &key, const QDBusVariant &value);

protected:
    static void gsettingPropertyChanged(GSettings *settings, gchar *key, GnomeSettingsPrivate *gnomeSettings);

private:
    template <typename T> T getSettingsProperty(GSettings *settings, const QString &property, bool *ok = nullptr);
    template <typename T> T getSettingsProperty(const QString &property, bool *ok = nullptr);
    QStringList xdgIconThemePaths() const;
    QString kvantumThemeForGtkTheme() const;
    void configureKvantum(const QString &theme) const;

    bool m_usePortal;
    bool m_canUseFileChooserPortal = false;
    bool m_gtkThemeDarkVariant = false;
    bool m_gtkThemeHighContrastVariant = false;
    TitlebarButtons m_titlebarButtons = TitlebarButton::CloseButton;
    TitlebarButtonsPlacement m_titlebarButtonPlacement = TitlebarButtonsPlacement::RightPlacement;
    QString m_gtkTheme = nullptr;
    GSettings *m_cinnamonSettings = nullptr;
    GSettings *m_gnomeDesktopSettings = nullptr;
    GSettings *m_settings = nullptr;
    QHash<QPlatformTheme::Font, QFont*> m_fonts;
    QHash<QPlatformTheme::ThemeHint, QVariant> m_hints;
    QMap<QString, QVariantMap> m_portalSettings;
    QPalette *m_palette = nullptr;
    QFont *m_fallbackFont = nullptr;
    std::function<void()> m_onThemeChanged = nullptr;
};

#endif // GNOME_SETTINGS_P_H
