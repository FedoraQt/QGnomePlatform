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

#ifndef PORTAL_HINT_PROVIDER_H
#define PORTAL_HINT_PROVIDER_H

#include "hintprovider.h"

class QDBusVariant;
class QFont;
class QString;
class QVariant;

class PortalHintProvider : public HintProvider
{
    Q_OBJECT
public:
    explicit PortalHintProvider(QObject *parent = nullptr);
    virtual ~PortalHintProvider() = default;

private Q_SLOTS:
    void settingChanged(const QString &group, const QString &key, const QDBusVariant &value);

private:
    void loadCursorBlinkTime();
    void loadCursorSize();
    void loadCursorTheme();
    void loadIconTheme();
    void loadFonts();
    void loadTheme();
    void loadTitlebar();
    void loadStaticHints();

    QMap<QString, QVariantMap> m_portalSettings;

};

#endif // PORTAL_HINT_PROVIDER_H


