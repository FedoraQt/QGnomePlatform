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

#include "gsettingshintprovider.h"
#include "utils.h"

#include <QFont>
#include <QGuiApplication>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(QGnomePlatformGSettingsHintProvider, "qt.qpa.qgnomeplatform.gsettingshintprovider")

static GSettings *loadGSettingsSchema(const QString &schema)
{
    GSettingsSchemaSource *source = g_settings_schema_source_get_default();
    GSettingsSchema *gschema = nullptr;

    gschema = g_settings_schema_source_lookup(source, schema.toLatin1(), TRUE);
    if (!gschema) {
        return nullptr;
    }

    GSettings *settings = g_settings_new(schema.toLatin1());
    g_settings_schema_unref(gschema);
    return settings;
}

GSettingsHintProvider::GSettingsHintProvider(QObject *parent)
    : HintProvider(parent)
    , m_gnomeDesktopSettings(loadGSettingsSchema(QLatin1String("org.gnome.desktop.wm.preferences")))
    , m_settings(loadGSettingsSchema(QLatin1String("org.gnome.desktop.interface")))
{
    // Check if this is a Cinnamon session to use additionally a different setting scheme
    if (qgetenv("XDG_CURRENT_DESKTOP").toLower() == QStringLiteral("x-cinnamon")) {
        m_cinnamonSettings = loadGSettingsSchema(QLatin1String("org.cinnamon.desktop.interface"));
    }

    // Do not continue on missing GSettings
    if (!m_settings && !m_cinnamonSettings) {
        return;
    }

    // Watch for changes
    QStringList watchListDesktopInterface = {"changed::gtk-theme",
                                             "changed::icon-theme",
                                             "changed::cursor-blink-time",
                                             "changed::font-name",
                                             "changed::monospace-font-name",
                                             "changed::cursor-size"};
    for (const QString &watchedProperty : watchListDesktopInterface) {
        g_signal_connect(m_settings, watchedProperty.toStdString().c_str(), G_CALLBACK(gsettingPropertyChanged), this);

        // Additionally watch Cinnamon configuration
        if (m_cinnamonSettings) {
            g_signal_connect(m_cinnamonSettings, watchedProperty.toStdString().c_str(), G_CALLBACK(gsettingPropertyChanged), this);
        }
    }

    QStringList watchListWmPreferences = {"changed::titlebar-font", "changed::button-layout"};
    for (const QString &watchedProperty : watchListWmPreferences) {
        g_signal_connect(m_gnomeDesktopSettings, watchedProperty.toStdString().c_str(), G_CALLBACK(gsettingPropertyChanged), this);
    }

    loadCursorBlinkTime();
    loadCursorSize();
    loadCursorTheme();
    loadIconTheme();
    loadFonts();
    loadStaticHints();
    loadTheme();
    loadTitlebar();
}

GSettingsHintProvider::~GSettingsHintProvider()
{
    if (m_cinnamonSettings) {
        g_object_unref(m_cinnamonSettings);
    }
    g_object_unref(m_gnomeDesktopSettings);
    g_object_unref(m_settings);
}

void GSettingsHintProvider::gsettingPropertyChanged(GSettings *settings, gchar *key, GSettingsHintProvider *hintProvider)
{
    Q_UNUSED(settings)

    const QString changedProperty = key;

    qCDebug(QGnomePlatformGSettingsHintProvider) << "GSetting property change: " << key;

    if (changedProperty == QStringLiteral("gtk-theme")) {
        hintProvider->loadTheme();
        Q_EMIT hintProvider->themeChanged();
    } else if (changedProperty == QStringLiteral("icon-theme")) {
        hintProvider->loadIconTheme();
        Q_EMIT hintProvider->iconThemeChanged();
    } else if (changedProperty == QStringLiteral("cursor-blink-time")) {
        hintProvider->loadCursorBlinkTime();
        Q_EMIT hintProvider->cursorBlinkTimeChanged();
    } else if (changedProperty == QStringLiteral("font-name") || changedProperty == QStringLiteral("monospace-font-name")
               || changedProperty == QStringLiteral("titlebar-font")) {
        hintProvider->loadFonts();
        Q_EMIT hintProvider->fontChanged();
    } else if (changedProperty == QStringLiteral("cursor-size")) {
        hintProvider->loadCursorSize();
        ;
        Q_EMIT hintProvider->fontChanged();
    } else if (changedProperty == QStringLiteral("cursor-theme")) {
        hintProvider->loadCursorTheme();
        Q_EMIT hintProvider->cursorThemeChanged();
    } else if (changedProperty == QStringLiteral("button-layout")) {
        hintProvider->loadTitlebar();
        Q_EMIT hintProvider->titlebarChanged();
    }
}

void GSettingsHintProvider::loadCursorBlinkTime()
{
    const int cursorBlinkTime = getSettingsProperty<int>(QStringLiteral("cursor-blink-time"));
    setCursorBlinkTime(cursorBlinkTime);
}

void GSettingsHintProvider::loadCursorSize()
{
    const int cursorSize = getSettingsProperty<int>(QStringLiteral("cursor-size"));
    setCursorSize(cursorSize);
}

void GSettingsHintProvider::loadCursorTheme()
{
    const QString cursorTheme = getSettingsProperty<QString>(QStringLiteral("cursor-theme"));
    setCursorTheme(cursorTheme);
}

void GSettingsHintProvider::loadIconTheme()
{
    const QString systemIconTheme = getSettingsProperty<QString>(QStringLiteral("icon-theme"));
    setIconTheme(systemIconTheme);
}

void GSettingsHintProvider::loadFonts()
{
    const QString fontName = getSettingsProperty<QString>(QStringLiteral("font-name"));
    const QString monospaceFontName = getSettingsProperty<QString>(QStringLiteral("monospace-font-name"));
    const QString titlebarFontName = getSettingsProperty<QString>(QStringLiteral("titlebar-font"));

    setFonts(fontName, monospaceFontName, titlebarFontName);
}

void GSettingsHintProvider::loadTitlebar()
{
    const QString buttonLayout = getSettingsProperty<QString>("button-layout");
    setTitlebar(buttonLayout);
}

void GSettingsHintProvider::loadTheme()
{
    bool isDarkTheme;
    g_object_get(gtk_settings_get_default(), "gtk-application-prefer-dark-theme", &isDarkTheme, NULL);
    const QString theme = getSettingsProperty<QString>(QStringLiteral("gtk-theme"));
    const GnomeSettings::Appearance appearance = isDarkTheme ? GnomeSettings::PreferDark : GnomeSettings::PreferLight;
    setTheme(theme, appearance);
}

void GSettingsHintProvider::loadStaticHints()
{
    gint doubleClickTime = 400;
    g_object_get(gtk_settings_get_default(), "gtk-double-click-time", &doubleClickTime, NULL);

    guint longPressTime = 500;
    g_object_get(gtk_settings_get_default(), "gtk-long-press-time", &longPressTime, NULL);

    gint doubleClickDistance = 5;
    g_object_get(gtk_settings_get_default(), "gtk-double-click-distance", &doubleClickDistance, NULL);

    gint startDragDistance = 8;
    g_object_get(gtk_settings_get_default(), "gtk-dnd-drag-threshold", &startDragDistance, NULL);

    guint passwordMaskDelay = 0;
    g_object_get(gtk_settings_get_default(), "gtk-entry-password-hint-timeout", &passwordMaskDelay, NULL);

    setStaticHints(doubleClickTime, longPressTime, doubleClickDistance, startDragDistance, passwordMaskDelay);
}

template<typename T>
T GSettingsHintProvider::getSettingsProperty(GSettings *settings, const QString &property, bool *ok)
{
    Q_UNUSED(settings)
    Q_UNUSED(property)
    Q_UNUSED(ok)
    return {};
}

template<typename T>
T GSettingsHintProvider::getSettingsProperty(const QString &property, bool *ok)
{
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

    return getSettingsProperty<T>(settings, property, ok);
}

template<>
int GSettingsHintProvider::getSettingsProperty(GSettings *settings, const QString &property, bool *ok)
{
    if (ok) {
        *ok = true;
    }
    return g_settings_get_int(settings, property.toStdString().c_str());
}

template<>
QString GSettingsHintProvider::getSettingsProperty(GSettings *settings, const QString &property, bool *ok)
{
    // be exception and resources safe
    std::unique_ptr<gchar, void (*)(gpointer)> raw{g_settings_get_string(settings, property.toStdString().c_str()), g_free};
    if (ok) {
        *ok = !!raw;
    }
    return QString{raw.get()};
}

template<>
qreal GSettingsHintProvider::getSettingsProperty(GSettings *settings, const QString &property, bool *ok)
{
    if (ok) {
        *ok = true;
    }
    return g_settings_get_double(settings, property.toStdString().c_str());
}
