/****************************************************************************
**
** Copyright (C) 2019-2021 Jan Grulich <jgrulich@redhat.com>
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

#include "gnomesettings.h"

#include <AdwaitaQt/adwaitacolors.h>
#include <AdwaitaQt/adwaitarenderer.h>

#include <QtGui/QColor>
#include <QtGui/QCursor>
#include <QtGui/QLinearGradient>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QPalette>
#include <QtGui/QPixmap>

#include <qpa/qwindowsysteminterface.h>

#include <QtWaylandClient/private/qwaylandshmbackingstore_p.h>
#include <QtWaylandClient/private/qwaylandwindow_p.h>
#include <QtWaylandClient/private/qwaylandshellsurface_p.h>
#include <QtWaylandClient/private/wayland-wayland-client-protocol.h>

// Button sizing
#define BUTTON_MARGINS 6
#define BUTTON_SPACING 8
#define BUTTON_WIDTH 28

// Decoration sizing
#define SHADOWS_WIDTH 0
#define TITLEBAR_HEIGHT 37
#define WINDOW_BORDER_WIDTH 1

QGnomePlatformDecoration::QGnomePlatformDecoration()
    : m_closeButtonHovered(false)
    , m_maximizeButtonHovered(false)
    , m_minimizeButtonHovered(false)
{
    m_lastButtonClick = QDateTime::currentDateTime();

    QTextOption option(Qt::AlignHCenter | Qt::AlignVCenter);
    option.setWrapMode(QTextOption::NoWrap);
    m_windowTitle.setTextOption(option);

    // Colors
    // TODO: move colors used for decorations to Adwaita-qt
    const bool darkVariant = GnomeSettings::isGtkThemeDarkVariant();
    const QPalette &palette(Adwaita::Colors::palette(darkVariant ? Adwaita::ColorVariant::AdwaitaDark : Adwaita::ColorVariant::Adwaita));

    m_foregroundColor         = palette.color(QPalette::Active, QPalette::Foreground);
    m_foregroundInactiveColor = palette.color(QPalette::Inactive, QPalette::Foreground);
    m_backgroundColorStart    = darkVariant ? QColor("#262626") : QColor("#dad6d2"); // Adwaita GtkHeaderBar color
    m_backgroundColorEnd      = darkVariant ? QColor("#2b2b2b") : QColor("#e1dedb"); // Adwaita GtkHeaderBar color
    m_foregroundInactiveColor = darkVariant ? QColor("#919190") : QColor("#929595");
    m_backgroundInactiveColor = darkVariant ? QColor("#353535") : QColor("#f6f5f4");
    m_borderColor             = darkVariant ? Adwaita::Colors::transparentize(QColor("#1b1b1b"), 0.1) : Adwaita::Colors::transparentize(QColor("black"), 0.77);
    m_borderInactiveColor     = darkVariant ? Adwaita::Colors::transparentize(QColor("#1b1b1b"), 0.1) : Adwaita::Colors::transparentize(QColor("black"), 0.82);
}

QRectF QGnomePlatformDecoration::closeButtonRect() const
{
    if (GnomeSettings::titlebarButtonPlacement() == GnomeSettings::RightPlacement) {
        return QRectF(window()->frameGeometry().width() - BUTTON_WIDTH - (BUTTON_SPACING * 0) - BUTTON_MARGINS - SHADOWS_WIDTH,
                      (margins().top() - BUTTON_WIDTH) / 2, BUTTON_WIDTH, BUTTON_WIDTH);
    } else {
        return QRectF(BUTTON_SPACING * 0 + BUTTON_MARGINS + SHADOWS_WIDTH,
                      (margins().top() - BUTTON_WIDTH) / 2, BUTTON_WIDTH, BUTTON_WIDTH);
    }
}

QRectF QGnomePlatformDecoration::maximizeButtonRect() const
{
    if (GnomeSettings::titlebarButtonPlacement() == GnomeSettings::RightPlacement) {
        return QRectF(window()->frameGeometry().width() - (BUTTON_WIDTH * 2) - (BUTTON_SPACING * 1) - BUTTON_MARGINS - SHADOWS_WIDTH,
                      (margins().top() - BUTTON_WIDTH) / 2, BUTTON_WIDTH, BUTTON_WIDTH);
    } else {
        return QRectF(BUTTON_WIDTH * 1 + (BUTTON_SPACING * 1) + BUTTON_MARGINS + SHADOWS_WIDTH,
                      (margins().top() - BUTTON_WIDTH) / 2, BUTTON_WIDTH, BUTTON_WIDTH);
    }
}

QRectF QGnomePlatformDecoration::minimizeButtonRect() const
{
    const bool maximizeEnabled = GnomeSettings::titlebarButtons().testFlag(GnomeSettings::MaximizeButton);

    if (GnomeSettings::titlebarButtonPlacement() == GnomeSettings::RightPlacement) {
        return QRectF(window()->frameGeometry().width() - BUTTON_WIDTH * (maximizeEnabled ? 3 : 2) - (BUTTON_SPACING * (maximizeEnabled ? 2 : 1)) - BUTTON_MARGINS - SHADOWS_WIDTH,
                      (margins().top() - BUTTON_WIDTH) / 2, BUTTON_WIDTH, BUTTON_WIDTH);
    } else {
        return QRectF(BUTTON_WIDTH * (maximizeEnabled ? 2 : 1) + (BUTTON_SPACING * (maximizeEnabled ? 2 : 1)) + BUTTON_MARGINS + SHADOWS_WIDTH,
                      (margins().top() - BUTTON_WIDTH) / 2, BUTTON_WIDTH, BUTTON_WIDTH);
    }
}

QMargins QGnomePlatformDecoration::margins() const
{
    if ((window()->windowStates() & Qt::WindowMaximized)) {
        return QMargins(0, TITLEBAR_HEIGHT, 0, 0);
    }

    return QMargins(WINDOW_BORDER_WIDTH + SHADOWS_WIDTH,                   // Left
                    TITLEBAR_HEIGHT + WINDOW_BORDER_WIDTH + SHADOWS_WIDTH, // Top
                    WINDOW_BORDER_WIDTH + SHADOWS_WIDTH,                   // Right
                    WINDOW_BORDER_WIDTH + SHADOWS_WIDTH);                  // Bottom
}

void QGnomePlatformDecoration::paint(QPaintDevice *device)
{
    const bool active = window()->handle()->isActive();
    const QRect surfaceRect(QPoint(), window()->frameGeometry().size());
    const QColor borderColor = active ? m_borderColor : m_borderInactiveColor;

    QPainter p(device);
    p.setRenderHint(QPainter::Antialiasing);

    // Shadows
    // ********************************
    // *                              *
    // *                              *
    // *                              *
    // *                              *
    // *                              *
    // *                              *
    // *                              *
    // ********************************

    // QPainterPath shadowRect;
    // shadowRect.addRoundedRect(0, 0, surfaceRect.width(), surfaceRect.height(), 12, 12);
    // p.fillPath(shadowRect, Qt::red);

    // Title bar (border)
    // ********************************
    // *------------------------------*
    // *|                            |*
    // *------------------------------*
    // *                              *
    // *                              *
    // *                              *
    // *                              *
    // *                              *
    // *                              *
    // ********************************
    QPainterPath borderRect;
    if (!(window()->windowStates() & Qt::WindowMaximized)) {
        borderRect.addRoundedRect(SHADOWS_WIDTH, SHADOWS_WIDTH, surfaceRect.width() - (2 * SHADOWS_WIDTH), margins().top() + 8, 10, 10);
        p.fillPath(borderRect.simplified(), borderColor);
    }

    // Title bar
    // ********************************
    // *------------------------------*
    // *|############################|*
    // *                              *
    // *                              *
    // *                              *
    // *                              *
    // *                              *
    // *                              *
    // *                              *
    // ********************************
    QPainterPath roundedRect;
    if ((window()->windowStates() & Qt::WindowMaximized)) {
        roundedRect.addRect(0, 0, surfaceRect.width(), margins().top() + 8);
    } else {
        roundedRect.addRoundedRect(SHADOWS_WIDTH + WINDOW_BORDER_WIDTH, SHADOWS_WIDTH + WINDOW_BORDER_WIDTH, surfaceRect.width() - margins().left() - margins().right(), margins().top() + 8, 8, 8);
    }

    QLinearGradient gradient(margins().left(), margins().top() + 6, margins().left(), 1);
    gradient.setColorAt(0, active ? m_backgroundColorStart : m_backgroundInactiveColor);
    gradient.setColorAt(1, active ? m_backgroundColorEnd : m_backgroundInactiveColor);
    p.fillPath(roundedRect.simplified(), gradient);

    // Border around
    // ********************************
    // *------------------------------*
    // *|############################|*
    // *|                            |*
    // *|                            |*
    // *|                            |*
    // *|                            |*
    // *|                            |*
    // *|                            |*
    // *------------------------------*
    // ********************************
    if (!(window()->windowStates() & Qt::WindowMaximized)) {
        QPainterPath borderPath;
        // Left
        borderPath.addRect(SHADOWS_WIDTH, margins().top(), margins().left(), surfaceRect.height() - margins().top() - SHADOWS_WIDTH - WINDOW_BORDER_WIDTH);
        // Bottom
        borderPath.addRect(SHADOWS_WIDTH, surfaceRect.height() - SHADOWS_WIDTH - WINDOW_BORDER_WIDTH, surfaceRect.width() - (2 * SHADOWS_WIDTH), WINDOW_BORDER_WIDTH);
        // Right
        borderPath.addRect(surfaceRect.width() - margins().right(), margins().top(), WINDOW_BORDER_WIDTH, surfaceRect.height() - margins().bottom() - margins().top());
        p.fillPath(borderPath, borderColor);
    }

    // Border between window and decorations
    // ********************************
    // *------------------------------*
    // *|############################|*
    // *------------------------------*
    // *|                            |*
    // *|                            |*
    // *|                            |*
    // *|                            |*
    // *|                            |*
    // *------------------------------*
    // ********************************
    p.save();
    p.setPen(borderColor);
    p.drawLine(SHADOWS_WIDTH + WINDOW_BORDER_WIDTH, margins().top() - WINDOW_BORDER_WIDTH,
               surfaceRect.width() - SHADOWS_WIDTH - WINDOW_BORDER_WIDTH, margins().top() - WINDOW_BORDER_WIDTH);
    p.restore();



    // Window title
    // ********************************
    // *------------------------------*
    // *|########## FOO #############|*
    // *------------------------------*
    // *|                            |*
    // *|                            |*
    // *|                            |*
    // *|                            |*
    // *|                            |*
    // *------------------------------*
    // ********************************

    const QRect top = QRect(0, 0, surfaceRect.width(), margins().top());
    const QString windowTitleText = window()->title();
    if (!windowTitleText.isEmpty()) {
        if (m_windowTitle.text() != windowTitleText) {
            m_windowTitle.setText(windowTitleText);
            m_windowTitle.prepare();
        }

        QRect titleBar = top;
        if (GnomeSettings::titlebarButtonPlacement() == GnomeSettings::RightPlacement) {
            titleBar.setLeft(margins().left());
            titleBar.setRight(static_cast<int>(minimizeButtonRect().left()) - 8);
        } else {
            titleBar.setLeft(static_cast<int>(minimizeButtonRect().right()) + 8);
            titleBar.setRight(surfaceRect.width() - margins().right());
        }

        p.save();
        p.setClipRect(titleBar);
        p.setPen(active ? m_foregroundColor : m_foregroundInactiveColor);
        QSizeF size = m_windowTitle.size();
        int dx = (static_cast<int>(top.width()) - static_cast<int>(size.width())) /2;
        int dy = (static_cast<int>(top.height())- static_cast<int>(size.height())) /2;
        QFont font;
        const QFont *themeFont = GnomeSettings::font(QPlatformTheme::TitleBarFont);
        font.setPointSizeF(themeFont->pointSizeF());
        font.setFamily(themeFont->family());
        font.setBold(themeFont->bold());
        p.setFont(font);
        QPoint windowTitlePoint(top.topLeft().x() + dx,
                 top.topLeft().y() + dy);
        p.drawStaticText(windowTitlePoint, m_windowTitle);
        p.restore();
    }

    QRectF rect;
    Adwaita::StyleOptions decorationButtonStyle(&p, QRect());
    decorationButtonStyle.setColor(active ? m_foregroundColor : m_foregroundInactiveColor);

    // Close button
    rect = closeButtonRect();
    if (m_closeButtonHovered && active) {
        QRect buttonRect(static_cast<int>(rect.x()), static_cast<int>(rect.y()), BUTTON_WIDTH, BUTTON_WIDTH);
        Adwaita::StyleOptions styleOptions(&p, buttonRect);
        styleOptions.setMouseOver(true);
        styleOptions.setSunken(m_clicking == Button::Close);
        styleOptions.setColorVariant(GnomeSettings::isGtkThemeDarkVariant() ? Adwaita::ColorVariant::AdwaitaDark : Adwaita::ColorVariant::Adwaita);
        styleOptions.setColor(Adwaita::Colors::buttonBackgroundColor(styleOptions));
        styleOptions.setOutlineColor(Adwaita::Colors::buttonOutlineColor(styleOptions));
        Adwaita::Renderer::renderFlatRoundedButtonFrame(styleOptions);
    }
    decorationButtonStyle.setRect(QRect(static_cast<int>(rect.x()) + (BUTTON_WIDTH / 4), static_cast<int>(rect.y()) + (BUTTON_WIDTH / 4), BUTTON_WIDTH / 2, BUTTON_WIDTH / 2));
    Adwaita::Renderer::renderDecorationButton(decorationButtonStyle, Adwaita::ButtonType::ButtonClose);

    // Maximize button
    if (GnomeSettings::titlebarButtons().testFlag(GnomeSettings::MaximizeButton)) {
        rect = maximizeButtonRect();
        if (m_maximizeButtonHovered && active) {
            QRect buttonRect(static_cast<int>(rect.x()), static_cast<int>(rect.y()), BUTTON_WIDTH, BUTTON_WIDTH);
            Adwaita::StyleOptions styleOptions(&p, buttonRect);
            styleOptions.setMouseOver(true);
            styleOptions.setSunken(m_clicking == Button::Maximize || m_clicking == Button::Restore);
            styleOptions.setColorVariant(GnomeSettings::isGtkThemeDarkVariant() ? Adwaita::ColorVariant::AdwaitaDark : Adwaita::ColorVariant::Adwaita);
            styleOptions.setColor(Adwaita::Colors::buttonBackgroundColor(styleOptions));
            styleOptions.setOutlineColor(Adwaita::Colors::buttonOutlineColor(styleOptions));
            Adwaita::Renderer::renderFlatRoundedButtonFrame(styleOptions);
        }
        decorationButtonStyle.setRect(QRect(static_cast<int>(rect.x()) + (BUTTON_WIDTH / 4), static_cast<int>(rect.y()) + (BUTTON_WIDTH / 4), BUTTON_WIDTH / 2, BUTTON_WIDTH / 2));
        const Adwaita::ButtonType buttonType = (window()->windowStates() & Qt::WindowMaximized) ? Adwaita::ButtonType::ButtonRestore : Adwaita::ButtonType::ButtonMaximize;
        Adwaita::Renderer::renderDecorationButton(decorationButtonStyle, buttonType);
    }

    // Minimize button
    if (GnomeSettings::titlebarButtons().testFlag(GnomeSettings::MinimizeButton)) {
        rect = minimizeButtonRect();
        if (m_minimizeButtonHovered && active) {
            QRect buttonRect(static_cast<int>(rect.x()), static_cast<int>(rect.y()), 28, 28);
            Adwaita::StyleOptions styleOptions(&p, buttonRect);
            styleOptions.setMouseOver(true);
            styleOptions.setSunken(m_clicking == Button::Minimize);
            styleOptions.setColorVariant(GnomeSettings::isGtkThemeDarkVariant() ? Adwaita::ColorVariant::AdwaitaDark : Adwaita::ColorVariant::Adwaita);
            styleOptions.setColor(Adwaita::Colors::buttonBackgroundColor(styleOptions));
            styleOptions.setOutlineColor(Adwaita::Colors::buttonOutlineColor(styleOptions));
            Adwaita::Renderer::renderFlatRoundedButtonFrame(styleOptions);
        }
        decorationButtonStyle.setRect(QRect(static_cast<int>(rect.x()) + (BUTTON_WIDTH / 4), static_cast<int>(rect.y()) + (BUTTON_WIDTH / 4), BUTTON_WIDTH / 2, BUTTON_WIDTH / 2));
        Adwaita::Renderer::renderDecorationButton(decorationButtonStyle, Adwaita::ButtonType::ButtonMinimize);
    }
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

bool QGnomePlatformDecoration::doubleClickButton(Qt::MouseButtons b, const QPointF &local,  const QDateTime &currentTime)
{
    if (b & Qt::LeftButton) {
        const qint64 clickInterval = m_lastButtonClick.msecsTo(currentTime);
        m_lastButtonClick = currentTime;
        const int doubleClickDistance = GnomeSettings::hint(QPlatformTheme::MouseDoubleClickDistance).toInt();
        const QPointF posDiff = m_lastButtonClickPosition - local;
        if ((clickInterval <= GnomeSettings::hint(QPlatformTheme::MouseDoubleClickInterval).toInt()) &&
            ((posDiff.x() <= doubleClickDistance && posDiff.x() >= -doubleClickDistance) && ((posDiff.y() <= doubleClickDistance && posDiff.y() >= -doubleClickDistance)))) {
            return true;
        }

        m_lastButtonClickPosition = local;
    }

    return false;
}

bool QGnomePlatformDecoration::handleMouse(QWaylandInputDevice *inputDevice, const QPointF &local, const QPointF &global, Qt::MouseButtons b, Qt::KeyboardModifiers mods)
{
    Q_UNUSED(global)

    if (local.y() > margins().top()) {
        updateButtonHoverState(Button::None);
    }

    // Figure out what area mouse is in
    if (local.y() <= margins().top()) {
        processMouseTop(inputDevice,local,b,mods);
    } else if (local.y() >= window()->height() + margins().top()) {
        processMouseBottom(inputDevice,local,b,mods);
    } else if (local.x() <= margins().left()) {
        processMouseLeft(inputDevice,local,b,mods);
    } else if (local.x() >= window()->width() + margins().left()) {
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
    Q_UNUSED(inputDevice)
    Q_UNUSED(global)
    Q_UNUSED(mods)
    bool handled = state == Qt::TouchPointPressed;
    if (handled) {
        if (closeButtonRect().contains(local)) {
            QWindowSystemInterface::handleCloseEvent(window());
        } else if (GnomeSettings::titlebarButtons().testFlag(GnomeSettings::MaximizeButton) && maximizeButtonRect().contains(local)) {
            window()->setWindowStates(window()->windowStates() ^ Qt::WindowMaximized);
        } else if (GnomeSettings::titlebarButtons().testFlag(GnomeSettings::MinimizeButton) && minimizeButtonRect().contains(local)) {
            window()->setWindowState(Qt::WindowMinimized);
        } else if (local.y() <= margins().top()) {
            waylandWindow()->shellSurface()->move(inputDevice);
        } else {
            handled = false;
        }
    }

    return handled;
}

void QGnomePlatformDecoration::processMouseTop(QWaylandInputDevice *inputDevice, const QPointF &local, Qt::MouseButtons b, Qt::KeyboardModifiers mods)
{
    Q_UNUSED(mods)

    QDateTime currentDateTime = QDateTime::currentDateTime();

    if (!closeButtonRect().contains(local) && !maximizeButtonRect().contains(local) && !minimizeButtonRect().contains(local)) {
        updateButtonHoverState(Button::None);
    }

    if (local.y() <= margins().bottom()) {
        if (local.x() <= margins().left()) {
            //top left bit
#if QT_CONFIG(cursor)
            waylandWindow()->setMouseCursor(inputDevice, Qt::SizeFDiagCursor);
#endif
#if (QT_VERSION < QT_VERSION_CHECK(5, 13, 0))
            startResize(inputDevice, WL_SHELL_SURFACE_RESIZE_TOP_LEFT, b);
#else
            startResize(inputDevice, Qt::TopEdge | Qt::LeftEdge, b);
#endif
        } else if (local.x() > window()->width() + margins().left()) {
            //top right bit
#if QT_CONFIG(cursor)
            waylandWindow()->setMouseCursor(inputDevice, Qt::SizeBDiagCursor);
#endif
#if (QT_VERSION < QT_VERSION_CHECK(5, 13, 0))
            startResize(inputDevice, WL_SHELL_SURFACE_RESIZE_TOP_RIGHT, b);
#else
            startResize(inputDevice, Qt::TopEdge | Qt::RightEdge, b);
#endif
        } else {
            //top resize bit
#if QT_CONFIG(cursor)
            waylandWindow()->setMouseCursor(inputDevice, Qt::SplitVCursor);
#endif
#if (QT_VERSION < QT_VERSION_CHECK(5, 13, 0))
            startResize(inputDevice, WL_SHELL_SURFACE_RESIZE_TOP, b);
#else
            startResize(inputDevice, Qt::TopEdge, b);
#endif
        }
    } else if (local.x() <= margins().left()) {
        processMouseLeft(inputDevice, local, b, mods);
    } else if (local.x() > window()->width() + margins().left()) {
        processMouseRight(inputDevice, local, b, mods);
    } else if (closeButtonRect().contains(local)) {
        if (clickButton(b, Close)) {
            QWindowSystemInterface::handleCloseEvent(window());
            m_closeButtonHovered = false;
        }
        updateButtonHoverState(Button::Close);
    }  else if (GnomeSettings::titlebarButtons().testFlag(GnomeSettings::MaximizeButton) && maximizeButtonRect().contains(local)) {
        updateButtonHoverState(Button::Maximize);
        if (clickButton(b, Maximize)) {
            window()->setWindowStates(window()->windowStates() ^ Qt::WindowMaximized);
            m_maximizeButtonHovered = false;
        }
    } else if (GnomeSettings::titlebarButtons().testFlag(GnomeSettings::MinimizeButton) && minimizeButtonRect().contains(local)) {
        updateButtonHoverState(Button::Minimize);
        if (clickButton(b, Minimize)) {
            window()->setWindowState(Qt::WindowMinimized);
            m_minimizeButtonHovered = false;
        }
    } else if (doubleClickButton(b, local, currentDateTime)) {
        window()->setWindowStates(window()->windowStates() ^ Qt::WindowMaximized);
    } else {
#if QT_CONFIG(cursor)
        waylandWindow()->restoreMouseCursor(inputDevice);
#endif
        startMove(inputDevice,b);
    }
}

void QGnomePlatformDecoration::processMouseBottom(QWaylandInputDevice *inputDevice, const QPointF &local, Qt::MouseButtons b, Qt::KeyboardModifiers mods)
{
    Q_UNUSED(mods)
    if (local.x() <= margins().left()) {
        //bottom left bit
#if QT_CONFIG(cursor)
        waylandWindow()->setMouseCursor(inputDevice, Qt::SizeBDiagCursor);
#endif
#if (QT_VERSION < QT_VERSION_CHECK(5, 13, 0))
        startResize(inputDevice, WL_SHELL_SURFACE_RESIZE_BOTTOM_LEFT, b);
#else
        startResize(inputDevice, Qt::BottomEdge | Qt::LeftEdge, b);
#endif
    } else if (local.x() > window()->width() + margins().right()) {
        //bottom right bit
#if QT_CONFIG(cursor)
        waylandWindow()->setMouseCursor(inputDevice, Qt::SizeFDiagCursor);
#endif
#if (QT_VERSION < QT_VERSION_CHECK(5, 13, 0))
        startResize(inputDevice, WL_SHELL_SURFACE_RESIZE_BOTTOM_RIGHT, b);
#else
        startResize(inputDevice, Qt::BottomEdge | Qt::RightEdge, b);
#endif
    } else {
        //bottom bit
#if QT_CONFIG(cursor)
        waylandWindow()->setMouseCursor(inputDevice, Qt::SplitVCursor);
#endif
#if (QT_VERSION < QT_VERSION_CHECK(5, 13, 0))
        startResize(inputDevice, WL_SHELL_SURFACE_RESIZE_BOTTOM, b);
#else
        startResize(inputDevice, Qt::BottomEdge, b);
#endif
    }
}

void QGnomePlatformDecoration::processMouseLeft(QWaylandInputDevice *inputDevice, const QPointF &local, Qt::MouseButtons b, Qt::KeyboardModifiers mods)
{
    Q_UNUSED(local)
    Q_UNUSED(mods)
#if QT_CONFIG(cursor)
    waylandWindow()->setMouseCursor(inputDevice, Qt::SplitHCursor);
#endif
#if (QT_VERSION < QT_VERSION_CHECK(5, 13, 0))
        startResize(inputDevice, WL_SHELL_SURFACE_RESIZE_LEFT, b);
#else
    startResize(inputDevice, Qt::LeftEdge, b);
#endif
}

void QGnomePlatformDecoration::processMouseRight(QWaylandInputDevice *inputDevice, const QPointF &local, Qt::MouseButtons b, Qt::KeyboardModifiers mods)
{
    Q_UNUSED(local)
    Q_UNUSED(mods)
#if QT_CONFIG(cursor)
    waylandWindow()->setMouseCursor(inputDevice, Qt::SplitHCursor);
#endif
#if (QT_VERSION < QT_VERSION_CHECK(5, 13, 0))
        startResize(inputDevice, WL_SHELL_SURFACE_RESIZE_RIGHT, b);
#else
    startResize(inputDevice, Qt::RightEdge, b);
#endif
}

bool QGnomePlatformDecoration::updateButtonHoverState(Button hoveredButton)
{
    bool currentCloseButtonState = m_closeButtonHovered;
    bool currentMaximizeButtonState = m_maximizeButtonHovered;
    bool currentMinimizeButtonState = m_minimizeButtonHovered;

    m_closeButtonHovered = hoveredButton == Button::Close;
    m_maximizeButtonHovered = hoveredButton == Button::Maximize;
    m_minimizeButtonHovered = hoveredButton == Button::Minimize;

    if (m_closeButtonHovered != currentCloseButtonState
        || m_maximizeButtonHovered != currentMaximizeButtonState
        || m_minimizeButtonHovered != currentMinimizeButtonState) {
        // Set dirty flag
        waylandWindow()->decoration()->update();
        // Force re-paint
        // NOTE: not sure it's correct, but it's the only way to make it work
        if (waylandWindow()->backingStore()) {
            waylandWindow()->backingStore()->flush(window(), QRegion(), QPoint());
        }
        return true;
    }

    return false;
}
