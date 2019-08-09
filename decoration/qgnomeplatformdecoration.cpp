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
#include <QtGui/QLinearGradient>
#include <QtGui/QPainter>
#include <QtGui/QPalette>
#include <QtGui/QPixmap>

#include <qpa/qwindowsysteminterface.h>

#include <QtWaylandClient/private/qwaylandwindow_p.h>
#include <QtWaylandClient/private/qwaylandshellsurface_p.h>

#define BUTTON_SPACING 8
#define BUTTON_WIDTH 26
#define BUTTONS_RIGHT_MARGIN 6

// Copied from adwaita-qt
static QColor transparentize(const QColor &color, qreal amount = 0.1)
{
    qreal h, s, l, a;
    color.getHslF(&h, &s, &l, &a);

    qreal alpha = a - amount;
    if (alpha < 0)
        alpha = 0;
    return QColor::fromHslF(h, s, l, alpha);
}

static QColor darken(const QColor &color, qreal amount = 0.1)
{
    qreal h, s, l, a;
    color.getHslF(&h, &s, &l, &a);

    qreal lightness = l - amount;
    if (lightness < 0)
        lightness = 0;

    return QColor::fromHslF(h, s, lightness, a);
}

static QColor desaturate(const QColor &color, qreal amount = 0.1)
{
    qreal h, s, l, a;
    color.getHslF(&h, &s, &l, &a);

    qreal saturation = s - amount;
    if (saturation < 0)
        saturation = 0;
    return QColor::fromHslF(h, saturation, l, a);
}

QGnomePlatformDecoration::QGnomePlatformDecoration()
    : m_closeButtonHovered(false)
    , m_maximizeButtonHovered(false)
    , m_minimizeButtonHovered(false)
    , m_hints(new GnomeHintsSettings)
{
    initializeButtonPixmaps();
    initializeColors();

    QTextOption option(Qt::AlignHCenter | Qt::AlignVCenter);
    option.setWrapMode(QTextOption::NoWrap);
    m_windowTitle.setTextOption(option);
}

QGnomePlatformDecoration::~QGnomePlatformDecoration()
{
    delete m_hints;
}

void QGnomePlatformDecoration::initializeButtonPixmaps()
{
    const QString iconTheme = m_hints->hint(QPlatformTheme::SystemIconThemeName).toString();
    const bool isAdwaitaIconTheme = iconTheme.toLower() == QStringLiteral("adwaita");
    const bool isDarkVariant = m_hints->gtkThemeDarkVariant();

    QIcon::setThemeName(m_hints->hint(QPlatformTheme::SystemIconThemeName).toString());

    QPixmap closeIcon = QIcon::fromTheme(QStringLiteral("window-close-symbolic"), QIcon::fromTheme(QStringLiteral("window-close"))).pixmap(QSize(16, 16));
    QPixmap maximizeIcon = QIcon::fromTheme(QStringLiteral("window-maximize-symbolic"), QIcon::fromTheme(QStringLiteral("window-maximize"))).pixmap(QSize(16, 16));
    QPixmap minimizeIcon = QIcon::fromTheme(QStringLiteral("window-minimize-symbolic"), QIcon::fromTheme(QStringLiteral("window-minimize"))).pixmap(QSize(16, 16));
    QPixmap restoreIcon = QIcon::fromTheme(QStringLiteral("window-restore-symbolic"), QIcon::fromTheme(QStringLiteral("window-restore"))).pixmap(QSize(16, 16));

    m_buttonPixmaps.insert(Button::Close, isAdwaitaIconTheme && isDarkVariant ? pixmapDarkVariant(closeIcon) : closeIcon);
    m_buttonPixmaps.insert(Button::Maximize, isAdwaitaIconTheme && isDarkVariant ? pixmapDarkVariant(maximizeIcon) : maximizeIcon);
    m_buttonPixmaps.insert(Button::Minimize, isAdwaitaIconTheme && isDarkVariant ? pixmapDarkVariant(minimizeIcon) : minimizeIcon);
    m_buttonPixmaps.insert(Button::Restore, isAdwaitaIconTheme && isDarkVariant ? pixmapDarkVariant(restoreIcon) : restoreIcon);
}

void QGnomePlatformDecoration::initializeColors()
{
    const bool darkVariant = m_hints->gtkThemeDarkVariant();
    m_foregroundColor         = darkVariant ? QColor("#eeeeec") : QColor("#2e3436"); // Adwaita fg_color
    m_backgroundColorStart    = darkVariant ? QColor("#262626") : QColor("#dad6d2"); // Adwaita GtkHeaderBar color
    m_backgroundColorEnd      = darkVariant ? QColor("#2b2b2b") : QColor("#e1dedb"); // Adwaita GtkHeaderBar color
    m_foregroundInactiveColor = darkVariant ? QColor("#919190") : QColor("#929595");
    m_backgroundInactiveColor = darkVariant ? QColor("#353535") : QColor("#f6f5f4");
    m_borderColor             = darkVariant ? transparentize(QColor("#1b1b1b"), 0.1) : transparentize(QColor("black"), 0.77);
    m_borderInactiveColor     = darkVariant ? transparentize(QColor("#1b1b1b"), 0.1) : transparentize(QColor("black"), 0.82);
}

QPixmap QGnomePlatformDecoration::pixmapDarkVariant(const QPixmap &pixmap)
{
    // FIXME: dark variant colors are probably not 1:1, but this is the easiest and most reliable approach for now
    QImage image = pixmap.toImage();
    image.invertPixels();
    return QPixmap::fromImage(image);
}

QRectF QGnomePlatformDecoration::closeButtonRect() const
{
    if (m_hints->titlebarButtonPlacement() == GnomeHintsSettings::RightPlacement) {
        return QRectF(window()->frameGeometry().width() - BUTTON_WIDTH - BUTTON_SPACING * 0 - BUTTONS_RIGHT_MARGIN,
                      (margins().top() - BUTTON_WIDTH) / 2, BUTTON_WIDTH, BUTTON_WIDTH);
    } else {
        return QRectF(BUTTON_SPACING * 0 + BUTTONS_RIGHT_MARGIN,
                      (margins().top() - BUTTON_WIDTH) / 2, BUTTON_WIDTH, BUTTON_WIDTH);
    }
}

QRectF QGnomePlatformDecoration::maximizeButtonRect() const
{
    if (m_hints->titlebarButtonPlacement() == GnomeHintsSettings::RightPlacement) {
        return QRectF(window()->frameGeometry().width() - BUTTON_WIDTH * 2 - BUTTON_SPACING * 1 - BUTTONS_RIGHT_MARGIN,
                      (margins().top() - BUTTON_WIDTH) / 2, BUTTON_WIDTH, BUTTON_WIDTH);
    } else {
        return QRectF(BUTTON_WIDTH * 1 + BUTTON_SPACING * 1 + BUTTONS_RIGHT_MARGIN,
                      (margins().top() - BUTTON_WIDTH) / 2, BUTTON_WIDTH, BUTTON_WIDTH);
    }
}

QRectF QGnomePlatformDecoration::minimizeButtonRect() const
{
    const bool maximizeEnabled = m_hints->titlebarButtons().testFlag(GnomeHintsSettings::MaximizeButton);

    if (m_hints->titlebarButtonPlacement() == GnomeHintsSettings::RightPlacement) {
        return QRectF(window()->frameGeometry().width() - BUTTON_WIDTH * (maximizeEnabled ? 3 : 2) - BUTTON_SPACING * (maximizeEnabled ? 2 : 1) - BUTTONS_RIGHT_MARGIN,
                      (margins().top() - BUTTON_WIDTH) / 2, BUTTON_WIDTH, BUTTON_WIDTH);
    } else {
        return QRectF(BUTTON_WIDTH * (maximizeEnabled ? 2 : 1) + BUTTON_SPACING * (maximizeEnabled ? 2 : 1) + BUTTONS_RIGHT_MARGIN,
                      (margins().top() - BUTTON_WIDTH) / 2, BUTTON_WIDTH, BUTTON_WIDTH);
    }
}

QMargins QGnomePlatformDecoration::margins() const
{
    return QMargins(1, 38, 1, 1);
}

void QGnomePlatformDecoration::paint(QPaintDevice *device)
{
    bool active = window()->handle()->isActive();
    QRect surfaceRect(QPoint(), window()->frameGeometry().size());

    QPainter p(device);
    p.setRenderHint(QPainter::Antialiasing);

    // Title bar (border)
    QPainterPath borderRect;
    if ((window()->windowStates() & Qt::WindowMaximized))
        borderRect.addRect(0, 0, surfaceRect.width(), margins().top() + 8);
    else
        borderRect.addRoundedRect(0, 0, surfaceRect.width(), margins().top() + 8, 10, 10);

    p.fillPath(borderRect.simplified(), active ? m_borderColor : m_borderInactiveColor);

    // Title bar
    QPainterPath roundedRect;
    if ((window()->windowStates() & Qt::WindowMaximized))
        roundedRect.addRect(1, 1, surfaceRect.width() - margins().left() - margins().right(), margins().top() + 8);
    else
        roundedRect.addRoundedRect(1, 1, surfaceRect.width() - margins().left() - margins().right(), margins().top() + 8, 8, 8);

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
        if (m_hints->titlebarButtonPlacement() == GnomeHintsSettings::RightPlacement) {
            titleBar.setLeft(margins().left());
            titleBar.setRight(minimizeButtonRect().left() - 8);
        } else {
            titleBar.setLeft(minimizeButtonRect().right() + 8);
            titleBar.setRight(surfaceRect.width() - margins().right());
        }

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

    // From adwaita-qt
    QColor windowColor;
    QColor buttonHoverBorderColor;
    // QColor buttonHoverFrameColor;
    if (m_hints->gtkThemeDarkVariant()) {
        windowColor = darken(desaturate(QColor("#3d3846"), 1.0), 0.04);
        buttonHoverBorderColor = darken(windowColor, 0.1);
        // buttonHoverFrameColor = darken(windowColor, 0.01);
    } else {
        windowColor = QColor("#f6f5f4");
        buttonHoverBorderColor = darken(windowColor, 0.18);
        // buttonHoverFrameColor = darken(windowColor, 0.04);
    }

    // Close button
    p.save();
    rect = closeButtonRect();
    if (m_closeButtonHovered) {
        QRectF buttonRect(rect.x() - 0.5, rect.y() - 0.5, 28, 28);
        // QLinearGradient buttonGradient(buttonRect.bottomLeft(), buttonRect.topLeft());
        // buttonGradient.setColorAt(0, buttonHoverFrameColor);
        // buttonGradient.setColorAt(1, windowColor);
        QPainterPath path;
        path.addRoundedRect(buttonRect, 4, 4);
        p.setPen(QPen(buttonHoverBorderColor, 1.0));
        p.fillPath(path, windowColor);
        p.drawPath(path);
    }
    p.drawPixmap(QPoint(rect.x() + 6, rect.y() + 6), m_buttonPixmaps[Button::Close]);

    p.restore();

    // Maximize button
    if (m_hints->titlebarButtons().testFlag(GnomeHintsSettings::MaximizeButton)) {
        p.save();
        rect = maximizeButtonRect();
        if (m_maximizeButtonHovered) {
            QRectF buttonRect(rect.x() - 0.5, rect.y() - 0.5, 28, 28);
            // QLinearGradient buttonGradient(buttonRect.bottomLeft(), buttonRect.topLeft());
            // buttonGradient.setColorAt(0, buttonHoverFrameColor);
            // buttonGradient.setColorAt(1, windowColor);
            QPainterPath path;
            path.addRoundedRect(buttonRect, 4, 4);
            p.setPen(QPen(buttonHoverBorderColor, 1.0));
            p.fillPath(path, windowColor);
            p.drawPath(path);
        }
        if ((window()->windowStates() & Qt::WindowMaximized)) {
            p.drawPixmap(QPoint(rect.x() + 5, rect.y() + 5), m_buttonPixmaps[Button::Restore]);
        } else {
            p.drawPixmap(QPoint(rect.x() + 5, rect.y() + 5), m_buttonPixmaps[Button::Maximize]);
        }
        p.restore();
    }

    // Minimize button
    if (m_hints->titlebarButtons().testFlag(GnomeHintsSettings::MinimizeButton)) {
        p.save();
        rect = minimizeButtonRect();
        if (m_minimizeButtonHovered) {
            QRectF buttonRect(rect.x() - 0.5, rect.y() - 0.5, 28, 28);
            // QLinearGradient buttonGradient(buttonRect.bottomLeft(), buttonRect.topLeft());
            // buttonGradient.setColorAt(0, buttonHoverFrameColor);
            // buttonGradient.setColorAt(1, windowColor);
            QPainterPath path;
            path.addRoundedRect(buttonRect, 4, 4);
            p.setPen(QPen(buttonHoverBorderColor, 1.0));
            p.fillPath(path, windowColor);
            p.drawPath(path);
        }
        p.drawPixmap(QPoint(rect.x() + 5, rect.y() + 5), m_buttonPixmaps[Button::Minimize]);
        p.restore();
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

bool QGnomePlatformDecoration::handleMouse(QWaylandInputDevice *inputDevice, const QPointF &local, const QPointF &global, Qt::MouseButtons b, Qt::KeyboardModifiers mods)
{
    Q_UNUSED(global);

    if (local.y() > margins().top()) {
        updateButtonHoverState(Button::None);
    }

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
        else if (m_hints->titlebarButtons().testFlag(GnomeHintsSettings::MaximizeButton) && maximizeButtonRect().contains(local))
            window()->setWindowStates(window()->windowStates() ^ Qt::WindowMaximized);
        else if (m_hints->titlebarButtons().testFlag(GnomeHintsSettings::MinimizeButton) && minimizeButtonRect().contains(local))
            window()->setWindowState(Qt::WindowMinimized);
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

    if (!closeButtonRect().contains(local) && !maximizeButtonRect().contains(local) && !minimizeButtonRect().contains(local)) {
        updateButtonHoverState(Button::None);
    }

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
        updateButtonHoverState(Button::Close);
        if (clickButton(b, Close))
            QWindowSystemInterface::handleCloseEvent(window());
    }  else if (m_hints->titlebarButtons().testFlag(GnomeHintsSettings::MaximizeButton) && maximizeButtonRect().contains(local)) {
        updateButtonHoverState(Button::Maximize);
        if (clickButton(b, Maximize))
            window()->setWindowStates(window()->windowStates() ^ Qt::WindowMaximized);
    } else if (m_hints->titlebarButtons().testFlag(GnomeHintsSettings::MinimizeButton) && minimizeButtonRect().contains(local)) {
        updateButtonHoverState(Button::Minimize);
        if (clickButton(b, Minimize))
            window()->setWindowState(Qt::WindowMinimized);
    } else {
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

bool QGnomePlatformDecoration::updateButtonHoverState(Button hoveredButton)
{
#if 0
    bool currentCloseButtonState = m_closeButtonHovered;
    bool currentMaximizeButtonState = m_maximizeButtonHovered;
    bool currentMinimizeButtonState = m_maximizeButtonHovered;

    m_closeButtonHovered = hoveredButton == Button::Close;
    m_maximizeButtonHovered = hoveredButton == Button::Maximize;
    m_minimizeButtonHovered = hoveredButton == Button::Minimize;

    if (m_closeButtonHovered != currentCloseButtonState
        || m_maximizeButtonHovered != currentMaximizeButtonState
        || m_minimizeButtonHovered != currentMinimizeButtonState) {
        waylandWindow()->requestUpdate();
        return true;
    }
#endif
    return false;
}
