/****************************************************************************
**
** Copyright (C) 2019-2022 Jan Grulich <jgrulich@redhat.com>
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

#include <QtGui/QColor>
#include <QtGui/QCursor>
#include <QtGui/QLinearGradient>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QPalette>
#include <QtGui/QPixmap>

#include <qpa/qwindowsysteminterface.h>

#include <QtWaylandClient/private/qwaylandshellsurface_p.h>
#include <QtWaylandClient/private/qwaylandshmbackingstore_p.h>
#include <QtWaylandClient/private/qwaylandwindow_p.h>
#include <QtWaylandClient/private/wayland-wayland-client-protocol.h>

// Button sizing
#define BUTTON_MARGINS 6
#define BUTTON_SPACING 8
#define BUTTON_WIDTH 28

// Decoration sizing
#define SHADOWS_WIDTH 10
#define TITLEBAR_HEIGHT 37
#define WINDOW_BORDER_WIDTH 1

Q_DECL_IMPORT void qt_blurImage(QPainter *p, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0);

QGnomePlatformDecoration::QGnomePlatformDecoration()
    : m_closeButtonHovered(false)
    , m_maximizeButtonHovered(false)
    , m_minimizeButtonHovered(false)
{
    m_lastButtonClick = QDateTime::currentDateTime();

    QTextOption option(Qt::AlignHCenter | Qt::AlignVCenter);
    option.setWrapMode(QTextOption::NoWrap);
    m_windowTitle.setTextOption(option);

    connect(&GnomeSettings::getInstance(), &GnomeSettings::themeChanged, this, [this]() {
        loadConfiguration();
        forceRepaint();
    });
    connect(&GnomeSettings::getInstance(), &GnomeSettings::titlebarChanged, this, [this]() {
        loadConfiguration();
        forceRepaint();
    });

    loadConfiguration();
}

QRectF QGnomePlatformDecoration::closeButtonRect() const
{
    if (GnomeSettings::getInstance().titlebarButtonPlacement() == GnomeSettings::getInstance().RightPlacement) {
        return QRectF(windowContentGeometry().width() - BUTTON_WIDTH - (BUTTON_SPACING * 0) - BUTTON_MARGINS - margins().right(),
                      (margins().top() - BUTTON_WIDTH + margins().bottom()) / 2,
                      BUTTON_WIDTH,
                      BUTTON_WIDTH);
    } else {
        return QRectF(BUTTON_SPACING * 0 + BUTTON_MARGINS + margins().left(),
                      (margins().top() - BUTTON_WIDTH + margins().bottom()) / 2,
                      BUTTON_WIDTH,
                      BUTTON_WIDTH);
    }
}

QRectF QGnomePlatformDecoration::maximizeButtonRect() const
{
    if (GnomeSettings::getInstance().titlebarButtonPlacement() == GnomeSettings::getInstance().RightPlacement) {
        return QRectF(windowContentGeometry().width() - (BUTTON_WIDTH * 2) - (BUTTON_SPACING * 1) - BUTTON_MARGINS - margins().right(),
                      (margins().top() - BUTTON_WIDTH + margins().bottom()) / 2,
                      BUTTON_WIDTH,
                      BUTTON_WIDTH);
    } else {
        return QRectF(BUTTON_WIDTH * 1 + (BUTTON_SPACING * 1) + BUTTON_MARGINS + margins().left(),
                      (margins().top() - BUTTON_WIDTH + margins().bottom()) / 2,
                      BUTTON_WIDTH,
                      BUTTON_WIDTH);
    }
}

QRectF QGnomePlatformDecoration::minimizeButtonRect() const
{
    const bool maximizeEnabled = GnomeSettings::getInstance().titlebarButtons().testFlag(GnomeSettings::getInstance().MaximizeButton);

    if (GnomeSettings::getInstance().titlebarButtonPlacement() == GnomeSettings::getInstance().RightPlacement) {
        return QRectF(windowContentGeometry().width() - BUTTON_WIDTH * (maximizeEnabled ? 3 : 2) - (BUTTON_SPACING * (maximizeEnabled ? 2 : 1)) - BUTTON_MARGINS
                          - margins().right(),
                      (margins().top() - BUTTON_WIDTH + margins().bottom()) / 2,
                      BUTTON_WIDTH,
                      BUTTON_WIDTH);
    } else {
        return QRectF(BUTTON_WIDTH * (maximizeEnabled ? 2 : 1) + (BUTTON_SPACING * (maximizeEnabled ? 2 : 1)) + BUTTON_MARGINS + margins().left(),
                      (margins().top() - BUTTON_WIDTH + margins().bottom()) / 2,
                      BUTTON_WIDTH,
                      BUTTON_WIDTH);
    }
}

#ifdef DECORATION_SHADOWS_SUPPORT // Qt 6.2.0+ or patched QtWayland
QMargins QGnomePlatformDecoration::margins(MarginsType marginsType) const
{
    const bool maximized = waylandWindow()->windowStates() & Qt::WindowMaximized;
    const bool tiledLeft = waylandWindow()->toplevelWindowTilingStates() & QWaylandWindow::WindowTiledLeft;
    const bool tiledRight = waylandWindow()->toplevelWindowTilingStates() & QWaylandWindow::WindowTiledRight;
    const bool tiledTop = waylandWindow()->toplevelWindowTilingStates() & QWaylandWindow::WindowTiledTop;
    const bool tiledBottom = waylandWindow()->toplevelWindowTilingStates() & QWaylandWindow::WindowTiledBottom;

    if (marginsType == Full || marginsType == ShadowsExcluded) {
        // For maximized window, we only include window title, no borders and no shadows
        if (maximized) {
            return QMargins(0, TITLEBAR_HEIGHT, 0, 0);
        }

        // Specifically requsted margins with shadows excluded, only on non-tiled side
        if (marginsType == ShadowsExcluded) {
            return QMargins(tiledLeft ? 0 : WINDOW_BORDER_WIDTH,
                            tiledTop ? TITLEBAR_HEIGHT : TITLEBAR_HEIGHT + WINDOW_BORDER_WIDTH,
                            tiledRight ? 0 : WINDOW_BORDER_WIDTH,
                            tiledBottom ? 0 : WINDOW_BORDER_WIDTH);
        }

        // Otherwise include borders and shadows only on non-tiled side
        return QMargins(tiledLeft ? 0 : WINDOW_BORDER_WIDTH + SHADOWS_WIDTH,
                        tiledTop ? TITLEBAR_HEIGHT : TITLEBAR_HEIGHT + WINDOW_BORDER_WIDTH + SHADOWS_WIDTH,
                        tiledRight ? 0 : WINDOW_BORDER_WIDTH + SHADOWS_WIDTH,
                        tiledBottom ? 0 : WINDOW_BORDER_WIDTH + SHADOWS_WIDTH);

    } else { // (marginsType == ShadowsOnly)
        // For maximized window, we only include window title, no borders and no shadows
        if (maximized) {
            return QMargins();
        } else {
            // For tiled window, we only include shadows on non-tiled side
            return QMargins(tiledLeft ? 0 : SHADOWS_WIDTH, tiledTop ? 0 : SHADOWS_WIDTH, tiledRight ? 0 : SHADOWS_WIDTH, tiledBottom ? 0 : SHADOWS_WIDTH);
        }
    }
#else
QMargins QGnomePlatformDecoration::margins() const
{
    if ((window()->windowStates() & Qt::WindowMaximized)) {
        return QMargins(0, TITLEBAR_HEIGHT, 0, 0);
    }

    return QMargins(WINDOW_BORDER_WIDTH, // Left
                    TITLEBAR_HEIGHT + WINDOW_BORDER_WIDTH, // Top
                    WINDOW_BORDER_WIDTH, // Right
                    WINDOW_BORDER_WIDTH); // Bottom
#endif
}

void QGnomePlatformDecoration::paint(QPaintDevice *device)
{
    Qt::WindowStates windowStates;
#ifdef DECORATION_SHADOWS_SUPPORT // Qt 6.2.0+ or patched QtWayland
    windowStates = waylandWindow()->windowStates();
#else
    windowStates = window()->windowStates();
#endif

    const bool active = windowStates & Qt::WindowActive;
    const QRect surfaceRect = windowContentGeometry();
    const QColor borderColor = active ? m_borderColor : m_borderInactiveColor;

    QPainter p(device);
    p.setRenderHint(QPainter::Antialiasing);

#ifdef DECORATION_SHADOWS_SUPPORT // Qt 6.2.0+ or patched QtWayland
    const bool maximized = windowStates & Qt::WindowMaximized;
    const bool tiledLeft = waylandWindow()->toplevelWindowTilingStates() & QWaylandWindow::WindowTiledLeft;
    const bool tiledRight = waylandWindow()->toplevelWindowTilingStates() & QWaylandWindow::WindowTiledRight;
    const bool tiledTop = waylandWindow()->toplevelWindowTilingStates() & QWaylandWindow::WindowTiledTop;
    const bool tiledBottom = waylandWindow()->toplevelWindowTilingStates() & QWaylandWindow::WindowTiledBottom;

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
    if (active && !(maximized || tiledBottom || tiledTop || tiledRight || tiledLeft)) {
        if (m_shadowPixmap.size() != surfaceRect.size()) {
            QPixmap source = QPixmap(surfaceRect.size());
            source.fill(Qt::transparent);
            {
                QPainter tmpPainter(&source);
                tmpPainter.setBrush(borderColor);
                tmpPainter.drawRoundedRect(SHADOWS_WIDTH, // Do not paint over shadows
                                           SHADOWS_WIDTH, // Do not paint over shadows
                                           surfaceRect.width() - (2 * SHADOWS_WIDTH), // Full width - shadows
                                           surfaceRect.height() / 2, // Half of the full height
                                           8,
                                           8);
                tmpPainter.drawRect(SHADOWS_WIDTH, // Do not paint over shadows
                                    surfaceRect.height() / 2, // Start somewhere in the middle
                                    surfaceRect.width() - (2 * SHADOWS_WIDTH), // Full width - shadows
                                    (surfaceRect.height() / 2) - SHADOWS_WIDTH); // Half of the full height - shadows
                tmpPainter.end();
            }

            QImage backgroundImage(surfaceRect.size(), QImage::Format_ARGB32_Premultiplied);
            backgroundImage.fill(0);

            QPainter backgroundPainter(&backgroundImage);
            backgroundPainter.drawPixmap(QPointF(), source);
            backgroundPainter.end();

            QImage blurredImage(surfaceRect.size(), QImage::Format_ARGB32_Premultiplied);
            blurredImage.fill(0);
            {
                QPainter blurPainter(&blurredImage);
                qt_blurImage(&blurPainter, backgroundImage, 12, false, false);
                blurPainter.end();
            }
            backgroundImage = blurredImage;

            backgroundPainter.begin(&backgroundImage);
            backgroundPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
            QRect rect = backgroundImage.rect().marginsRemoved(QMargins(8, 8, 8, 8));
            backgroundPainter.fillRect(rect, QColor(0, 0, 0, 160));
            backgroundPainter.end();

            m_shadowPixmap = QPixmap::fromImage(backgroundImage);
        }

        QRect clips[] = {
            QRect(0, 0, surfaceRect.width(), margins().top()),
            QRect(0, margins().top(), margins().left(), surfaceRect.height() - margins().top() - margins().bottom()),
            QRect(0, surfaceRect.height() - margins().bottom(), surfaceRect.width(), margins().bottom()),
            QRect(surfaceRect.width() - margins().right(), margins().top(), margins().right(), surfaceRect.height() - margins().top() - margins().bottom())};

        for (int i = 0; i < 4; ++i) {
            p.save();
            p.setClipRect(clips[i]);
            p.drawPixmap(QPoint(), m_shadowPixmap);
            p.restore();
        }
    }

    // Title bar (border) - painted only when the window is not maximized or tiled
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
    if (!(maximized || tiledLeft || tiledRight)) {
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
    if (maximized || tiledRight || tiledLeft) {
        roundedRect.addRect(margins().left(), margins().bottom(), surfaceRect.width() - margins().left() - margins().right(), margins().top() + 8);
    } else {
        roundedRect.addRoundedRect(margins().left(), margins().bottom(), surfaceRect.width() - margins().left() - margins().right(), margins().top() + 8, 8, 8);
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
    if (!maximized) {
        QPainterPath borderPath;
        // Left
        if (!tiledLeft) {
            // Assume tiled-left also means it will be tiled-top and tiled bottom
            borderPath.addRect(SHADOWS_WIDTH,
                               tiledTop || tiledBottom ? 0 : margins().top(),
                               WINDOW_BORDER_WIDTH,
                               tiledTop || tiledBottom ? surfaceRect.height() : surfaceRect.height() - margins().top() - SHADOWS_WIDTH - WINDOW_BORDER_WIDTH);
        }
        // Bottom
        if (!tiledBottom) {
            borderPath.addRect(SHADOWS_WIDTH,
                               surfaceRect.height() - SHADOWS_WIDTH - WINDOW_BORDER_WIDTH,
                               surfaceRect.width() - (2 * SHADOWS_WIDTH),
                               WINDOW_BORDER_WIDTH);
        }
        // Right
        if (!tiledRight) {
            borderPath.addRect(surfaceRect.width() - margins().right(),
                               tiledTop || tiledBottom ? 0 : margins().top(),
                               WINDOW_BORDER_WIDTH,
                               tiledTop || tiledBottom ? surfaceRect.height() : surfaceRect.height() - margins().top() - SHADOWS_WIDTH - WINDOW_BORDER_WIDTH);
        }
        p.fillPath(borderPath, borderColor);
    }
#else
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
    if (!(windowStates & Qt::WindowMaximized)) {
        borderRect.addRoundedRect(0, 0, surfaceRect.width(), margins().top() + 8, 10, 10);
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
    if ((windowStates & Qt::WindowMaximized)) {
        roundedRect.addRect(0, 0, surfaceRect.width(), margins().top() + 8);
    } else {
        roundedRect
            .addRoundedRect(WINDOW_BORDER_WIDTH, WINDOW_BORDER_WIDTH, surfaceRect.width() - margins().left() - margins().right(), margins().top() + 8, 8, 8);
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
    if (!(windowStates & Qt::WindowMaximized)) {
        QPainterPath borderPath;
        // Left
        borderPath.addRect(0, margins().top(), margins().left(), surfaceRect.height() - margins().top() - WINDOW_BORDER_WIDTH);
        // Bottom
        borderPath.addRect(0, surfaceRect.height() - WINDOW_BORDER_WIDTH, surfaceRect.width(), WINDOW_BORDER_WIDTH);
        // Right
        borderPath.addRect(surfaceRect.width() - margins().right(),
                           margins().top(),
                           WINDOW_BORDER_WIDTH,
                           surfaceRect.height() - margins().bottom() - margins().top());
        p.fillPath(borderPath, borderColor);
    }
#endif
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
    p.drawLine(margins().left(), margins().top() - WINDOW_BORDER_WIDTH, surfaceRect.width() - margins().right(), margins().top() - WINDOW_BORDER_WIDTH);
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

    const QRect top = QRect(margins().left(), margins().bottom(), surfaceRect.width(), margins().top() - margins().bottom());
    const QString windowTitleText = window()->title();
    if (!windowTitleText.isEmpty()) {
        if (m_windowTitle.text() != windowTitleText) {
            m_windowTitle.setText(windowTitleText);
            m_windowTitle.prepare();
        }

        QRect titleBar = top;
        if (GnomeSettings::getInstance().titlebarButtonPlacement() == GnomeSettings::getInstance().RightPlacement) {
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
        int dx = (static_cast<int>(top.width()) - static_cast<int>(size.width())) / 2;
        int dy = (static_cast<int>(top.height()) - static_cast<int>(size.height())) / 2;
        QFont font;
        const QFont *themeFont = GnomeSettings::getInstance().font(QPlatformTheme::TitleBarFont);
        font.setPointSizeF(themeFont->pointSizeF());
        font.setFamily(themeFont->family());
        font.setBold(themeFont->bold());
        p.setFont(font);
        QPoint windowTitlePoint(top.topLeft().x() + dx, top.topLeft().y() + dy);
        p.drawStaticText(windowTitlePoint, m_windowTitle);
        p.restore();
    }

    // Close button
    renderButton(&p, closeButtonRect(), Adwaita::ButtonType::ButtonClose, m_closeButtonHovered && active, m_clicking == Button::Close);

    // Maximize button
    if (GnomeSettings::getInstance().titlebarButtons().testFlag(GnomeSettings::getInstance().MaximizeButton)) {
        renderButton(&p,
                     maximizeButtonRect(),
                     (windowStates & Qt::WindowMaximized) ? Adwaita::ButtonType::ButtonRestore : Adwaita::ButtonType::ButtonMaximize,
                     m_maximizeButtonHovered && active,
                     m_clicking == Button::Maximize || m_clicking == Button::Restore);
    }

    // Minimize button
    if (GnomeSettings::getInstance().titlebarButtons().testFlag(GnomeSettings::getInstance().MinimizeButton)) {
        renderButton(&p, minimizeButtonRect(), Adwaita::ButtonType::ButtonMinimize, m_minimizeButtonHovered && active, m_clicking == Button::Minimize);
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

bool QGnomePlatformDecoration::doubleClickButton(Qt::MouseButtons b, const QPointF &local, const QDateTime &currentTime)
{
    if (b & Qt::LeftButton) {
        const qint64 clickInterval = m_lastButtonClick.msecsTo(currentTime);
        m_lastButtonClick = currentTime;
        const int doubleClickDistance = GnomeSettings::getInstance().hint(QPlatformTheme::MouseDoubleClickDistance).toInt();
        const QPointF posDiff = m_lastButtonClickPosition - local;
        if ((clickInterval <= GnomeSettings::getInstance().hint(QPlatformTheme::MouseDoubleClickInterval).toInt())
            && ((posDiff.x() <= doubleClickDistance && posDiff.x() >= -doubleClickDistance)
                && ((posDiff.y() <= doubleClickDistance && posDiff.y() >= -doubleClickDistance)))) {
            return true;
        }

        m_lastButtonClickPosition = local;
    }

    return false;
}

bool QGnomePlatformDecoration::handleMouse(QWaylandInputDevice *inputDevice,
                                           const QPointF &local,
                                           const QPointF &global,
                                           Qt::MouseButtons b,
                                           Qt::KeyboardModifiers mods)
{
    Q_UNUSED(global)

    if (local.y() > margins().top()) {
        updateButtonHoverState(Button::None);
    }

    // Figure out what area mouse is in
    QRect surfaceRect = windowContentGeometry();
    if (local.y() <= surfaceRect.top() + margins().top()) {
        processMouseTop(inputDevice, local, b, mods);
    } else if (local.y() > surfaceRect.bottom() - margins().bottom()) {
        processMouseBottom(inputDevice, local, b, mods);
    } else if (local.x() <= surfaceRect.left() + margins().left()) {
        processMouseLeft(inputDevice, local, b, mods);
    } else if (local.x() > surfaceRect.right() - margins().right()) {
        processMouseRight(inputDevice, local, b, mods);
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

#if QT_VERSION >= 0x060000
bool QGnomePlatformDecoration::handleTouch(QWaylandInputDevice *inputDevice,
                                           const QPointF &local,
                                           const QPointF &global,
                                           QEventPoint::State state,
                                           Qt::KeyboardModifiers mods)
#else
bool QGnomePlatformDecoration::handleTouch(QWaylandInputDevice *inputDevice,
                                           const QPointF &local,
                                           const QPointF &global,
                                           Qt::TouchPointState state,
                                           Qt::KeyboardModifiers mods)
#endif
{
    Q_UNUSED(inputDevice)
    Q_UNUSED(global)
    Q_UNUSED(mods)
#if QT_VERSION >= 0x060000
    bool handled = state == QEventPoint::Pressed;
#else
    bool handled = state == Qt::TouchPointPressed;
#endif
    if (handled) {
        if (closeButtonRect().contains(local)) {
            QWindowSystemInterface::handleCloseEvent(window());
        } else if (GnomeSettings::getInstance().titlebarButtons().testFlag(GnomeSettings::getInstance().MaximizeButton)
                   && maximizeButtonRect().contains(local)) {
            window()->setWindowStates(window()->windowStates() ^ Qt::WindowMaximized);
        } else if (GnomeSettings::getInstance().titlebarButtons().testFlag(GnomeSettings::getInstance().MinimizeButton)
                   && minimizeButtonRect().contains(local)) {
            window()->setWindowState(Qt::WindowMinimized);
        } else if (local.y() <= margins().top()) {
            waylandWindow()->shellSurface()->move(inputDevice);
        } else {
            handled = false;
        }
    }

    return handled;
}

QRect QGnomePlatformDecoration::windowContentGeometry() const
{
#ifdef DECORATION_SHADOWS_SUPPORT // Qt 6.2.0+ or patched QtWayland
    return waylandWindow()->windowContentGeometry() + margins(ShadowsOnly);
#else
    return waylandWindow()->windowContentGeometry();
#endif
}

void QGnomePlatformDecoration::loadConfiguration()
{
    // Colors
    // TODO: move colors used for decorations to Adwaita-qt
    const bool darkVariant = GnomeSettings::getInstance().useGtkThemeDarkVariant();
    const bool highContrastVariant = GnomeSettings::getInstance().useGtkThemeHighContrastVariant();

    m_adwaitaVariant = darkVariant ? highContrastVariant ? Adwaita::ColorVariant::AdwaitaHighcontrastInverse : Adwaita::ColorVariant::AdwaitaDark
        : highContrastVariant      ? Adwaita::ColorVariant::AdwaitaHighcontrast
                                   : Adwaita::ColorVariant::Adwaita;

    const QPalette &palette(Adwaita::Colors::palette(m_adwaitaVariant));

    m_foregroundColor = palette.color(QPalette::Active, QPalette::WindowText);
    m_foregroundInactiveColor = palette.color(QPalette::Inactive, QPalette::WindowText);
    m_backgroundColorStart = darkVariant ? QColor("#262626") : QColor("#dad6d2"); // Adwaita GtkHeaderBar color
    m_backgroundColorEnd = darkVariant ? QColor("#2b2b2b") : QColor("#e1dedb"); // Adwaita GtkHeaderBar color
    m_foregroundInactiveColor = darkVariant ? QColor("#919190") : QColor("#929595");
    m_backgroundInactiveColor = darkVariant ? QColor("#353535") : QColor("#f6f5f4");
    m_borderColor = darkVariant ? Adwaita::Colors::transparentize(QColor("#1b1b1b"), 0.1) : Adwaita::Colors::transparentize(QColor("black"), 0.77);
    m_borderInactiveColor = darkVariant ? Adwaita::Colors::transparentize(QColor("#1b1b1b"), 0.1) : Adwaita::Colors::transparentize(QColor("black"), 0.82);
}

void QGnomePlatformDecoration::forceRepaint()
{
    // Set dirty flag
    waylandWindow()->decoration()->update();
    // Force re-paint
    // NOTE: not sure it's correct, but it's the only way to make it work
    if (waylandWindow()->backingStore()) {
        waylandWindow()->backingStore()->flush(window(), QRegion(), QPoint());
    }
}

void QGnomePlatformDecoration::processMouseTop(QWaylandInputDevice *inputDevice, const QPointF &local, Qt::MouseButtons b, Qt::KeyboardModifiers mods)
{
    Q_UNUSED(mods)

    QDateTime currentDateTime = QDateTime::currentDateTime();
    QRect surfaceRect = windowContentGeometry();

    if (!closeButtonRect().contains(local) && !maximizeButtonRect().contains(local) && !minimizeButtonRect().contains(local)) {
        updateButtonHoverState(Button::None);
    }

    if (local.y() <= surfaceRect.top() + margins().bottom()) {
        if (local.x() <= margins().left()) {
            // top left bit
#if QT_CONFIG(cursor)
            waylandWindow()->setMouseCursor(inputDevice, Qt::SizeFDiagCursor);
#endif
            startResize(inputDevice, Qt::TopEdge | Qt::LeftEdge, b);
        } else if (local.x() > surfaceRect.right() - margins().left()) {
            // top right bit
#if QT_CONFIG(cursor)
            waylandWindow()->setMouseCursor(inputDevice, Qt::SizeBDiagCursor);
#endif
            startResize(inputDevice, Qt::TopEdge | Qt::RightEdge, b);
        } else {
            // top resize bit
#if QT_CONFIG(cursor)
            waylandWindow()->setMouseCursor(inputDevice, Qt::SplitVCursor);
#endif
            startResize(inputDevice, Qt::TopEdge, b);
        }
    } else if (local.x() <= surfaceRect.left() + margins().left()) {
        processMouseLeft(inputDevice, local, b, mods);
    } else if (local.x() > surfaceRect.right() - margins().right()) {
        processMouseRight(inputDevice, local, b, mods);
    } else if (closeButtonRect().contains(local)) {
        if (clickButton(b, Close)) {
            QWindowSystemInterface::handleCloseEvent(window());
            m_closeButtonHovered = false;
        }
        updateButtonHoverState(Button::Close);
    } else if (GnomeSettings::getInstance().titlebarButtons().testFlag(GnomeSettings::getInstance().MaximizeButton) && maximizeButtonRect().contains(local)) {
        updateButtonHoverState(Button::Maximize);
        if (clickButton(b, Maximize)) {
            window()->setWindowStates(window()->windowStates() ^ Qt::WindowMaximized);
            m_maximizeButtonHovered = false;
        }
    } else if (GnomeSettings::getInstance().titlebarButtons().testFlag(GnomeSettings::getInstance().MinimizeButton) && minimizeButtonRect().contains(local)) {
        updateButtonHoverState(Button::Minimize);
        if (clickButton(b, Minimize)) {
            window()->setWindowState(Qt::WindowMinimized);
            m_minimizeButtonHovered = false;
        }
    } else if (doubleClickButton(b, local, currentDateTime)) {
        window()->setWindowStates(window()->windowStates() ^ Qt::WindowMaximized);
    } else {
        // Show window menu
        if (b == Qt::MouseButton::RightButton) {
            waylandWindow()->shellSurface()->showWindowMenu(inputDevice);
        }
#if QT_CONFIG(cursor)
        waylandWindow()->restoreMouseCursor(inputDevice);
#endif
        startMove(inputDevice, b);
    }
}

void QGnomePlatformDecoration::processMouseBottom(QWaylandInputDevice *inputDevice, const QPointF &local, Qt::MouseButtons b, Qt::KeyboardModifiers mods)
{
    Q_UNUSED(mods)
    if (local.x() <= margins().left()) {
        // bottom left bit
#if QT_CONFIG(cursor)
        waylandWindow()->setMouseCursor(inputDevice, Qt::SizeBDiagCursor);
#endif
        startResize(inputDevice, Qt::BottomEdge | Qt::LeftEdge, b);
    } else if (local.x() > window()->width() + margins().right()) {
        // bottom right bit
#if QT_CONFIG(cursor)
        waylandWindow()->setMouseCursor(inputDevice, Qt::SizeFDiagCursor);
#endif
        startResize(inputDevice, Qt::BottomEdge | Qt::RightEdge, b);
    } else {
        // bottom bit
#if QT_CONFIG(cursor)
        waylandWindow()->setMouseCursor(inputDevice, Qt::SplitVCursor);
#endif
        startResize(inputDevice, Qt::BottomEdge, b);
    }
}

void QGnomePlatformDecoration::processMouseLeft(QWaylandInputDevice *inputDevice, const QPointF &local, Qt::MouseButtons b, Qt::KeyboardModifiers mods)
{
    Q_UNUSED(local)
    Q_UNUSED(mods)
#if QT_CONFIG(cursor)
    waylandWindow()->setMouseCursor(inputDevice, Qt::SplitHCursor);
#endif
    startResize(inputDevice, Qt::LeftEdge, b);
}

void QGnomePlatformDecoration::processMouseRight(QWaylandInputDevice *inputDevice, const QPointF &local, Qt::MouseButtons b, Qt::KeyboardModifiers mods)
{
    Q_UNUSED(local)
    Q_UNUSED(mods)
#if QT_CONFIG(cursor)
    waylandWindow()->setMouseCursor(inputDevice, Qt::SplitHCursor);
#endif
    startResize(inputDevice, Qt::RightEdge, b);
}

void QGnomePlatformDecoration::renderButton(QPainter *painter, const QRectF &rect, Adwaita::ButtonType button, bool renderFrame, bool sunken)
{
    Qt::WindowStates windowStates;
#ifdef DECORATION_SHADOWS_SUPPORT // Qt 6.2.0+ or patched QtWayland
    windowStates = waylandWindow()->windowStates();
#else
    windowStates = window()->windowStates();
#endif

    const bool active = windowStates & Qt::WindowActive;
    Adwaita::StyleOptions decorationButtonStyle(painter, QRect());
    decorationButtonStyle.setColor(active ? m_foregroundColor : m_foregroundInactiveColor);

    if (renderFrame) {
        QRect buttonRect(static_cast<int>(rect.x()), static_cast<int>(rect.y()), BUTTON_WIDTH, BUTTON_WIDTH);
        Adwaita::StyleOptions styleOptions(painter, buttonRect);
        styleOptions.setMouseOver(true);
        styleOptions.setSunken(sunken);
        styleOptions.setColorVariant(m_adwaitaVariant);
        styleOptions.setColor(Adwaita::Colors::buttonBackgroundColor(styleOptions));
        styleOptions.setOutlineColor(Adwaita::Colors::buttonOutlineColor(styleOptions));
        Adwaita::Renderer::renderFlatRoundedButtonFrame(styleOptions);
    }
    decorationButtonStyle.setRect(
        QRect(static_cast<int>(rect.x()) + (BUTTON_WIDTH / 4), static_cast<int>(rect.y()) + (BUTTON_WIDTH / 4), BUTTON_WIDTH / 2, BUTTON_WIDTH / 2));
    Adwaita::Renderer::renderDecorationButton(decorationButtonStyle, button);
}

bool QGnomePlatformDecoration::updateButtonHoverState(Button hoveredButton)
{
    bool currentCloseButtonState = m_closeButtonHovered;
    bool currentMaximizeButtonState = m_maximizeButtonHovered;
    bool currentMinimizeButtonState = m_minimizeButtonHovered;

    m_closeButtonHovered = hoveredButton == Button::Close;
    m_maximizeButtonHovered = hoveredButton == Button::Maximize;
    m_minimizeButtonHovered = hoveredButton == Button::Minimize;

    if (m_closeButtonHovered != currentCloseButtonState || m_maximizeButtonHovered != currentMaximizeButtonState
        || m_minimizeButtonHovered != currentMinimizeButtonState) {
        forceRepaint();
        return true;
    }

    return false;
}
