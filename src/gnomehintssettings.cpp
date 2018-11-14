/*
 * Copyright (C) 2016 Jan Grulich
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

#include "gnomehintssettings.h"

#include <QDir>
#include <QString>
#include <QPalette>
#include <QMainWindow>
#include <QApplication>
#include <QGuiApplication>
#include <QDialogButtonBox>
#include <QToolBar>
#include <QLoggingCategory>
#include <QStyleFactory>
#include <QSettings>
#include <QStandardPaths>

Q_LOGGING_CATEGORY(QGnomePlatform, "qt.qpa.qgnomeplatform")

void gtkMessageHandler(const gchar *log_domain,
                       GLogLevelFlags log_level,
                       const gchar *message,
                       gpointer unused_data) {
    /* Silence false-positive Gtk warnings (we are using Xlib to set
     * the WM_TRANSIENT_FOR hint).
     */
    if (g_strcmp0(message, "GtkDialog mapped without a transient parent. "
                           "This is discouraged.") != 0) {
        /* For other messages, call the default handler. */
        g_log_default_handler(log_domain, log_level, message, unused_data);
    }
}

static const char* SETTINGS_SCHEME_GNOME = "org.gnome.desktop.interface";
static const char* SETTINGS_SCHEME_CINNAMON = "org.cinnamon.desktop.interface";

GnomeHintsSettings::GnomeHintsSettings()
    : QObject(0)
    , m_gtkThemeDarkVariant(false)
    , m_palette(nullptr)
    , m_settingsFallback(g_settings_new(SETTINGS_SCHEME_GNOME))
{
    gtk_init(nullptr, nullptr);

    configureSettingsSource();

    // Set log handler to suppress false GtkDialog warnings
    g_log_set_handler("Gtk", G_LOG_LEVEL_MESSAGE, gtkMessageHandler, NULL);

    // Get current theme and variant
    loadTheme();

    loadStaticHints();

    m_hints[QPlatformTheme::DialogButtonBoxLayout] = QDialogButtonBox::GnomeLayout;
    m_hints[QPlatformTheme::DialogButtonBoxButtonsHaveIcons] = true;
    m_hints[QPlatformTheme::KeyboardScheme] = QPlatformTheme::GnomeKeyboardScheme;
    m_hints[QPlatformTheme::IconPixmapSizes] = QVariant::fromValue(QList<int>() << 512 << 256 << 128 << 64 << 32 << 22 << 16 << 8);
    m_hints[QPlatformTheme::PasswordMaskCharacter] = QVariant(QChar(0x2022));

    // Watch for changes
    setupSettingsSignalHandling(m_settings);
    setupSettingsSignalHandling(m_settingsFallback);

    // g_signal_connect(gtk_settings_get_default(), "notify::gtk-theme-name", G_CALLBACK(gtkThemeChanged), this);

                                        /* Other theme hints */
    // KeyboardInputInterval, StartDragTime, KeyboardAutoRepeatRate, StartDragVelocity, DropShadow,
    // MaximumScrollBarDragDistance, ItemViewActivateItemOnSingleClick, WindowAutoPlacement, DialogButtonBoxButtonsHaveIcons
    // UseFullScreenForPopupMenu, UiEffects, SpellCheckUnderlineStyle, TabFocusBehavior, TabAllWidgets, PasswordMaskCharacter
    // DialogSnapToDefaultButton, ContextMenuOnMouseRelease, WheelScrollLines
    //  TODO TextCursorWidth, ToolButtonStyle, ToolBarIconSize

    // Load fonts
    loadFonts();

    // Load palette
    loadPalette();
}

GnomeHintsSettings::~GnomeHintsSettings()
{
    qDeleteAll(m_fonts);
    delete m_palette;
}

void GnomeHintsSettings::gsettingPropertyChanged(GSettings *settings, gchar *key, GnomeHintsSettings *gnomeHintsSettings)
{
    Q_UNUSED(settings);

    const QString changedProperty = key;

    if (changedProperty == QLatin1String("gtk-theme")) {
        gnomeHintsSettings->themeChanged();
    } else if (changedProperty == QLatin1String("icon-theme")) {
        gnomeHintsSettings->iconsChanged();
    } else if (changedProperty == QLatin1String("cursor-blink-time")) {
        gnomeHintsSettings->cursorBlinkTimeChanged();
    } else if (changedProperty == QLatin1String("font-name")) {
        gnomeHintsSettings->fontChanged();
    } else if (changedProperty == QLatin1String("monospace-font-name")) {
        gnomeHintsSettings->fontChanged();
    } else if (changedProperty == QLatin1String("text-scaling-factor")) {
        gnomeHintsSettings->fontChanged();
    } else {
        qCDebug(QGnomePlatform) << "GSetting property change: " << key;
    }
}

void GnomeHintsSettings::cursorBlinkTimeChanged()
{
    gint cursorBlinkTime = settingsGetInt("cursor-blink-time");
    if (cursorBlinkTime >= 100) {
        qCDebug(QGnomePlatform) << "Cursor blink time changed to: " << cursorBlinkTime;
        m_hints[QPlatformTheme::CursorFlashTime] = cursorBlinkTime;
    } else {
        qCDebug(QGnomePlatform) << "Cursor blink time changed to: 1200";
        m_hints[QPlatformTheme::CursorFlashTime] = 1200;
    }

    //If we are not a QApplication, means that we are a QGuiApplication, then we do nothing.
    if (!qobject_cast<QApplication *>(QCoreApplication::instance())) {
        return;
    }

    QWidgetList widgets = QApplication::allWidgets();
    Q_FOREACH (QWidget *widget, widgets) {
        if (qobject_cast<QToolBar *>(widget) || qobject_cast<QMainWindow *>(widget)) {
            QEvent event(QEvent::StyleChange);
            QApplication::sendEvent(widget, &event);
        }
    }
}

void GnomeHintsSettings::fontChanged()
{
    const QFont oldSysFont = *m_fonts[QPlatformTheme::SystemFont];
    loadFonts();

    if (qobject_cast<QApplication *>(QCoreApplication::instance())) {
        QApplication::setFont(*m_fonts[QPlatformTheme::SystemFont]);
        QWidgetList widgets = QApplication::allWidgets();
        Q_FOREACH (QWidget *widget, widgets) {
            if (widget->font() == oldSysFont) {
                widget->setFont(*m_fonts[QPlatformTheme::SystemFont]);
            }
        }
    } else {
        QGuiApplication::setFont(*m_fonts[QPlatformTheme::SystemFont]);
    }
}

void GnomeHintsSettings::iconsChanged()
{
    gchar *systemIconTheme = settingsGetString("icon-theme");
    if (systemIconTheme) {
        qCDebug(QGnomePlatform) << "Icon theme changed to: " << systemIconTheme;
        m_hints[QPlatformTheme::SystemIconThemeName] = systemIconTheme;
        free(systemIconTheme);
    } else {
        qCDebug(QGnomePlatform) << "Icon theme changed to: Adwaita";
        m_hints[QPlatformTheme::SystemIconThemeName] = "Adwaita";
    }

    //If we are not a QApplication, means that we are a QGuiApplication, then we do nothing.
    if (!qobject_cast<QApplication *>(QCoreApplication::instance())) {
        return;
    }

    QWidgetList widgets = QApplication::allWidgets();
    Q_FOREACH (QWidget *widget, widgets) {
        if (qobject_cast<QToolBar *>(widget) || qobject_cast<QMainWindow *>(widget)) {
            QEvent event(QEvent::StyleChange);
            QApplication::sendEvent(widget, &event);
        }
    }
}

void GnomeHintsSettings::themeChanged()
{
    loadPalette();
    loadTheme();

    // QApplication::setPalette and QGuiApplication::setPalette are different functions
    // and non virtual. Call the correct one
    if (qobject_cast<QApplication *>(QCoreApplication::instance())) {
        QApplication::setPalette(*m_palette);
        if (QStyleFactory::keys().contains(m_gtkTheme, Qt::CaseInsensitive))
            QApplication::setStyle(m_gtkTheme);
    } else if (qobject_cast<QGuiApplication *>(QCoreApplication::instance())) {
        QGuiApplication::setPalette(*m_palette);
    }
}

void GnomeHintsSettings::loadTheme()
{
    // g_object_get(gtk_settings_get_default(), "gtk-theme-name", &m_gtkTheme, NULL);
    m_gtkTheme = settingsGetString("gtk-theme");
    g_object_get(gtk_settings_get_default(), "gtk-application-prefer-dark-theme", &m_gtkThemeDarkVariant, NULL);

    if (!m_gtkTheme) {
        qCWarning(QGnomePlatform) << "Couldn't get current gtk theme!";
    } else {
        qCDebug(QGnomePlatform) << "Theme name: " << m_gtkTheme;
        qCDebug(QGnomePlatform) << "Dark version: " << (m_gtkThemeDarkVariant ? "yes" : "no");
    }

    QStringList styleNames;

    // First try to use GTK theme if it's Qt version is available
    styleNames << m_gtkTheme;

    // Detect if we have a Kvantum theme for this Gtk theme
    QString kvTheme = kvantumThemeForGtkTheme();

    if (!kvTheme.isEmpty()) {
        // Found matching Kvantum theme, configure user's Kvantum setting to use this
        configureKvantum(kvTheme);

        if (m_gtkThemeDarkVariant) {
            styleNames << QStringLiteral("kvantum-dark");
        }
        styleNames << QStringLiteral("kvantum");
    }

    // Otherwise, use adwaita or try default themes
    if (m_gtkThemeDarkVariant) {
        styleNames << QStringLiteral("adwaita-dark");
    }

    styleNames << QStringLiteral("adwaita")
               // Avoid using gtk+ style as it uses gtk2 and we use gtk3 which is causing a crash
               // << QStringLiteral("gtk+")
               << QStringLiteral("fusion")
               << QStringLiteral("windows");
    m_hints[QPlatformTheme::StyleNames] = styleNames;
}

void GnomeHintsSettings::loadFonts()
{
    qDeleteAll(m_fonts);
    m_fonts.clear();

    // Qt 5.6 introduced new HiDPI support. It uses evironment settings to take DPI information
    // and recalculates font and interface scaling. Adding 'text-scaling-factor' doubling
    // scaling and fonts will be so huge.
#if (QT_VERSION < QT_VERSION_CHECK(5, 6, 0))
    gdouble scaling = settingsGetDouble("text-scaling-factor");
    qCDebug(QGnomePlatform) << "Font scaling: " << scaling;
#else
    gdouble scaling = 1.0;
    qCDebug(QGnomePlatform) << "Font scaling: forced to 1.0 (for Qt>=5.6)";
#endif

    const QStringList fontTypes { "font-name", "monospace-font-name" };

    Q_FOREACH (const QString fontType, fontTypes) {
        gchar *fontName = settingsGetString(fontType.toStdString().c_str(), nullptr);
        if (!fontName) {
            qCWarning(QGnomePlatform) << "Couldn't get " << fontType;
        } else {
            QString fontNameString(fontName);
            QRegExp re("(.+)[ \t]+([0-9]+)");
            int fontSize;
            if (re.indexIn(fontNameString) == 0) {
                fontSize = re.cap(2).toInt();
                QFont* font = new QFont(re.cap(1));
                font->setPointSizeF(fontSize * scaling);
                if (fontType == QLatin1String("font-name")) {
                    m_fonts[QPlatformTheme::SystemFont] = font;
                    qCDebug(QGnomePlatform) << "Font name: " << re.cap(1) << " (size " << fontSize << ")";
                } else if (fontType == QLatin1String("monospace-font-name")) {
                    m_fonts[QPlatformTheme::FixedFont] = font;
                    qCDebug(QGnomePlatform) << "Monospace font name: " << re.cap(1) << " (size " << fontSize << ")";
                }
            } else {
                if (fontType == QLatin1String("font-name")) {
                    m_fonts[QPlatformTheme::SystemFont] = new QFont(fontNameString);
                    qCDebug(QGnomePlatform) << "Font name: " << fontNameString;
                } else if (fontType == QLatin1String("monospace-font-name")) {
                    m_fonts[QPlatformTheme::FixedFont] = new QFont(fontNameString);
                    qCDebug(QGnomePlatform) << "Monospace font name: " << fontNameString;
                }
            }
            free(fontName);
        }
    }
}

void GnomeHintsSettings::loadPalette()
{
    if (m_palette) {
        delete m_palette;
        m_palette = nullptr;
    }

    m_palette = new QPalette();
}

void GnomeHintsSettings::loadStaticHints() {
    gint cursorBlinkTime = settingsGetInt("cursor-blink-time");
//     g_object_get(gtk_settings_get_default(), "gtk-cursor-blink-time", &cursorBlinkTime, NULL);
    if (cursorBlinkTime >= 100) {
        qCDebug(QGnomePlatform) << "Cursor blink time: " << cursorBlinkTime;
        m_hints[QPlatformTheme::CursorFlashTime] = cursorBlinkTime;
    } else {
        m_hints[QPlatformTheme::CursorFlashTime] = 1200;
    }

    gint doubleClickTime = 400;
    g_object_get(gtk_settings_get_default(), "gtk-double-click-time", &doubleClickTime, NULL);
    qCDebug(QGnomePlatform) << "Double click time: " << doubleClickTime;
    m_hints[QPlatformTheme::MouseDoubleClickInterval] = doubleClickTime;

    guint longPressTime = 500;
    g_object_get(gtk_settings_get_default(), "gtk-long-press-time", &longPressTime, NULL);
    qCDebug(QGnomePlatform) << "Long press time: " << longPressTime;
    m_hints[QPlatformTheme::MousePressAndHoldInterval] = longPressTime;

    gint doubleClickDistance = 5;
    g_object_get(gtk_settings_get_default(), "gtk-double-click-distance", &doubleClickDistance, NULL);
    qCDebug(QGnomePlatform) << "Double click distance: " << doubleClickDistance;
    m_hints[QPlatformTheme::MouseDoubleClickDistance] = doubleClickDistance;

    gint startDragDistance = 8;
    g_object_get(gtk_settings_get_default(), "gtk-dnd-drag-threshold", &startDragDistance, NULL);
    qCDebug(QGnomePlatform) << "Dnd drag threshold: " << startDragDistance;
    m_hints[QPlatformTheme::StartDragDistance] = startDragDistance;

    guint passwordMaskDelay = 0;
    g_object_get(gtk_settings_get_default(), "gtk-entry-password-hint-timeout", &passwordMaskDelay, NULL);
    qCDebug(QGnomePlatform) << "Password hint timeout: " << passwordMaskDelay;
    m_hints[QPlatformTheme::PasswordMaskDelay] = passwordMaskDelay;

    gchar *systemIconTheme = settingsGetString("icon-theme");
//     g_object_get(gtk_settings_get_default(), "gtk-icon-theme-name", &systemIconTheme, NULL);
    if (systemIconTheme) {
        qCDebug(QGnomePlatform) << "Icon theme: " << systemIconTheme;
        m_hints[QPlatformTheme::SystemIconThemeName] = systemIconTheme;
        free(systemIconTheme);
    } else {
        m_hints[QPlatformTheme::SystemIconThemeName] = "Adwaita";
    }
    m_hints[QPlatformTheme::SystemIconFallbackThemeName] = "breeze";
    m_hints[QPlatformTheme::IconThemeSearchPaths] = xdgIconThemePaths();
}

QStringList GnomeHintsSettings::xdgIconThemePaths() const
{
    QStringList paths;

    const QFileInfo homeIconDir(QDir::homePath() + QStringLiteral("/.icons"));
    if (homeIconDir.isDir()) {
        paths << homeIconDir.absoluteFilePath();
    }

    QString xdgDirString = QFile::decodeName(qgetenv("XDG_DATA_DIRS"));

    if (xdgDirString.isEmpty()) {
        xdgDirString = QStringLiteral("/usr/local/share:/usr/share");
    }

    Q_FOREACH (const QString &xdgDir, xdgDirString.split(QLatin1Char(':'))) {
        const QFileInfo xdgIconsDir(xdgDir + QStringLiteral("/icons"));
        if (xdgIconsDir.isDir()) {
            paths << xdgIconsDir.absoluteFilePath();
        }
    }

    return paths;
}

QString GnomeHintsSettings::kvantumThemeForGtkTheme() const
{
    if (!m_gtkTheme) {
        // No Gtk theme? Then can't match to Kvantum!
        return QString();
    }

    QString gtkName(m_gtkTheme);
    QStringList dirs = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);

    // Look for a matching KVantum config file in the theme's folder
    Q_FOREACH(const QString &dir, dirs) {
        if (QFile::exists(QStringLiteral("%1/themes/%2/Kvantum/%3.kvconfig").arg(dir).arg(gtkName).arg(gtkName))) {
            return gtkName;
        }
    }

    // No config found in theme folder, look for a Kv<Theme> as shipped as part of Kvantum itself
    // (Kvantum ships KvAdapta, KvAmbiance, KvArc, etc.
    QStringList names { QStringLiteral("Kv") + gtkName };

    // Convert Ark-Dark to ArcDark to look for KvArcDark
    if (gtkName.indexOf("-") != -1) {
        names.append("Kv" + gtkName.replace("-", ""));
    }

    Q_FOREACH(const QString &name, names) {
        Q_FOREACH(const QString &dir, dirs) {
            if (QFile::exists(QStringLiteral("%1/Kvantum/%2/%3.kvconfig").arg(dir).arg(name).arg(name))) {
                return name;
            }
        }
    }

    return QString();
}

void GnomeHintsSettings::configureKvantum(const QString &theme) const
{
    QSettings config(QDir::homePath() + "/.config/Kvantum/kvantum.kvconfig", QSettings::NativeFormat);
    if (!config.contains("theme") || config.value("theme").toString() != theme) {
        config.setValue("theme", theme);
    }
}

void GnomeHintsSettings::configureSettingsSource()
{
    auto settingsScheme = SETTINGS_SCHEME_GNOME;
    auto desktopSession = qgetenv("SETTINGS_SCHEME_GNOME");
    auto gdmSession = qgetenv("GDMSESSION");
    auto xdgCurrentDesktop = qgetenv("XDG_CURRENT_DESKTOP");
    auto xdgSessionDesktop = qgetenv("XDG_SESSION_DESKTOP");

    if (desktopSession == "cinnamon" ||
        gdmSession == "cinnamon" ||
        xdgCurrentDesktop == "X-Cinnamon" ||
        xdgSessionDesktop == "cinnamon") {
        settingsScheme = SETTINGS_SCHEME_CINNAMON;
    }

    m_settings = g_settings_new(settingsScheme);
}

void GnomeHintsSettings::setupSettingsSignalHandling(GSettings *settings)
{
    g_signal_connect(settings, "changed::gtk-theme", G_CALLBACK(gsettingPropertyChanged), this);
    g_signal_connect(settings, "changed::icon-theme", G_CALLBACK(gsettingPropertyChanged), this);
    g_signal_connect(settings, "changed::cursor-blink-time", G_CALLBACK(gsettingPropertyChanged), this);
    g_signal_connect(settings, "changed::font-name", G_CALLBACK(gsettingPropertyChanged), this);
    g_signal_connect(settings, "changed::monospace-font-name", G_CALLBACK(gsettingPropertyChanged), this);
    g_signal_connect(settings, "changed::text-scaling-factor", G_CALLBACK(gsettingPropertyChanged), this);
}

bool GnomeHintsSettings::settingsHasKey(GSettings *settings, const gchar *key)
{
    GSettingsSchema *schema = nullptr;
    g_object_get(G_OBJECT(settings), "settings-schema", &schema, NULL);
    if (!schema)
        return false;
    auto result = g_settings_schema_has_key(schema, key);
    return result;
}

GSettings* GnomeHintsSettings::selectSettingsSource(const gchar* key)
{
    if (settingsHasKey(m_settings, key))
        return m_settings;
    if (settingsHasKey(m_settingsFallback, key))
        return m_settingsFallback;
    return nullptr;
}

gdouble GnomeHintsSettings::settingsGetDouble(const gchar* name, gdouble def)
{
    auto settings = selectSettingsSource(name);
    if (settings)
        return g_settings_get_double(settings, name);
    return def;
}


gchar* GnomeHintsSettings::settingsGetString(const gchar* name, gchar *def)
{
    auto settings = selectSettingsSource(name);
    if (settings)
        return g_settings_get_string(settings, name);
    return def;
}


gint GnomeHintsSettings::settingsGetInt(const gchar* name, gint def)
{
    auto settings = selectSettingsSource(name);
    if (settings)
        return g_settings_get_int(settings, name);
    return def;

}

