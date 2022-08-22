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

#include "gnomesettings.h"
#include "hintprovider.h"
#include "gsettingshintprovider.h"
#include "portalhintprovider.h"

#if QT_VERSION >= 0x060000
#include <AdwaitaQt6/adwaitacolors.h>
#else
#include <AdwaitaQt/adwaitacolors.h>
#endif

// QtCore
#include <QDir>
#include <QLoggingCategory>
#include <QSettings>
#include <QStandardPaths>
#include <QString>
#include <QTimer>
#include <QVariant>

// QtDbus
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusPendingReply>
#include <QDBusPendingCallWatcher>
#include <QDBusReply>
#include <QDBusServiceWatcher>

// QtGui
#include <QApplication>
#include <QFont>
#include <QMainWindow>
#include <QPalette>
#include <QToolBar>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

Q_GLOBAL_STATIC(GnomeSettings, gnomeSettingsGlobal)
Q_LOGGING_CATEGORY(QGnomePlatform, "qt.qpa.qgnomeplatform")

static inline bool checkSandboxApplication()
{
    return !QStandardPaths::locate(QStandardPaths::RuntimeLocation, QStringLiteral("flatpak-info")).isEmpty() || qEnvironmentVariableIsSet("SNAP");
}

GnomeSettings &GnomeSettings::getInstance()
{
    return *gnomeSettingsGlobal;
}

GnomeSettings::GnomeSettings(QObject *parent)
    : QObject(parent)
    , m_fallbackFont(new QFont(QLatin1String("Sans"), 10))
    , m_isRunningInSandbox(checkSandboxApplication())
    , m_canUseFileChooserPortal(!m_isRunningInSandbox)
{
    gtk_init();

    if (m_isRunningInSandbox) {
        qCDebug(QGnomePlatform) << "Using xdg-desktop-portal backend";
        m_hintProvider = std::make_unique<PortalHintProvider>(this);
    } else if (qgetenv("XDG_CURRENT_DESKTOP").toLower() == QStringLiteral("x-cinnamon")) {
        qCDebug(QGnomePlatform) << "Using GSettings backend";
        m_hintProvider = std::make_unique<GSettingsHintProvider>(this);
    } else {
        // check if service already exists on QGnomePlatform initialization
        QDBusConnectionInterface *interface = QDBusConnection::sessionBus().interface();
        const bool dbusServiceExists = interface && interface->isServiceRegistered(QString::fromLatin1("org.freedesktop.impl.portal.desktop.gnome"));

        if (dbusServiceExists) {
            qCDebug(QGnomePlatform) << "Using xdg-desktop-portal backend";
            m_hintProvider = std::make_unique<PortalHintProvider>(this);
        } else {
            qCDebug(QGnomePlatform) << "Using GSettings backend";
            m_hintProvider = std::make_unique<GSettingsHintProvider>(this);
        }

        // to switch between backends on runtime
        QDBusServiceWatcher *watcher = new QDBusServiceWatcher(this);
        watcher->setConnection(QDBusConnection::sessionBus());
        watcher->setWatchMode(QDBusServiceWatcher::WatchForOwnerChange);
        watcher->addWatchedService(QString::fromLatin1("org.freedesktop.portal.Desktop"));
        connect(watcher, &QDBusServiceWatcher::serviceOwnerChanged, this, [=] (const QString &service, const QString &oldOwner, const QString &newOwner) {
            Q_UNUSED(service)

            if (newOwner.isEmpty()) {
                qCDebug(QGnomePlatform) << "Portal service disappeared. Switching to GSettings backend";
                m_hintProvider = std::make_unique<GSettingsHintProvider>(this);
                onHintProviderChanged();
            } else if (oldOwner.isEmpty()) {
                qCDebug(QGnomePlatform) << "Portal service appeared. Switching xdg-desktop-portal backend";
                PortalHintProvider *provider = new PortalHintProvider(this, true);
                connect(provider, &PortalHintProvider::settingsRecieved, this, [=]() {
                    m_hintProvider.reset(provider);
                    onHintProviderChanged();
                });
            }
        });
    }

    initializeHintProvider();

    // Initialize some cursor env variables needed by QtWayland
    onCursorSizeChanged();
    onCursorThemeChanged();

    loadPalette();

    if (m_canUseFileChooserPortal) {
        QTimer::singleShot(0, this, [this]() {
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
            QObject::connect(watcher, &QDBusPendingCallWatcher::finished, [this](QDBusPendingCallWatcher *watcher) {
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

GnomeSettings::~GnomeSettings()
{
    delete m_fallbackFont;
    delete m_palette;
}

void GnomeSettings::initializeHintProvider() const
{
    connect(m_hintProvider.get(), &HintProvider::cursorBlinkTimeChanged, this, &GnomeSettings::onCursorBlinkTimeChanged);
    connect(m_hintProvider.get(), &HintProvider::cursorSizeChanged, this, &GnomeSettings::onCursorSizeChanged);
    connect(m_hintProvider.get(), &HintProvider::cursorThemeChanged, this, &GnomeSettings::onCursorThemeChanged);
    connect(m_hintProvider.get(), &HintProvider::fontChanged, this, &GnomeSettings::onFontChanged);
    connect(m_hintProvider.get(), &HintProvider::iconThemeChanged, this, &GnomeSettings::onIconThemeChanged);
    connect(m_hintProvider.get(), &HintProvider::titlebarChanged, this, &GnomeSettings::titlebarChanged);
    connect(m_hintProvider.get(), &HintProvider::themeChanged, this, &GnomeSettings::loadPalette);
    connect(m_hintProvider.get(), &HintProvider::themeChanged, this, &GnomeSettings::themeChanged);
    connect(m_hintProvider.get(), &HintProvider::themeChanged, this, &GnomeSettings::onThemeChanged);
}

QFont *GnomeSettings::font(QPlatformTheme::Font type) const
{
    auto fonts = m_hintProvider->fonts();

    if (fonts.contains(type)) {
        return fonts[type];
    } else if (fonts.contains(QPlatformTheme::SystemFont)) {
        return fonts[QPlatformTheme::SystemFont];
    } else {
        // GTK default font
        return m_fallbackFont;
    }
}

QPalette *GnomeSettings::palette() const
{
    return m_palette;
}

bool GnomeSettings::canUseFileChooserPortal() const
{
    return m_canUseFileChooserPortal;
}

bool GnomeSettings::useGtkThemeDarkVariant() const
{
    const QString theme = m_hintProvider->gtkTheme();

    if (m_hintProvider->canRelyOnAppearance()) {
        return m_hintProvider->appearance() == PreferDark;
    }

    return theme.toLower().contains("-dark") ||
           theme.toLower().endsWith("inverse") ||
           m_hintProvider->appearance() == PreferDark;
}

bool GnomeSettings::useGtkThemeHighContrastVariant() const
{
    const QString theme = m_hintProvider->gtkTheme();
    return theme.toLower().startsWith("highcontrast");
}

QString GnomeSettings::gtkTheme() const
{
    return m_hintProvider->gtkTheme();
}

QVariant GnomeSettings::hint(QPlatformTheme::ThemeHint hint) const
{
    if (hint == QPlatformTheme::StyleNames) {
        return styleNames();
    } else if (hint == QPlatformTheme::IconThemeSearchPaths) {
        return xdgIconThemePaths();
    }

    return m_hintProvider->hints()[hint];
}

GnomeSettings::TitlebarButtons GnomeSettings::titlebarButtons() const
{
    return m_hintProvider->titlebarButtons();
}

GnomeSettings::TitlebarButtonsPlacement GnomeSettings::titlebarButtonPlacement() const
{
    return m_hintProvider->titlebarButtonPlacement();
}

void GnomeSettings::loadPalette()
{
    if (useGtkThemeHighContrastVariant()) {
        m_palette = new QPalette(
            Adwaita::Colors::palette(useGtkThemeDarkVariant() ? Adwaita::ColorVariant::AdwaitaHighcontrastInverse : Adwaita::ColorVariant::AdwaitaHighcontrast));
    } else {
        m_palette = new QPalette(Adwaita::Colors::palette(useGtkThemeDarkVariant() ? Adwaita::ColorVariant::AdwaitaDark : Adwaita::ColorVariant::Adwaita));
    }
}

void GnomeSettings::onCursorBlinkTimeChanged()
{
    // If we are not a QApplication, means that we are a QGuiApplication, then we do nothing.
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

void GnomeSettings::onCursorSizeChanged()
{
    if (QGuiApplication::platformName() != QStringLiteral("xcb")) {
        qputenv("XCURSOR_SIZE", QString::number(m_hintProvider->cursorSize()).toUtf8());
    }
}

void GnomeSettings::onCursorThemeChanged()
{
    if (QGuiApplication::platformName() != QStringLiteral("xcb")) {
        qputenv("XCURSOR_THEME", m_hintProvider->cursorTheme().toUtf8());
    }
}

void GnomeSettings::onFontChanged()
{
    if (qobject_cast<QApplication *>(QCoreApplication::instance())) {
        QApplication::setFont(*m_hintProvider->fonts()[QPlatformTheme::SystemFont]);
        QWidgetList widgets = QApplication::allWidgets();
        for (QWidget *widget : widgets) {
            widget->setFont(*m_hintProvider->fonts()[QPlatformTheme::SystemFont]);
        }
    } else {
        QGuiApplication::setFont(*m_hintProvider->fonts()[QPlatformTheme::SystemFont]);
    }
}

void GnomeSettings::onIconThemeChanged()
{
    // If we are not a QApplication, means that we are a QGuiApplication, then we do nothing.
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

void GnomeSettings::onThemeChanged()
{
    QApplication *app = qobject_cast<QApplication *>(QCoreApplication::instance());
    if (!app) {
        return;
    }

    app->setStyle(styleNames().first());
}

void GnomeSettings::onHintProviderChanged()
{
    initializeHintProvider();

    // Reload some configuration as switching backends we might get different configuration
    loadPalette();
    onThemeChanged();
    // Also notify to update decorations
    Q_EMIT themeChanged();
}

QStringList GnomeSettings::styleNames() const
{
    QStringList styleNames;

    // QT_STYLE_OVERRIDE should always have highest priority
    QString styleOverride;
    if (qEnvironmentVariableIsSet("QT_STYLE_OVERRIDE")) {
        styleOverride = QString::fromLocal8Bit(qgetenv("QT_STYLE_OVERRIDE"));
        styleNames << styleOverride;
    }

    bool isDarkTheme = false;
    const bool preferDarkTheme = m_hintProvider->appearance() == Appearance::PreferDark;
    const QString gtkTheme = m_hintProvider->gtkTheme();

    if (gtkTheme.toLower().contains("-dark") || gtkTheme.toLower().endsWith("inverse")) {
        isDarkTheme = true;
    }

    // 2) Use GTK theme
    if (!gtkTheme.isEmpty()) {
        const QStringList adwaitaStyles = { QStringLiteral("adwaita"), QStringLiteral("adwaita-dark"), QStringLiteral("highcontrast"), QStringLiteral("highcontrastinverse") };
        if (adwaitaStyles.contains(gtkTheme.toLower())) {
            QString theme = gtkTheme;

            if (m_hintProvider->canRelyOnAppearance()) {
                if (gtkTheme.toLower().contains(QStringLiteral("adwaita"))) {
                    theme = preferDarkTheme ? QStringLiteral("adwaita-dark") : QStringLiteral("adwaita");
                } else if (gtkTheme.toLower().contains(QStringLiteral("highcontrast"))) {
                    theme = preferDarkTheme ? QStringLiteral("highcontrastinverse") : QStringLiteral("highcontrast");
                }
            }

            styleNames << theme;
        }
    }

    // 3) Use Kvantum
    // Detect if we have a Kvantum theme for this Gtk theme
    QString kvTheme = kvantumThemeForGtkTheme();

    if (!kvTheme.isEmpty()) {
        // Found matching Kvantum theme, configure user's Kvantum setting to use this
        configureKvantum(kvTheme);

        if (isDarkTheme || preferDarkTheme) {
            styleNames << QStringLiteral("kvantum-dark");
        }
        styleNames << QStringLiteral("kvantum");
    }

    // 4) Use light/dark adwaita as fallback
    if (isDarkTheme || preferDarkTheme) {
        styleNames << QStringLiteral("adwaita-dark");
    } else {
        styleNames << QStringLiteral("adwaita");
    }

    // 5) Use other styles
    styleNames << QStringLiteral("fusion")
               << QStringLiteral("windows");

    return styleNames;
}

QStringList GnomeSettings::xdgIconThemePaths() const
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

QString GnomeSettings::kvantumThemeForGtkTheme() const
{
    if (m_hintProvider->gtkTheme().isEmpty()) {
        // No Gtk theme? Then can't match to Kvantum!
        return QString();
    }

    QString gtkName = m_hintProvider->gtkTheme();
    QStringList dirs = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);

    // Look for a matching KVantum config file in the theme's folder
    for (const QString &dir : dirs) {
        if (QFile::exists(QStringLiteral("%1/themes/%2/Kvantum/%3.kvconfig").arg(dir).arg(gtkName).arg(gtkName))) {
            return gtkName;
        }
    }

    // No config found in theme folder, look for a Kv<Theme> as shipped as part of Kvantum itself
    // (Kvantum ships KvAdapta, KvAmbiance, KvArc, etc.
    QStringList names{QStringLiteral("Kv") + gtkName};

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

void GnomeSettings::configureKvantum(const QString &theme) const
{
    QSettings config(QDir::homePath() + "/.config/Kvantum/kvantum.kvconfig", QSettings::NativeFormat);
    if (!config.contains("theme") || config.value("theme").toString() != theme) {
        config.setValue("theme", theme);
    }
}
