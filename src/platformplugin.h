/*
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

#ifndef PLATFORMPLUGIN_H
#define PLATFORMPLUGIN_H

#include <qpa/qplatformtheme.h>
#include <qpa/qplatformthemeplugin.h>

#include "qgnomeplatformtheme.h"

class QGnomePlatformThemePlugin : public QPlatformThemePlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QPA.QPlatformThemeFactoryInterface.5.1" FILE "qgnomeplatform.json")
public:
    QGnomePlatformThemePlugin(QObject *parent = 0);

    virtual QPlatformTheme *create(const QString &key, const QStringList &paramList);
};

#endif // PLATFORMPLUGIN_H
