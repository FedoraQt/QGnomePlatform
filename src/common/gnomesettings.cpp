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

#include "gnomesettings.h"
#include "gnomesettings_p.h"

#include <AdwaitaQt/adwaitacolors.h>

// QtCore
#include <QDir>
#include <QLoggingCategory>
#include <QSettings>
#include <QString>
#include <QStandardPaths>
#include <QVariant>
#include <QTimer>

// QtDBus
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusVariant>

// QtGui
#include <QApplication>
#include <QDialogButtonBox>
#include <QGuiApplication>
#include <QFont>
#include <QMainWindow>
#include <QPalette>
#include <QStyleFactory>
#include <QToolBar>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

Q_GLOBAL_STATIC(GnomeSettingsPrivate, gnomeSettingsGlobal)

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

GnomeSettings::GnomeSettings(QObject *parent)
    : QObject(parent)
{
}

QFont * GnomeSettings::font(QPlatformTheme::Font type)
{
    return gnomeSettingsGlobal->font(type);
}

QPalette * GnomeSettings::palette()
{
    return gnomeSettingsGlobal->palette();
}

bool GnomeSettings::canUseFileChooserPortal()
{
    return gnomeSettingsGlobal->canUseFileChooserPortal();
}

bool GnomeSettings::isGtkThemeDarkVariant()
{
    return gnomeSettingsGlobal->isGtkThemeDarkVariant();
}

bool GnomeSettings::isGtkThemeHighContrastVariant()
{
    return gnomeSettingsGlobal->isGtkThemeHighContrastVariant();
}

QString GnomeSettings::gtkTheme()
{
    return gnomeSettingsGlobal->gtkTheme();
}

QVariant GnomeSettings::hint(QPlatformTheme::ThemeHint hint)
{
    return gnomeSettingsGlobal->hint(hint);
}

GnomeSettings::TitlebarButtons GnomeSettings::titlebarButtons()
{
    return gnomeSettingsGlobal->titlebarButtons();
}

GnomeSettings::TitlebarButtonsPlacement GnomeSettings::titlebarButtonPlacement()
{
    return gnomeSettingsGlobal->titlebarButtonPlacement();
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

GnomeSettingsPrivate::GnomeSettingsPrivate(QObject *parent)
    : GnomeSettings(parent)
    , m_usePortal(checkUsePortalSupport())
    , m_canUseFileChooserPortal(!m_usePortal)
    , m_gnomeDesktopSettings(g_settings_new("org.gnome.desktop.wm.preferences"))
    , m_settings(g_settings_new("org.gnome.desktop.interface"))
    , m_fallbackFont(new QFont(QLatin1String("Sans"), 10))
{
    gtk_init(nullptr, nullptr);

    // Set log handler to suppress false GtkDialog warnings
    g_log_set_handler("Gtk", G_LOG_LEVEL_MESSAGE, gtkMessageHandler, nullptr);

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

    if (QGuiApplication::platformName() != QStringLiteral("xcb")) {
        cursorSizeChanged();
    }

    loadFonts();
    loadStaticHints();
    loadTheme();
    loadTitlebar();

    if (m_gtkThemeHighContrastVariant) {
        m_palette = new QPalette(Adwaita::Colors::palette(m_gtkThemeDarkVariant ? Adwaita::ColorVariant::AdwaitaHighcontrastInverse : Adwaita::ColorVariant::AdwaitaHighcontrast));
    } else {
        m_palette = new QPalette(Adwaita::Colors::palette(m_gtkThemeDarkVariant ? Adwaita::ColorVariant::AdwaitaDark : Adwaita::ColorVariant::Adwaita));
    }

    if (m_canUseFileChooserPortal) {
        QTimer::singleShot(0, this, [this] () {
            const QString filePath = QStringLiteral("/proc/%1/root").arg(QCoreApplication::applicationPid());
            struct stat info;
            if (lstat(filePath.toStdString().c_str(), &info) == 0) {
                if (!static_cast<int>(info.st_uid)) {
                    m_canUseFileChooserPortal = false;
                }
            } else {
                // Do not use FileChooser portal if we fail to get information about the file
                m_canUseFileChooserPortal = false;
            }
        });

        if (m_canUseFileChooserPortal) {
            // Get information about portal version
            QDBusMessage message = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.portal.Desktop"),
                                                                  QLatin1String("/org/freedesktop/portal/desktop"),
                                                                  QLatin1String("org.freedesktop.DBus.Properties"),
                                                                  QLatin1String("Get"));
            message << QLatin1String("org.freedesktop.portal.FileChooser") << QLatin1String("version");
            QDBusPendingCall pendingCall = QDBusConnection::sessionBus().asyncCall(message);
            QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall);
            QObject::connect(watcher, &QDBusPendingCallWatcher::finished, [this] (QDBusPendingCallWatcher *watcher) {
                QDBusPendingReply<QVariant> reply = *watcher;
                if (reply.isValid()) {
                    uint fileChooserPortalVersion = reply.value().toUInt();
                    if (fileChooserPortalVersion < 3) {
                        m_canUseFileChooserPortal = false;
                    }
                } else {
                    m_canUseFileChooserPortal = false;
                }
                watcher->deleteLater();
            });
        }
    }
}

GnomeSettingsPrivate::~GnomeSettingsPrivate()
{
    qDeleteAll(m_fonts);
    delete m_fallbackFont;
    delete m_palette;
    if (m_cinnamonSettings) {
        g_object_unref(m_cinnamonSettings);
    }
    g_object_unref(m_gnomeDesktopSettings);
    g_object_unref(m_settings);
}

QFont * GnomeSettingsPrivate::font(QPlatformTheme::Font type) const
{
    if (m_fonts.contains(type)) {
        return m_fonts[type];
    } else if (m_fonts.contains(QPlatformTheme::SystemFont)) {
        return m_fonts[QPlatformTheme::SystemFont];
    } else {
        // GTK default font
        return m_fallbackFont;
    }
}

QPalette * GnomeSettingsPrivate::palette() const
{
    return m_palette;
}

bool GnomeSettingsPrivate::canUseFileChooserPortal() const
{
    return m_canUseFileChooserPortal;
}

bool GnomeSettingsPrivate::isGtkThemeDarkVariant() const
{
    return m_gtkThemeDarkVariant;
}

bool GnomeSettingsPrivate::isGtkThemeHighContrastVariant() const
{
    return m_gtkThemeHighContrastVariant;
}

QString GnomeSettingsPrivate::gtkTheme() const
{
    return QString(m_gtkTheme);
}

QVariant GnomeSettingsPrivate::hint(QPlatformTheme::ThemeHint hint) const
{
    return m_hints[hint];
}

GnomeSettings::TitlebarButtons GnomeSettingsPrivate::titlebarButtons() const
{
    return m_titlebarButtons;
}

GnomeSettings::TitlebarButtonsPlacement GnomeSettingsPrivate::titlebarButtonPlacement() const
{
    return m_titlebarButtonPlacement;
}

void GnomeSettingsPrivate::gsettingPropertyChanged(GSettings *settings, gchar *key, GnomeSettingsPrivate *gnomeSettings)
{
    Q_UNUSED(settings)

    const QString changedProperty = key;

    // Org.gnome.desktop.interface
    if (changedProperty == QStringLiteral("gtk-theme")) {
        gnomeSettings->themeChanged();
    } else if (changedProperty == QStringLiteral("icon-theme")) {
        gnomeSettings->iconsChanged();
    } else if (changedProperty == QStringLiteral("cursor-blink-time")) {
        gnomeSettings->cursorBlinkTimeChanged();
    } else if (changedProperty == QStringLiteral("font-name")) {
        gnomeSettings->fontChanged();
    } else if (changedProperty == QStringLiteral("monospace-font-name")) {
        gnomeSettings->fontChanged();
    } else if (changedProperty == QStringLiteral("cursor-size")) {
        if (QGuiApplication::platformName() != QStringLiteral("xcb")) {
            gnomeSettings->cursorSizeChanged();
        }
    // Org.gnome.wm.preferences
    } else if (changedProperty == QStringLiteral("titlebar-font")) {
        gnomeSettings->fontChanged();
    } else if (changedProperty == QStringLiteral("button-layout")) {
        gnomeSettings->loadTitlebar();
    // Fallback
    } else {
        qCDebug(QGnomePlatform) << "GSetting property change: " << key;
    }
}

void GnomeSettingsPrivate::cursorBlinkTimeChanged()
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

void GnomeSettingsPrivate::cursorSizeChanged()
{
    int cursorSize = getSettingsProperty<int>(QStringLiteral("cursor-size"));
    qputenv("XCURSOR_SIZE", QString::number(cursorSize).toUtf8());
}

void GnomeSettingsPrivate::fontChanged()
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

void GnomeSettingsPrivate::iconsChanged()
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

void GnomeSettingsPrivate::themeChanged()
{
    loadTheme();
}

void GnomeSettingsPrivate::loadTitlebar()
{
    const QString buttonLayout = getSettingsProperty<QString>("button-layout");

    if (buttonLayout.isEmpty()) {
        return;
    }

    const QStringList btnList = buttonLayout.split(QLatin1Char(':'));
    if (btnList.count() == 2) {
        const QString &leftButtons = btnList.first();
        const QString &rightButtons = btnList.last();

        m_titlebarButtonPlacement = leftButtons.contains(QStringLiteral("close")) ? GnomeSettingsPrivate::LeftPlacement : GnomeSettingsPrivate::RightPlacement;

        // TODO support button order
        TitlebarButtons buttons;
        if (leftButtons.contains(QStringLiteral("close")) || rightButtons.contains("close")) {
            buttons = buttons | GnomeSettingsPrivate::CloseButton;
        }

        if (leftButtons.contains(QStringLiteral("maximize")) || rightButtons.contains("maximize")) {
            buttons = buttons | GnomeSettingsPrivate::MaximizeButton;
        }

        if (leftButtons.contains(QStringLiteral("minimize")) || rightButtons.contains("minimize")) {
            buttons = buttons | GnomeSettingsPrivate::MinimizeButton;
        }

        m_titlebarButtons = buttons;
    }
}

void GnomeSettingsPrivate::loadTheme()
{
    QString styleOverride;

    // g_object_get(gtk_settings_get_default(), "gtk-theme-name", &m_gtkTheme, NULL);
    m_gtkTheme = getSettingsProperty<QString>(QStringLiteral("gtk-theme"));
    g_object_get(gtk_settings_get_default(), "gtk-application-prefer-dark-theme", &m_gtkThemeDarkVariant, NULL);

    if (qEnvironmentVariableIsSet("QT_STYLE_OVERRIDE")) {
        styleOverride = QString::fromLocal8Bit(qgetenv("QT_STYLE_OVERRIDE"));
    }

    if (styleOverride.isEmpty()) {
        if (m_gtkTheme.isEmpty()) {
            qCWarning(QGnomePlatform) << "Couldn't get current gtk theme!";
        } else {
            qCDebug(QGnomePlatform) << "Theme name: " << m_gtkTheme;

            if (m_gtkTheme.toLower().startsWith("highcontrast")) {
                m_gtkThemeHighContrastVariant = true;
            }

            if (m_gtkTheme.toLower().contains("-dark") || m_gtkTheme.toLower().endsWith("inverse")) {
                m_gtkThemeDarkVariant = true;
            }

            qCDebug(QGnomePlatform) << "Dark version: " << (m_gtkThemeDarkVariant ? "yes" : "no");
        }
    } else {
        qCDebug(QGnomePlatform) << "Theme name: " << styleOverride;

        if (styleOverride.toLower().startsWith("highcontrast")) {
            m_gtkThemeHighContrastVariant = true;
        }

        if (styleOverride.toLower().contains("-dark") || styleOverride.toLower().endsWith("inverse")) {
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

void GnomeSettingsPrivate::loadFonts()
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
            QRegExp re("^([^,]+)[, \t]+([0-9]+)$");
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

void GnomeSettingsPrivate::loadStaticHints() {
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
    m_hints[QPlatformTheme::SystemIconFallbackThemeName] = "hicolor";
    m_hints[QPlatformTheme::IconThemeSearchPaths] = xdgIconThemePaths();
}

void GnomeSettingsPrivate::portalSettingChanged(const QString &group, const QString &key, const QDBusVariant &value)
{
    if (group == QStringLiteral("org.gnome.desktop.interface") || group == QStringLiteral("org.gnome.desktop.wm.preferences")) {
        m_portalSettings[group][key] = value.variant();
        gsettingPropertyChanged(nullptr, const_cast<gchar*>(key.toStdString().c_str()), this);
    }
}

QStringList GnomeSettingsPrivate::xdgIconThemePaths() const
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

QString GnomeSettingsPrivate::kvantumThemeForGtkTheme() const
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

void GnomeSettingsPrivate::configureKvantum(const QString &theme) const
{
    QSettings config(QDir::homePath() + "/.config/Kvantum/kvantum.kvconfig", QSettings::NativeFormat);
    if (!config.contains("theme") || config.value("theme").toString() != theme) {
        config.setValue("theme", theme);
    }
}

template <typename T>
T GnomeSettingsPrivate::getSettingsProperty(GSettings *settings, const QString &property, bool *ok) {
    Q_UNUSED(settings)
    Q_UNUSED(property)
    Q_UNUSED(ok)
    return {};
}

template <typename T>
T GnomeSettingsPrivate::getSettingsProperty(const QString &property, bool *ok) {
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
        if (!value.isNull() && value.canConvert<T>()) {
            return value.value<T>();
        }
        value = m_portalSettings.value(QStringLiteral("org.gnome.desktop.wm.preferences")).value(property);
        if (!value.isNull() && value.canConvert<T>()) {
            return value.value<T>();
        }
    }

    return getSettingsProperty<T>(settings, property, ok);
}


template <>
int GnomeSettingsPrivate::getSettingsProperty(GSettings *settings, const QString &property, bool *ok) {
    if (ok) {
        *ok = true;
    }
    return g_settings_get_int(settings, property.toStdString().c_str());
}

template <>
QString GnomeSettingsPrivate::getSettingsProperty(GSettings *settings, const QString &property, bool *ok) {
    // be exception and resources safe
    std::unique_ptr<gchar, void(*)(gpointer)> raw {g_settings_get_string(settings, property.toStdString().c_str()), g_free};
    if (ok) {
        *ok = !!raw;
    }
    return QString{raw.get()};
}

template <>
qreal GnomeSettingsPrivate::getSettingsProperty(GSettings *settings, const QString &property, bool *ok) {
    if (ok) {
        *ok = true;
    }
    return g_settings_get_double(settings, property.toStdString().c_str());
}
