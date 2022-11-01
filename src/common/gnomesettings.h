/*
 * Copyright (C) 2016-2022 Jan Grulich <jgrulich@redhat.com>
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

#ifndef GNOME_SETTINGS_H
#define GNOME_SETTINGS_H

#include <QFlags>
#include <QObject>
#include <QStringList>

#include <qpa/qplatformtheme.h>

#include <memory>

class QFont;
class QVariant;
class QPalette;

class HintProvider;

class GnomeSettings : public QObject
{
    Q_OBJECT
public:
    enum Appearance { PreferDark = 1, PreferLight = 2 };
    enum TitlebarButtonsPlacement { LeftPlacement = 0, RightPlacement = 1 };
    enum TitlebarButton { CloseButton = 0x1, MinimizeButton = 0x02, MaximizeButton = 0x04 };
    Q_DECLARE_FLAGS(TitlebarButtons, TitlebarButton);

    explicit GnomeSettings(QObject *parent = nullptr);
    virtual ~GnomeSettings();

    static GnomeSettings &getInstance();

    QFont *font(QPlatformTheme::Font type) const;
    QPalette *palette() const;
    QVariant hint(QPlatformTheme::ThemeHint hint) const;
    bool canUseFileChooserPortal() const;
    bool useGtkThemeDarkVariant() const;
    bool useGtkThemeHighContrastVariant() const;
    QString gtkTheme() const;
    TitlebarButtons titlebarButtons() const;
    TitlebarButtonsPlacement titlebarButtonPlacement() const;

Q_SIGNALS:
    void themeChanged();
    void titlebarChanged();

private Q_SLOTS:
    void loadPalette();

    void onCursorBlinkTimeChanged();
    void onCursorSizeChanged();
    void onCursorThemeChanged();
    void onFontChanged();
    void onIconThemeChanged();
    void onThemeChanged();

    void onHintProviderChanged();

private:
    void configureKvantum(const QString &theme) const;
    void initializeHintProvider() const;
    QString kvantumThemeForGtkTheme() const;
    QStringList styleNames() const;
    QStringList xdgIconThemePaths() const;

    QFont *m_fallbackFont = nullptr;
    QPalette *m_palette = nullptr;

    std::unique_ptr<HintProvider> m_hintProvider;

    bool m_relyOnAppearance = false;
    bool m_isRunningInSandbox;
    bool m_canUseFileChooserPortal = false;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(GnomeSettings::TitlebarButtons)

#endif // GNOME_SETTINGS_H
