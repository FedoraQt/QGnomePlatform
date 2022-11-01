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

#include "portalhintprovider.h"

// QtDBus
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusVariant>
#include <QLoggingCategory>
#include <QVariant>
#include <QtDBus/QtDBus>

Q_LOGGING_CATEGORY(QGnomePlatformPortalHintProvider, "qt.qpa.qgnomeplatform.portalhintprovider")

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

PortalHintProvider::PortalHintProvider(QObject *parent, bool asynchronous)
    : HintProvider(parent)
{
    QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.portal.Desktop"),
                                                          QStringLiteral("/org/freedesktop/portal/desktop"),
                                                          QStringLiteral("org.freedesktop.portal.Settings"),
                                                          QStringLiteral("ReadAll"));
    message << QStringList{{QStringLiteral("org.gnome.desktop.interface")},
                           {QStringLiteral("org.gnome.desktop.wm.preferences")},
                           {QStringLiteral("org.freedesktop.appearance")}};

    qCDebug(QGnomePlatformPortalHintProvider) << "Reading settings from xdg-desktop-portal";
    if (asynchronous) {
        qDBusRegisterMetaType<QMap<QString, QVariantMap>>();
        QDBusPendingCall pendingCall = QDBusConnection::sessionBus().asyncCall(message);
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall);
        QObject::connect(watcher, &QDBusPendingCallWatcher::finished, [=](QDBusPendingCallWatcher *watcher) {
            QDBusPendingReply<QMap<QString, QVariantMap>> reply = *watcher;
            if (reply.isValid()) {
                m_portalSettings = reply.value();
                onSettingsReceived();
                Q_EMIT settingsRecieved();
                watcher->deleteLater();
            }
        });
    } else {
        QDBusMessage resultMessage = QDBusConnection::sessionBus().call(message);
        qCDebug(QGnomePlatformPortalHintProvider) << "Received settings from xdg-desktop-portal";

        if (resultMessage.type() == QDBusMessage::ReplyMessage) {
            QDBusArgument dbusArgument = resultMessage.arguments().at(0).value<QDBusArgument>();
            dbusArgument >> m_portalSettings;
            onSettingsReceived();
        }
    }

    QDBusConnection::sessionBus().connect(QString(),
                                          QStringLiteral("/org/freedesktop/portal/desktop"),
                                          QStringLiteral("org.freedesktop.portal.Settings"),
                                          QStringLiteral("SettingChanged"),
                                          this,
                                          SLOT(settingChanged(QString, QString, QDBusVariant)));
}

void PortalHintProvider::onSettingsReceived()
{
    if (m_portalSettings.contains(QStringLiteral("org.freedesktop.appearance"))) {
        m_canRelyOnAppearance = true;
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
void PortalHintProvider::settingChanged(const QString &group, const QString &key, const QDBusVariant &value)
{
    qCDebug(QGnomePlatformPortalHintProvider) << "Setting property change: " << group << " : " << key;
    m_portalSettings[group][key] = value.variant();

    if (key == QStringLiteral("gtk-theme") || key == QStringLiteral("color-scheme")) {
        loadTheme();
        Q_EMIT themeChanged();
    } else if (key == QStringLiteral("icon-theme")) {
        loadIconTheme();
        Q_EMIT iconThemeChanged();
    } else if (key == QStringLiteral("cursor-blink-time")) {
        loadCursorBlinkTime();
        Q_EMIT cursorBlinkTimeChanged();
    } else if (key == QStringLiteral("font-name") || key == QStringLiteral("monospace-font-name") || key == QStringLiteral("titlebar-font")) {
        loadFonts();
        Q_EMIT fontChanged();
    } else if (key == QStringLiteral("cursor-size")) {
        loadCursorSize();
        ;
        Q_EMIT fontChanged();
    } else if (key == QStringLiteral("cursor-theme")) {
        loadCursorTheme();
        Q_EMIT cursorThemeChanged();
    } else if (key == QStringLiteral("button-layout")) {
        loadTitlebar();
        Q_EMIT titlebarChanged();
    }
}

void PortalHintProvider::loadCursorBlinkTime()
{
    const int cursorBlinkTime = m_portalSettings.value(QStringLiteral("org.gnome.desktop.interface")).value(QStringLiteral("cursor-blink-time")).toInt();
    setCursorBlinkTime(cursorBlinkTime);
}

void PortalHintProvider::loadCursorSize()
{
    const int cursorSize = m_portalSettings.value(QStringLiteral("org.gnome.desktop.interface")).value(QStringLiteral("cursor-size")).toInt();
    setCursorSize(cursorSize);
}

void PortalHintProvider::loadCursorTheme()
{
    const QString cursorTheme = m_portalSettings.value(QStringLiteral("org.gnome.desktop.interface")).value(QStringLiteral("cursor-theme")).toString();
    setCursorTheme(cursorTheme);
}

void PortalHintProvider::loadIconTheme()
{
    const QString systemIconTheme = m_portalSettings.value(QStringLiteral("org.gnome.desktop.interface")).value(QStringLiteral("icon-theme")).toString();
    setIconTheme(systemIconTheme);
}

void PortalHintProvider::loadFonts()
{
    const QString fontName = m_portalSettings.value(QStringLiteral("org.gnome.desktop.interface")).value(QStringLiteral("font-name")).toString();
    const QString monospaceFontName =
        m_portalSettings.value(QStringLiteral("org.gnome.desktop.interface")).value(QStringLiteral("monospace-font-name")).toString();
    const QString titlebarFontName =
        m_portalSettings.value(QStringLiteral("org.gnome.desktop.wm.preferences")).value(QStringLiteral("titlebar-font")).toString();
    setFonts(fontName, monospaceFontName, titlebarFontName);
}

void PortalHintProvider::loadTitlebar()
{
    const QString buttonLayout = m_portalSettings.value(QStringLiteral("org.gnome.desktop.wm.preferences")).value(QStringLiteral("button-layout")).toString();
    setTitlebar(buttonLayout);
}

void PortalHintProvider::loadTheme()
{
    const QString theme = m_portalSettings.value(QStringLiteral("org.gnome.desktop.interface")).value(QStringLiteral("gtk-theme")).toString();
    const GnomeSettings::Appearance appearance = static_cast<GnomeSettings::Appearance>(
        m_portalSettings.value(QStringLiteral("org.freedesktop.appearance")).value(QStringLiteral("color-scheme")).toUInt());
    setTheme(theme, appearance);
}

void PortalHintProvider::loadStaticHints()
{
    int doubleClickTime = 400;
    int longPressTime = 500;
    int doubleClickDistance = 5;
    int startDragDistance = 8;
    uint passwordMaskDelay = 0;

    setStaticHints(doubleClickTime, longPressTime, doubleClickDistance, startDragDistance, passwordMaskDelay);
}
