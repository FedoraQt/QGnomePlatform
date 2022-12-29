/*
 * Copyright (C) 2019-2022 Jan Grulich <jgrulich@redhat.com>
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

#if QT_VERSION >= 0x060000
#include <AdwaitaQt6/adwaitacolors.h>
#include <AdwaitaQt6/adwaitarenderer.h>
#else
#include <AdwaitaQt/adwaitacolors.h>
#include <AdwaitaQt/adwaitarenderer.h>
#endif

#include <QtGlobal>

#include <QDateTime>
#include <QPixmap>

using namespace QtWaylandClient;

enum Button { None, Close, Maximize, Minimize, Restore };

class QGnomePlatformDecoration : public QWaylandAbstractDecoration
{
public:
    QGnomePlatformDecoration();
    virtual ~QGnomePlatformDecoration() override = default;

protected:
#ifdef DECORATION_SHADOWS_SUPPORT // Qt 6.2.0+ or patched QtWayland
    QMargins margins(MarginsType marginsType = Full) const override;
#else
    QMargins margins() const override;
#endif
    void paint(QPaintDevice *device) override;
    bool handleMouse(QWaylandInputDevice *inputDevice, const QPointF &local, const QPointF &global, Qt::MouseButtons b, Qt::KeyboardModifiers mods) override;
#if QT_VERSION >= 0x060000
    bool
    handleTouch(QWaylandInputDevice *inputDevice, const QPointF &local, const QPointF &global, QEventPoint::State state, Qt::KeyboardModifiers mods) override;
#else
    bool
    handleTouch(QWaylandInputDevice *inputDevice, const QPointF &local, const QPointF &global, Qt::TouchPointState state, Qt::KeyboardModifiers mods) override;
#endif

protected slots:
    void forceWindowActivation();

private:
    QRect windowContentGeometry() const;

    void forceRepaint();
    void loadConfiguration();

    void processMouseTop(QWaylandInputDevice *inputDevice, const QPointF &local, Qt::MouseButtons b, Qt::KeyboardModifiers mods);
    void processMouseBottom(QWaylandInputDevice *inputDevice, const QPointF &local, Qt::MouseButtons b, Qt::KeyboardModifiers mods);
    void processMouseLeft(QWaylandInputDevice *inputDevice, const QPointF &local, Qt::MouseButtons b, Qt::KeyboardModifiers mods);
    void processMouseRight(QWaylandInputDevice *inputDevice, const QPointF &local, Qt::MouseButtons b, Qt::KeyboardModifiers mods);
    void renderButton(QPainter *painter, const QRectF &rect, Adwaita::ButtonType button, bool renderFrame, bool sunken);

    bool clickButton(Qt::MouseButtons b, Button btn);
    bool doubleClickButton(Qt::MouseButtons b, const QPointF &local, const QDateTime &currentTime);
    bool updateButtonHoverState(Button hoveredButton);

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
    bool m_closeButtonHovered;
    bool m_maximizeButtonHovered;
    bool m_minimizeButtonHovered;

    // For double-click support
    QDateTime m_lastButtonClick;
    QPointF m_lastButtonClickPosition;
    Button m_doubleClicking = None;

    QStaticText m_windowTitle;
    Button m_clicking = None;

    // Shadows
    QPixmap m_shadowPixmap;

    Adwaita::ColorVariant m_adwaitaVariant;
};

#endif // QGNOMEPLATFORMDECORATION_H
