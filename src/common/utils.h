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

#ifndef UTILS_H
#define UTILS_H

#include "gnomesettings.h"

class QFont;
class QString;

namespace Utils
{
QFont *qt_fontFromString(const QString &name);
GnomeSettings::TitlebarButtons titlebarButtonsFromString(const QString &layout);
GnomeSettings::TitlebarButtonsPlacement titlebarButtonPlacementFromString(const QString &layout);
}

#endif // UTILS_H
