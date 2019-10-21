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

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>

#include <QX11Info>

Q_LOGGING_CATEGORY(QGnomePlatform, "qt.qpa.qgnomeplatform")

const QDBusArgument &operator>>(const QDBusArgument &argument, QMap<QString, QVariantMap> &map)
{
    argument.beginMap();
    map.clear();

    while (!argument.atEnd()) {
        QString key;
        QVariantMap value;
        argument.beginMapEntry();
        argument >> key >> value;
        argument.endMapEntry();
        map.insert(key, value);
    }

    argument.endMap();
    return argument;
}

static inline bool checkUsePortalSupport()
{
    return !QStandardPaths::locate(QStandardPaths::RuntimeLocation, QStringLiteral("flatpak-info")).isEmpty() || qEnvironmentVariableIsSet("SNAP");
}

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

GnomeHintsSettings::GnomeHintsSettings()
    : QObject(0)
    , m_usePortal(checkUsePortalSupport())
    , m_gnomeDesktopSettings(g_settings_new("org.gnome.desktop.wm.preferences"))
    , m_settings(g_settings_new("org.gnome.desktop.interface"))
{
    gtk_init(nullptr, nullptr);

    // Set log handler to suppress false GtkDialog warnings
    g_log_set_handler("Gtk", G_LOG_LEVEL_MESSAGE, gtkMessageHandler, NULL);

    // Check if this is a Cinnamon session to use additionally a different setting scheme
    if (qgetenv("XDG_CURRENT_DESKTOP").toLower() == QStringLiteral("x-cinnamon")) {
        m_cinnamonSettings = g_settings_new("org.cinnamon.desktop.interface");
    }

    if (m_usePortal) {
        QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.portal.Desktop"),
                                                              QStringLiteral("/org/freedesktop/portal/desktop"),
                                                              QStringLiteral("org.freedesktop.portal.Settings"),
                                                              QStringLiteral("ReadAll"));
        message << QStringList{{QStringLiteral("org.gnome.desktop.interface")}, {QStringLiteral("org.gnome.desktop.wm.preferences")}};

        // FIXME: async?
        QDBusMessage resultMessage = QDBusConnection::sessionBus().call(message);
        if (resultMessage.type() == QDBusMessage::ReplyMessage) {
            QDBusArgument dbusArgument = resultMessage.arguments().at(0).value<QDBusArgument>();
            dbusArgument >> m_portalSettings;
        }
    }

    m_hints[QPlatformTheme::DialogButtonBoxLayout] = QDialogButtonBox::GnomeLayout;
    m_hints[QPlatformTheme::DialogButtonBoxButtonsHaveIcons] = true;
    m_hints[QPlatformTheme::KeyboardScheme] = QPlatformTheme::GnomeKeyboardScheme;
    m_hints[QPlatformTheme::IconPixmapSizes] = QVariant::fromValue(QList<int>() << 512 << 256 << 128 << 64 << 32 << 22 << 16 << 8);
    m_hints[QPlatformTheme::PasswordMaskCharacter] = QVariant(QChar(0x2022));

    // Watch for changes
    QStringList watchListDesktopInterface = { "changed::gtk-theme", "changed::icon-theme", "changed::cursor-blink-time", "changed::font-name", "changed::monospace-font-name", "changed::cursor-size" };
    for (const QString &watchedProperty : watchListDesktopInterface) {
        g_signal_connect(m_settings, watchedProperty.toStdString().c_str(), G_CALLBACK(gsettingPropertyChanged), this);

        // Additionally watch Cinnamon configuration
        if (m_cinnamonSettings) {
            g_signal_connect(m_cinnamonSettings, watchedProperty.toStdString().c_str(), G_CALLBACK(gsettingPropertyChanged), this);
        }
    }

    QStringList watchListWmPreferences = { "changed::titlebar-font", "changed::button-layout" };
    for (const QString &watchedProperty : watchListWmPreferences) {
        g_signal_connect(m_gnomeDesktopSettings, watchedProperty.toStdString().c_str(), G_CALLBACK(gsettingPropertyChanged), this);
    }

    if (m_usePortal) {
        QDBusConnection::sessionBus().connect(QString(), QStringLiteral("/org/freedesktop/portal/desktop"), QStringLiteral("org.freedesktop.portal.Settings"),
                                              QStringLiteral("SettingChanged"), this, SLOT(portalSettingChanged(QString,QString,QDBusVariant)));
    }

    if (!QX11Info::isPlatformX11())
        cursorSizeChanged();

    loadFonts();
    loadPalette();
    loadStaticHints();
    loadTheme();
    loadTitlebar();
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

    // Org.gnome.desktop.interface
    if (changedProperty == QStringLiteral("gtk-theme")) {
        gnomeHintsSettings->themeChanged();
    } else if (changedProperty == QStringLiteral("icon-theme")) {
        gnomeHintsSettings->iconsChanged();
    } else if (changedProperty == QStringLiteral("cursor-blink-time")) {
        gnomeHintsSettings->cursorBlinkTimeChanged();
    } else if (changedProperty == QStringLiteral("font-name")) {
        gnomeHintsSettings->fontChanged();
    } else if (changedProperty == QStringLiteral("monospace-font-name")) {
        gnomeHintsSettings->fontChanged();
    } else if (changedProperty == QStringLiteral("cursor-size")) {
        if (!QX11Info::isPlatformX11())
            gnomeHintsSettings->cursorSizeChanged();
    // Org.gnome.wm.preferences
    } else if (changedProperty == QStringLiteral("titlebar-font")) {
        gnomeHintsSettings->fontChanged();
    } else if (changedProperty == QStringLiteral("button-layout")) {
        gnomeHintsSettings->loadTitlebar();
    // Fallback
    } else {
        qCDebug(QGnomePlatform) << "GSetting property change: " << key;
    }
}

void GnomeHintsSettings::cursorBlinkTimeChanged()
{
    int cursorBlinkTime = getSettingsProperty<int>(QStringLiteral("cursor-blink-time"));
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
    for (QWidget *widget : widgets) {
        if (qobject_cast<QToolBar *>(widget) || qobject_cast<QMainWindow *>(widget)) {
            QEvent event(QEvent::StyleChange);
            QApplication::sendEvent(widget, &event);
        }
    }
}

void GnomeHintsSettings::cursorSizeChanged()
{
    int cursorSize = getSettingsProperty<int>(QStringLiteral("cursor-size"));
    qputenv("XCURSOR_SIZE", QString::number(cursorSize).toUtf8());
}

void GnomeHintsSettings::fontChanged()
{
    const QFont oldSysFont = *m_fonts[QPlatformTheme::SystemFont];
    loadFonts();

    if (qobject_cast<QApplication *>(QCoreApplication::instance())) {
        QApplication::setFont(*m_fonts[QPlatformTheme::SystemFont]);
        QWidgetList widgets = QApplication::allWidgets();
        for (QWidget *widget : widgets) {
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
    QString systemIconTheme = getSettingsProperty<QString>(QStringLiteral("icon-theme"));
    if (!systemIconTheme.isEmpty()) {
        qCDebug(QGnomePlatform) << "Icon theme changed to: " << systemIconTheme;
        m_hints[QPlatformTheme::SystemIconThemeName] = systemIconTheme;
    } else {
        qCDebug(QGnomePlatform) << "Icon theme changed to: Adwaita";
        m_hints[QPlatformTheme::SystemIconThemeName] = "Adwaita";
    }

    //If we are not a QApplication, means that we are a QGuiApplication, then we do nothing.
    if (!qobject_cast<QApplication *>(QCoreApplication::instance())) {
        return;
    }

    QWidgetList widgets = QApplication::allWidgets();
    for (QWidget *widget : widgets) {
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

void GnomeHintsSettings::loadTitlebar()
{
    const QString buttonLayout = getSettingsProperty<QString>("button-layout");

    if (buttonLayout.isEmpty()) {
        return;
    }

    const QStringList btnList = buttonLayout.split(QLatin1Char(':'));
    if (btnList.count() == 2) {
        const QString &leftButtons = btnList.first();
        const QString &rightButtons = btnList.last();

        m_titlebarButtonPlacement = leftButtons.contains(QStringLiteral("close")) ? GnomeHintsSettings::LeftPlacement : GnomeHintsSettings::RightPlacement;

        // TODO support button order
        TitlebarButtons buttons;
        if (leftButtons.contains(QStringLiteral("close")) || rightButtons.contains("close")) {
            buttons = buttons | GnomeHintsSettings::CloseButton;
        }

        if (leftButtons.contains(QStringLiteral("maximize")) || rightButtons.contains("maximize")) {
            buttons = buttons | GnomeHintsSettings::MaximizeButton;
        }

        if (leftButtons.contains(QStringLiteral("minimize")) || rightButtons.contains("minimize")) {
            buttons = buttons | GnomeHintsSettings::MinimizeButton;
        }

        m_titlebarButtons = buttons;
    }
}

void GnomeHintsSettings::loadTheme()
{
    // g_object_get(gtk_settings_get_default(), "gtk-theme-name", &m_gtkTheme, NULL);
    m_gtkTheme = getSettingsProperty<QString>(QStringLiteral("gtk-theme"));
    g_object_get(gtk_settings_get_default(), "gtk-application-prefer-dark-theme", &m_gtkThemeDarkVariant, NULL);

    if (m_gtkTheme.isEmpty()) {
        qCWarning(QGnomePlatform) << "Couldn't get current gtk theme!";
    } else {
        qCDebug(QGnomePlatform) << "Theme name: " << m_gtkTheme;

        if (m_gtkTheme.toLower() == QStringLiteral("adwaita-dark")) {
            m_gtkThemeDarkVariant = true;
        }

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
               << QStringLiteral("fusion")
               << QStringLiteral("windows");
    m_hints[QPlatformTheme::StyleNames] = styleNames;
}

void GnomeHintsSettings::loadFonts()
{
    qDeleteAll(m_fonts);
    m_fonts.clear();

    const QStringList fontTypes { "font-name", "monospace-font-name", "titlebar-font" };

    for (const QString &fontType : fontTypes) {
        const QString fontName = getSettingsProperty<QString>(fontType);
        if (fontName.isEmpty()) {
            qCWarning(QGnomePlatform) << "Couldn't get " << fontType;
        } else {
            bool bold = false;
            int fontSize;
            QString name;
            QRegExp re("(.+)[ \t]+([0-9]+)");
            if (re.indexIn(fontName) == 0) {
                fontSize = re.cap(2).toInt();
                name = re.cap(1);
                // Bold is most likely not part of the name
                if (name.endsWith(QStringLiteral(" Bold"))) {
                    bold = true;
                    name = name.remove(QStringLiteral(" Bold"));
                }

                QFont *font = new QFont(name, fontSize, bold ? QFont::Bold : QFont::Normal);
                if (fontType == QStringLiteral("font-name")) {
                    m_fonts[QPlatformTheme::SystemFont] = font;
                    qCDebug(QGnomePlatform) << "Font name: " << name << " (size " << fontSize << ")";
                } else if (fontType == QStringLiteral("monospace-font-name")) {
                    m_fonts[QPlatformTheme::FixedFont] = font;
                    qCDebug(QGnomePlatform) << "Monospace font name: " << name << " (size " << fontSize << ")";
                } else if (fontType == QStringLiteral("titlebar-font")) {
                    m_fonts[QPlatformTheme::TitleBarFont] = font;
                    qCDebug(QGnomePlatform) << "TitleBar font name: " << name << " (size " << fontSize << ")";
                }
            } else {
                if (fontType == QStringLiteral("font-name")) {
                    m_fonts[QPlatformTheme::SystemFont] = new QFont(fontName);
                    qCDebug(QGnomePlatform) << "Font name: " << fontName;
                } else if (fontType == QStringLiteral("monospace-font-name")) {
                    m_fonts[QPlatformTheme::FixedFont] = new QFont(fontName);
                    qCDebug(QGnomePlatform) << "Monospace font name: " << fontName;
                } else if (fontType == QStringLiteral("titlebar-font")) {
                    m_fonts[QPlatformTheme::TitleBarFont] = new QFont(fontName);
                    qCDebug(QGnomePlatform) << "TitleBar font name: " << fontName;
                }
            }
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
    int cursorBlinkTime = getSettingsProperty<int>(QStringLiteral("cursor-blink-time"));
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

    QString systemIconTheme = getSettingsProperty<QString>(QStringLiteral("icon-theme"));
    if (!systemIconTheme.isEmpty()) {
        qCDebug(QGnomePlatform) << "Icon theme: " << systemIconTheme;
        m_hints[QPlatformTheme::SystemIconThemeName] = systemIconTheme;
    } else {
        m_hints[QPlatformTheme::SystemIconThemeName] = "Adwaita";
    }
    m_hints[QPlatformTheme::SystemIconFallbackThemeName] = "breeze";
    m_hints[QPlatformTheme::IconThemeSearchPaths] = xdgIconThemePaths();
}

void GnomeHintsSettings::portalSettingChanged(const QString &group, const QString &key, const QDBusVariant &value)
{
    if (group == QStringLiteral("org.gnome.desktop.interface") || group == QStringLiteral("org.gnome.desktop.wm.preferences")) {
        m_portalSettings[group][key] = value.variant();
        gsettingPropertyChanged(nullptr, (gchar*)(key.toStdString().c_str()), this);
    }
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

    for (const QString &xdgDir : xdgDirString.split(QLatin1Char(':'))) {
        const QFileInfo xdgIconsDir(xdgDir + QStringLiteral("/icons"));
        if (xdgIconsDir.isDir()) {
            paths << xdgIconsDir.absoluteFilePath();
        }
    }

    return paths;
}

QString GnomeHintsSettings::kvantumThemeForGtkTheme() const
{
    if (m_gtkTheme.isEmpty()) {
        // No Gtk theme? Then can't match to Kvantum!
        return QString();
    }

    QString gtkName = m_gtkTheme;
    QStringList dirs = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);

    // Look for a matching KVantum config file in the theme's folder
    for (const QString &dir : dirs) {
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

    for (const QString &name : names) {
        for (const QString &dir : dirs) {
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
