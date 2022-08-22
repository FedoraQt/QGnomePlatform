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

#ifndef GSETTINGS_HINT_PROVIDER_H
#define GSETTINGS_HINT_PROVIDER_H

#include "hintprovider.h"

#undef signals
#include <gio/gio.h>
#include <gtk-4.0/gtk/gtk.h>
#include <gtk-4.0/gtk/gtksettings.h>
#define signals Q_SIGNALS

class QFont;
class QString;
class QVariant;

class GSettingsHintProvider : public HintProvider
{
    Q_OBJECT
public:
    explicit GSettingsHintProvider(QObject *parent = nullptr);
    virtual ~GSettingsHintProvider();

protected:
    static void gsettingPropertyChanged(GSettings *settings, gchar *key, GSettingsHintProvider *hintProvider);

private:
    template<typename T>
    T getSettingsProperty(GSettings *settings, const QString &property, bool *ok = nullptr);
    template<typename T>
    T getSettingsProperty(const QString &property, bool *ok = nullptr);

    void loadCursorBlinkTime();
    void loadCursorSize();
    void loadCursorTheme();
    void loadIconTheme();
    void loadFonts();
    void loadTheme();
    void loadTitlebar();
    void loadStaticHints();

    GSettings *m_cinnamonSettings = nullptr;
    GSettings *m_gnomeDesktopSettings = nullptr;
    GSettings *m_settings = nullptr;
};

#endif // GSETTINGS_HINT_PROVIDER_H

