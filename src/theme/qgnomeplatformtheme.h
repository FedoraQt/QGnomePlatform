/*
 * Copyright (C) 2017-2022 Jan Grulich <jgrulich@redhat.com>
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
#ifndef QGNOME_PLATFORM_THEME_H
#define QGNOME_PLATFORM_THEME_H

#include <QFont>
#include <QPalette>
#include <QVariant>
#include <qpa/qplatformtheme.h>

class QGnomePlatformTheme : public QPlatformTheme
{
public:
    QGnomePlatformTheme();
    ~QGnomePlatformTheme();

    QVariant themeHint(ThemeHint hint) const Q_DECL_OVERRIDE;
    const QFont *font(Font type) const Q_DECL_OVERRIDE;
    const QPalette *palette(Palette type = SystemPalette) const Q_DECL_OVERRIDE;
    bool usePlatformNativeDialog(DialogType type) const Q_DECL_OVERRIDE;
    QPlatformDialogHelper *createPlatformDialogHelper(DialogType type) const Q_DECL_OVERRIDE;
#ifndef QT_NO_SYSTEMTRAYICON
    QPlatformSystemTrayIcon *createPlatformSystemTrayIcon() const Q_DECL_OVERRIDE;
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    Qt::ColorScheme colorScheme() const Q_DECL_OVERRIDE;
#elif QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
    QPlatformTheme::Appearance appearance() const Q_DECL_OVERRIDE;
#endif

private:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Used to load Qt's internall platform theme to get access to
    // non-public stuff, like QDBusTrayIcon
    QPlatformTheme *m_platformTheme = nullptr;
#endif
};

#endif // QGNOME_PLATFORM_THEME_H
