/****************************************************************************
**
** Copyright (C) 2016 Robin Burchell <robin.burchell@viroteck.net>
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qgnomeplatformdecoration.h"

#include "gnomehintssettings.h"

#include <QtGui/QColor>
#include <QtGui/QCursor>
#include <QtGui/QPainter>
#include <QtGui/QPalette>
#include <QtGui/QLinearGradient>

#include <qpa/qwindowsysteminterface.h>

#include <QtWaylandClient/private/qwaylandwindow_p.h>
#include <QtWaylandClient/private/qwaylandshellsurface_p.h>

#define BUTTON_SPACING 5
#define BUTTON_WIDTH 18
#define BUTTONS_RIGHT_MARGIN 8

static QColor transparentize(const QColor &color, qreal amount = 0.1)
{
    qreal h, s, l, a;
    color.getHslF(&h, &s, &l, &a);

    qreal alpha = a - amount;
    if (alpha < 0)
        alpha = 0;
    return QColor::fromHslF(h, s, l, alpha);
}

QGnomePlatformDecoration::QGnomePlatformDecoration()
{
    m_hints = new GnomeHintsSettings;

    if (m_hints->gtkThemeDarkVariant()) {
        // Dark variant
        m_foregroundColor = QColor("#eeeeec"); // Adwaita fg_color
        m_backgroundColorStart = QColor("#262626"); // Adwaita GtkHeaderBar color
        m_backgroundColorEnd = QColor("#2b2b2b"); // Adwaita GtkHeaderBar color
        m_foregroundInactiveColor = QColor("#919190");
        m_backgroundInactiveColor = QColor("#353535");
        m_borderColor = transparentize(QColor("#1b1b1b"), 0.1);
        m_borderInactiveColor = transparentize(QColor("#1b1b1b"), 0.1);
    } else {
        // Light variant
        m_foregroundColor = QColor("#2e3436"); // Adwaita fg_color
        m_backgroundColorStart = QColor("#dad6d2"); // Adwaita GtkHeaderBar color
        m_backgroundColorEnd = QColor("#e1dedb"); // Adwaita GtkHeaderBar color
        m_foregroundInactiveColor = QColor("#929595");
        m_backgroundInactiveColor = QColor("#f6f5f4");
        m_borderColor = transparentize(QColor("black"), 0.77);
        m_borderInactiveColor = transparentize(QColor("black"), 0.82);
    }

    QTextOption option(Qt::AlignHCenter | Qt::AlignVCenter);
    option.setWrapMode(QTextOption::NoWrap);
    m_windowTitle.setTextOption(option);
}

QGnomePlatformDecoration::~QGnomePlatformDecoration()
{
    delete m_hints;
}

QRectF QGnomePlatformDecoration::closeButtonRect() const
{
    return QRectF(window()->frameGeometry().width() - BUTTON_WIDTH - BUTTON_SPACING * 0 - BUTTONS_RIGHT_MARGIN,
                  (margins().top() - BUTTON_WIDTH) / 2, BUTTON_WIDTH, BUTTON_WIDTH);
}

QRectF QGnomePlatformDecoration::maximizeButtonRect() const
{
    return QRectF(window()->frameGeometry().width() - BUTTON_WIDTH * 2 - BUTTON_SPACING * 1 - BUTTONS_RIGHT_MARGIN,
                  (margins().top() - BUTTON_WIDTH) / 2, BUTTON_WIDTH, BUTTON_WIDTH);
}

QRectF QGnomePlatformDecoration::minimizeButtonRect() const
{
    return QRectF(window()->frameGeometry().width() - BUTTON_WIDTH * 3 - BUTTON_SPACING * 2 - BUTTONS_RIGHT_MARGIN,
                  (margins().top() - BUTTON_WIDTH) / 2, BUTTON_WIDTH, BUTTON_WIDTH);
}

QMargins QGnomePlatformDecoration::margins() const
{
    return QMargins(1, 36, 1, 1);
}

void QGnomePlatformDecoration::paint(QPaintDevice *device)
{
    bool active = window()->handle()->isActive();
    QRect surfaceRect(QPoint(), window()->frameGeometry().size());

    QPainter p(device);
    p.setRenderHint(QPainter::Antialiasing);

    // Title bar (border)
    QPainterPath borderRect;
    borderRect.addRoundedRect(0, 0, surfaceRect.width(), margins().top() + 6, 10, 10);
    p.fillPath(borderRect.simplified(), active ? m_borderColor : m_borderInactiveColor);

    // Title bar
    QPainterPath roundedRect;
    roundedRect.addRoundedRect(1, 1, surfaceRect.width() - margins().left() - margins().right(), margins().top() + 6, 8, 8);
    QLinearGradient gradient(margins().left(), margins().top() + 6, margins().left(), 1);
    gradient.setColorAt(0, active ? m_backgroundColorStart : m_backgroundInactiveColor);
    gradient.setColorAt(1, active ? m_backgroundColorEnd : m_backgroundInactiveColor);
    p.fillPath(roundedRect.simplified(), gradient);

    QPainterPath borderPath;
    borderPath.addRect(0, margins().top(), margins().left(), surfaceRect.height() - margins().top());
    borderPath.addRect(0, surfaceRect.height() - margins().bottom(), surfaceRect.width(), margins().bottom());
    borderPath.addRect(surfaceRect.width() - margins().right(), margins().top(), margins().right(), surfaceRect.height() - margins().bottom());
    p.fillPath(borderPath, active ? m_borderColor : m_borderInactiveColor);


    QRect top = QRect(0, 0, surfaceRect.width(), margins().top());

    // Window title
    QString windowTitleText = window()->title();
    if (!windowTitleText.isEmpty()) {
        if (m_windowTitle.text() != windowTitleText) {
            m_windowTitle.setText(windowTitleText);
            m_windowTitle.prepare();
        }

        QRect titleBar = top;
        titleBar.setLeft(margins().left());
        titleBar.setRight(closeButtonRect().left() - 8);

        p.save();
        p.setClipRect(titleBar);
        p.setPen(active ? m_foregroundColor : m_foregroundInactiveColor);
        QSizeF size = m_windowTitle.size();
        int dx = (top.width() - size.width()) /2;
        int dy = (top.height()- size.height()) /2;
        QFont font;
        const QFont *themeFont = m_hints->font(QPlatformTheme::SystemFont);
        font.setPointSizeF(themeFont->pointSizeF());
        font.setFamily(themeFont->family());
        font.setBold(true);
        p.setFont(font);
        QPoint windowTitlePoint(top.topLeft().x() + dx,
                 top.topLeft().y() + dy);
        p.drawStaticText(windowTitlePoint, m_windowTitle);
        p.restore();
    }

    QRectF rect;

    // Default pen
    QPen pen(active ? m_foregroundColor : m_foregroundInactiveColor);
    p.setPen(pen);

    // Close button
    p.save();
    rect = closeButtonRect();
    qreal crossSize = rect.height() / 2.3;
    QPointF crossCenter(rect.center());
    QRectF crossRect(crossCenter.x() - crossSize / 2, crossCenter.y() - crossSize / 2, crossSize, crossSize);
    pen.setWidth(2);
    p.setPen(pen);
    p.drawLine(crossRect.topLeft(), crossRect.bottomRight());
    p.drawLine(crossRect.bottomLeft(), crossRect.topRight());
    p.restore();
}

bool QGnomePlatformDecoration::clickButton(Qt::MouseButtons b, Button btn)
{
    if (isLeftClicked(b)) {
        m_clicking = btn;
        return false;
    } else if (isLeftReleased(b)) {
        if (m_clicking == btn) {
            m_clicking = None;
            return true;
        } else {
            m_clicking = None;
        }
    }
    return false;
}

bool QGnomePlatformDecoration::handleMouse(QWaylandInputDevice *inputDevice, const QPointF &local, const QPointF &global, Qt::MouseButtons b, Qt::KeyboardModifiers mods)

{
    Q_UNUSED(global);

    // Figure out what area mouse is in
    if (local.y() <= margins().top()) {
        processMouseTop(inputDevice,local,b,mods);
    } else if (local.y() > window()->height() + margins().top()) {
        processMouseBottom(inputDevice,local,b,mods);
    } else if (local.x() <= margins().left()) {
        processMouseLeft(inputDevice,local,b,mods);
    } else if (local.x() > window()->width() + margins().left()) {
        processMouseRight(inputDevice,local,b,mods);
    } else {
#if QT_CONFIG(cursor)
        waylandWindow()->restoreMouseCursor(inputDevice);
#endif
        setMouseButtons(b);
        return false;
    }

    setMouseButtons(b);
    return true;
}

bool QGnomePlatformDecoration::handleTouch(QWaylandInputDevice *inputDevice, const QPointF &local, const QPointF &global, Qt::TouchPointState state, Qt::KeyboardModifiers mods)
{
    Q_UNUSED(inputDevice);
    Q_UNUSED(global);
    Q_UNUSED(mods);
    bool handled = state == Qt::TouchPointPressed;
    if (handled) {
        if (closeButtonRect().contains(local))
            QWindowSystemInterface::handleCloseEvent(window());
//         else if (maximizeButtonRect().contains(local))
//             window()->setWindowStates(window()->windowStates() ^ Qt::WindowMaximized);
//         else if (minimizeButtonRect().contains(local))
//             window()->setWindowState(Qt::WindowMinimized);
        else if (local.y() <= margins().top())
            waylandWindow()->shellSurface()->move(inputDevice);
        else
            handled = false;
    }

    return handled;
}

void QGnomePlatformDecoration::processMouseTop(QWaylandInputDevice *inputDevice, const QPointF &local, Qt::MouseButtons b, Qt::KeyboardModifiers mods)
{
    Q_UNUSED(mods);
    if (local.y() <= margins().bottom()) {
        if (local.x() <= margins().left()) {
            //top left bit
#if QT_CONFIG(cursor)
            waylandWindow()->setMouseCursor(inputDevice, Qt::SizeFDiagCursor);
#endif
            startResize(inputDevice,WL_SHELL_SURFACE_RESIZE_TOP_LEFT,b);
        } else if (local.x() > window()->width() + margins().left()) {
            //top right bit
#if QT_CONFIG(cursor)
            waylandWindow()->setMouseCursor(inputDevice, Qt::SizeBDiagCursor);
#endif
            startResize(inputDevice,WL_SHELL_SURFACE_RESIZE_TOP_RIGHT,b);
        } else {
            //top resize bit
#if QT_CONFIG(cursor)
            waylandWindow()->setMouseCursor(inputDevice, Qt::SplitVCursor);
#endif
            startResize(inputDevice,WL_SHELL_SURFACE_RESIZE_TOP,b);
        }
    } else if (local.x() <= margins().left()) {
        processMouseLeft(inputDevice, local, b, mods);
    } else if (local.x() > window()->width() + margins().left()) {
        processMouseRight(inputDevice, local, b, mods);
    } else if (closeButtonRect().contains(local)) {
        if (clickButton(b, Close))
            QWindowSystemInterface::handleCloseEvent(window());
    } /* else if (maximizeButtonRect().contains(local)) {
        if (clickButton(b, Maximize))
            window()->setWindowStates(window()->windowStates() ^ Qt::WindowMaximized);
    } else if (minimizeButtonRect().contains(local)) {
        if (clickButton(b, Minimize))
            window()->setWindowState(Qt::WindowMinimized);
    } */else {
#if QT_CONFIG(cursor)
        waylandWindow()->restoreMouseCursor(inputDevice);
#endif
        startMove(inputDevice,b);
    }
}

void QGnomePlatformDecoration::processMouseBottom(QWaylandInputDevice *inputDevice, const QPointF &local, Qt::MouseButtons b, Qt::KeyboardModifiers mods)
{
    Q_UNUSED(mods);
    if (local.x() <= margins().left()) {
        //bottom left bit
#if QT_CONFIG(cursor)
        waylandWindow()->setMouseCursor(inputDevice, Qt::SizeBDiagCursor);
#endif
        startResize(inputDevice, WL_SHELL_SURFACE_RESIZE_BOTTOM_LEFT,b);
    } else if (local.x() > window()->width() + margins().left()) {
        //bottom right bit
#if QT_CONFIG(cursor)
        waylandWindow()->setMouseCursor(inputDevice, Qt::SizeFDiagCursor);
#endif
        startResize(inputDevice, WL_SHELL_SURFACE_RESIZE_BOTTOM_RIGHT,b);
    } else {
        //bottom bit
#if QT_CONFIG(cursor)
        waylandWindow()->setMouseCursor(inputDevice, Qt::SplitVCursor);
#endif
        startResize(inputDevice,WL_SHELL_SURFACE_RESIZE_BOTTOM,b);
    }
}

void QGnomePlatformDecoration::processMouseLeft(QWaylandInputDevice *inputDevice, const QPointF &local, Qt::MouseButtons b, Qt::KeyboardModifiers mods)
{
    Q_UNUSED(local);
    Q_UNUSED(mods);
#if QT_CONFIG(cursor)
    waylandWindow()->setMouseCursor(inputDevice, Qt::SplitHCursor);
#endif
    startResize(inputDevice,WL_SHELL_SURFACE_RESIZE_LEFT,b);
}

void QGnomePlatformDecoration::processMouseRight(QWaylandInputDevice *inputDevice, const QPointF &local, Qt::MouseButtons b, Qt::KeyboardModifiers mods)
{
    Q_UNUSED(local);
    Q_UNUSED(mods);
#if QT_CONFIG(cursor)
    waylandWindow()->setMouseCursor(inputDevice, Qt::SplitHCursor);
#endif
    startResize(inputDevice, WL_SHELL_SURFACE_RESIZE_RIGHT,b);
}

