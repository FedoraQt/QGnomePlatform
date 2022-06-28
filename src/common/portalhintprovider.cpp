/*
 * Copyright (C) 2016-2022 Jan Grulich
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

const QDBusArgument
&operator>>(const QDBusArgument &argument, QMap<QString, QVariantMap> &map)
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

PortalHintProvider::PortalHintProvider(QObject *parent)
    : HintProvider(parent)
{
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

    QDBusConnection::sessionBus().connect(QString(),
                                          QStringLiteral("/org/freedesktop/portal/desktop"),
                                          QStringLiteral("org.freedesktop.portal.Settings"),
                                          QStringLiteral("SettingChanged"),
                                          this,
                                          SLOT(settingChanged(QString, QString, QDBusVariant)));
}
