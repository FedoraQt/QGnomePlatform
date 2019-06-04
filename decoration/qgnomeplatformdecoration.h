/*
 * Copyright (C) 2019 Jan Grulich <jgrulich@redhat.com>
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

#ifndef QGNOMEPLATFORMDECORATION_H
#define QGNOMEPLATFORMDECORATION_H

#include <QtWaylandClient/private/qwaylandabstractdecoration_p.h>

class GnomeHintsSettings;
class QPixmap;

using namespace QtWaylandClient;

enum Button
{
    None,
    Close,
    Maximize,
    Minimize,
    Restore
};

class QGnomePlatformDecoration : public QWaylandAbstractDecoration
{
public:
    QGnomePlatformDecoration();
    ~QGnomePlatformDecoration();
protected:
    QMargins margins() const override;
    void paint(QPaintDevice *device) override;
    bool handleMouse(QWaylandInputDevice *inputDevice, const QPointF &local, const QPointF &global,Qt::MouseButtons b,Qt::KeyboardModifiers mods) override;
    bool handleTouch(QWaylandInputDevice *inputDevice, const QPointF &local, const QPointF &global, Qt::TouchPointState state, Qt::KeyboardModifiers mods) override;
private:
    void initializeButtonPixmaps();
    void initializeColors();
    QPixmap pixmapDarkVariant(const QPixmap &pixmap);

    void processMouseTop(QWaylandInputDevice *inputDevice, const QPointF &local, Qt::MouseButtons b,Qt::KeyboardModifiers mods);
    void processMouseBottom(QWaylandInputDevice *inputDevice, const QPointF &local, Qt::MouseButtons b,Qt::KeyboardModifiers mods);
    void processMouseLeft(QWaylandInputDevice *inputDevice, const QPointF &local, Qt::MouseButtons b,Qt::KeyboardModifiers mods);
    void processMouseRight(QWaylandInputDevice *inputDevice, const QPointF &local, Qt::MouseButtons b,Qt::KeyboardModifiers mods);
    bool clickButton(Qt::MouseButtons b, Button btn);

    QRectF closeButtonRect() const;
    QRectF maximizeButtonRect() const;
    QRectF minimizeButtonRect() const;

    // Colors
    QColor m_backgroundColorStart;
    QColor m_backgroundColorEnd;
    QColor m_backgroundInactiveColor;
    QColor m_borderColor;
    QColor m_borderInactiveColor;
    QColor m_foregroundColor;
    QColor m_foregroundInactiveColor;

    // Buttons
    QHash<Button, QPixmap> m_buttonPixmaps;

    QStaticText m_windowTitle;
    Button m_clicking = None;

    GnomeHintsSettings *m_hints;
};


#endif // QGNOMEPLATFORMDECORATION_H
