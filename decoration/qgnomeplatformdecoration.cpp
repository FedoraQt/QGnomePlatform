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

static const int buttonSpacing  = 8;
static const int buttonWidth = 26;
static const int buttonsRightMargin = 6;

static const int bordersWidth = 1;
#if QT_VERSION > QT_VERSION_CHECK(5, 12, 0)
static const int shadowsWidth = 10;
#else
static const int shadowsWidth = 0;
#endif

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

    m_lastButtonClick = QDateTime::currentDateTime();

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
        return QRectF(window()->frameGeometry().width() - buttonWidth - buttonSpacing * 0 - buttonsRightMargin - shadowsWidth,
                      (margins().top() - buttonWidth + shadowsWidth) / 2, buttonWidth, buttonWidth);
    } else {
        return QRectF(buttonSpacing * 0 + buttonsRightMargin + shadowsWidth,
                      (margins().top() - buttonWidth + shadowsWidth) / 2, buttonWidth, buttonWidth);
    }
}

QRectF QGnomePlatformDecoration::maximizeButtonRect() const
{
    if (m_hints->titlebarButtonPlacement() == GnomeHintsSettings::RightPlacement) {
        return QRectF(window()->frameGeometry().width() - buttonWidth * 2 - buttonSpacing * 1 - buttonsRightMargin - shadowsWidth,
                      (margins().top() - buttonWidth + shadowsWidth) / 2, buttonWidth, buttonWidth);
    } else {
        return QRectF(buttonWidth * 1 + buttonSpacing * 1 + buttonsRightMargin + shadowsWidth,
                      (margins().top() - buttonWidth + shadowsWidth) / 2, buttonWidth, buttonWidth);
    }
}

QRectF QGnomePlatformDecoration::minimizeButtonRect() const
{
    const bool maximizeEnabled = m_hints->titlebarButtons().testFlag(GnomeHintsSettings::MaximizeButton);

    if (m_hints->titlebarButtonPlacement() == GnomeHintsSettings::RightPlacement) {
        return QRectF(window()->frameGeometry().width() - buttonWidth * (maximizeEnabled ? 3 : 2) - buttonSpacing * (maximizeEnabled ? 2 : 1) - buttonsRightMargin - shadowsWidth,
                      (margins().top() - buttonWidth + shadowsWidth) / 2, buttonWidth, buttonWidth);
    } else {
        return QRectF(buttonWidth * (maximizeEnabled ? 2 : 1) + buttonSpacing * (maximizeEnabled ? 2 : 1) + buttonsRightMargin + shadowsWidth,
                      (margins().top() - buttonWidth + shadowsWidth) / 2, buttonWidth, buttonWidth);
    }
}

#if QT_VERSION > QT_VERSION_CHECK(5, 12, 0)
QMargins QGnomePlatformDecoration::margins(bool excludeShadows) const
#else
QMargins QGnomePlatformDecoration::margins() const
#endif
{
    QMargins margins(1, 38, 1, 1);
#if QT_VERSION > QT_VERSION_CHECK(5, 12, 0)

    if (excludeShadows)
        return margins;

    return margins + shadowsWidth;
#else
    return margins;
#endif
}

/* Border region
*  -----------
*  |         |
*  |         |
*  |         |
*  |         |
*  |         |
*  -----------
*/
static QRegion borderRegion(const QSize &size, const QMargins &margins)
{
    QRegion r;
    // Top
    r += QRect(shadowsWidth, margins.top() - bordersWidth, size.width() - (2 * shadowsWidth), bordersWidth);
    // Left
    r += QRect(shadowsWidth, margins.top(), bordersWidth, size.height() - margins.top() - shadowsWidth - bordersWidth);
    // Bottom
    r += QRect(shadowsWidth, size.height() - margins.bottom(), size.width() - (2 * shadowsWidth), bordersWidth);
    // Right
    r += QRect(size.width() - margins.right(), margins.top(), bordersWidth, size.height() - margins.top() - shadowsWidth - bordersWidth);
    return r;
}

void QGnomePlatformDecoration::paint(QPaintDevice *device)
{
    bool active = window()->handle()->isActive();
    QRect surfaceRect(QPoint(), window()->frameGeometry().size());

    QPainter p(device);
    p.setRenderHint(QPainter::Antialiasing);

    /*       Draw shadows   */
    /*************************/
    p.save();
    p.setPen(Qt::NoPen);
    p.setCompositionMode(QPainter::CompositionMode_Source);

    QColor beginShadowColor = active ? transparentize(QColor("black"), 0.85) : transparentize(QColor("black"), 0.90);
    QColor endShadowColor = active ? transparentize(QColor("black"), 1) : transparentize(QColor("black"), 1);

    QLinearGradient gradient;

    gradient.setColorAt(0.0, beginShadowColor);
    gradient.setColorAt(1.0, endShadowColor);

    // Right
    QPointF right0(surfaceRect.width() - shadowsWidth, surfaceRect.height() / 2);
    QPointF right1(surfaceRect.width(), surfaceRect.height() / 2);
    gradient.setStart(right0);
    gradient.setFinalStop(right1);
    p.setBrush(QBrush(gradient));
    p.drawRoundRect(QRectF(QPointF(surfaceRect.width() - (2 * shadowsWidth), shadowsWidth), QPointF(surfaceRect.width(), surfaceRect.height() - shadowsWidth)), 0.0, 0.0);

    // Left
    QPointF left0(shadowsWidth, surfaceRect.height() / 2);
    QPointF left1(0, surfaceRect.height() / 2);
    gradient.setStart(left0);
    gradient.setFinalStop(left1);
    p.setBrush(QBrush(gradient));
    p.drawRoundRect(QRectF(QPointF(2 * shadowsWidth, shadowsWidth), QPointF(0, surfaceRect.height() - shadowsWidth)), 0.0, 0.0);

    // Top
    QPointF top0(surfaceRect.width() / 2, shadowsWidth);
    QPointF top1(surfaceRect.width() / 2, 0);
    gradient.setStart(top0);
    gradient.setFinalStop(top1);
    p.setBrush(QBrush(gradient));
    p.drawRoundRect(QRectF(QPointF(surfaceRect.width() - shadowsWidth, 0), QPointF(shadowsWidth, 2 * shadowsWidth)), 0.0, 0.0);

    // Bottom
    QPointF bottom0(surfaceRect.width() / 2, surfaceRect.height() - shadowsWidth);
    QPointF bottom1(surfaceRect.width() / 2, surfaceRect.height());
    gradient.setStart(bottom0);
    gradient.setFinalStop(bottom1);
    p.setBrush(QBrush(gradient));
    p.drawRoundRect(QRectF(QPointF(shadowsWidth, surfaceRect.height() - (2 * shadowsWidth)), QPointF(surfaceRect.width() - shadowsWidth, surfaceRect.height())), 0.0, 0.0);

    // BottomRight
    QPointF bottomright0(surfaceRect.width() - shadowsWidth, surfaceRect.height() - shadowsWidth);
    QPointF bottomright1(surfaceRect.width(), surfaceRect.height());
    gradient.setStart(bottomright0);
    gradient.setFinalStop(bottomright1);
    gradient.setColorAt(0.55, transparentize(endShadowColor, 0.9));
    p.setBrush(QBrush(gradient));
    p.drawRoundRect(QRectF(bottomright0, bottomright1), 0.0, 0.0);

    // BottomLeft
    QPointF bottomleft0(shadowsWidth, surfaceRect.height() - shadowsWidth);
    QPointF bottomleft1(0, surfaceRect.height());
    gradient.setStart(bottomleft0);
    gradient.setFinalStop(bottomleft1);
    gradient.setColorAt(0.55, transparentize(endShadowColor, 0.9));
    p.setBrush(QBrush(gradient));
    p.drawRoundRect(QRectF(bottomleft0, bottomleft1), 0.0, 0.0);

    // TopLeft
    QPointF topleft0(shadowsWidth, shadowsWidth);
    QPointF topleft1(0, 0);
    gradient.setStart(topleft0);
    gradient.setFinalStop(topleft1);
    gradient.setColorAt(0.55, transparentize(endShadowColor, 0.9));
    p.setBrush(QBrush(gradient));
    p.drawRoundRect(QRectF(topleft0, topleft1), 0.0, 0.0);

    // TopRight
    QPointF topright0(surfaceRect.width() - shadowsWidth, shadowsWidth);
    QPointF topright1(surfaceRect.width(), 0);
    gradient.setStart(topright0);
    gradient.setFinalStop(topright1);
    gradient.setColorAt(0.55, transparentize(endShadowColor, 0.9));
    p.setBrush(QBrush(gradient));
    p.drawRoundRect(QRectF(topright0, topright1), 0.0, 0.0);

    p.restore();

    /*    Draw borders   */
    /*********************/

    // Title bar (border)
    QPainterPath borderRect;
    if ((window()->windowStates() & Qt::WindowMaximized))
        borderRect.addRect(shadowsWidth, shadowsWidth, surfaceRect.width() - (2 * shadowsWidth), margins().top() + 8);
    else
        borderRect.addRoundedRect(shadowsWidth, shadowsWidth, surfaceRect.width() - (2 * shadowsWidth), margins().top() + 8, 10, 10);

    p.fillPath(borderRect.simplified(), active ? m_borderColor : m_borderInactiveColor);

    // Left/Bottom/Right border
    QPainterPath borderPath;
    borderPath.addRegion(borderRegion(QSize(surfaceRect.width(), surfaceRect.height()), margins()));
    p.fillPath(borderPath, active ? m_borderColor : m_borderInactiveColor);

    // Title bar
    QPainterPath roundedRect;
    if ((window()->windowStates() & Qt::WindowMaximized))
        roundedRect.addRect(shadowsWidth + bordersWidth, shadowsWidth + bordersWidth, surfaceRect.width() - margins().left() - margins().right(), margins().top());
    else
        roundedRect.addRoundedRect(shadowsWidth + bordersWidth, shadowsWidth + bordersWidth, surfaceRect.width() - margins().left() - margins().right(), margins().top(), 8, 8);
    QLinearGradient titleBarGradient(margins().left(), margins().top() + 6, margins().left(), 1);
    titleBarGradient.setColorAt(0, active ? m_backgroundColorStart : m_backgroundInactiveColor);
    titleBarGradient.setColorAt(1, active ? m_backgroundColorEnd : m_backgroundInactiveColor);
    p.fillPath(roundedRect.simplified(), titleBarGradient);

    // *********************************************************************************************** //

    QRect top = QRect(0, 0, surfaceRect.width(), margins().top());

    /*    Window title   */
    /*********************/

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
        int dx = (top.width() - size.width() + (2 * shadowsWidth)) / 2;
        int dy = (top.height() - size.height() + shadowsWidth) / 2;
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

    /*    Buttons   */
    /*********************/

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

    QDateTime currentDateTime = QDateTime::currentDateTime();

    if (!closeButtonRect().contains(local) && !maximizeButtonRect().contains(local) && !minimizeButtonRect().contains(local)) {
        updateButtonHoverState(Button::None);
    }

    if (local.y() <= margins().bottom() && !(window()->windowStates() & Qt::WindowMaximized)) {
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
        if (clickButton(b, Maximize)) {
            const int doubleClickDistance = m_hints->hint(QPlatformTheme::MouseDoubleClickDistance).toInt();
            QPointF posDiff = m_lastButtonClickPosition - local;
            if ((m_lastButtonClick.msecsTo(currentDateTime) <= m_hints->hint(QPlatformTheme::MouseDoubleClickInterval).toInt()) &&
                ((posDiff.x() <= doubleClickDistance && posDiff.x() >= -doubleClickDistance) && ((posDiff.y() <= doubleClickDistance && posDiff.y() >= -doubleClickDistance))))
                window()->setWindowStates(window()->windowStates() ^ Qt::WindowMaximized);
            m_lastButtonClick = currentDateTime;
            m_lastButtonClickPosition = local;
        } else {
#if QT_CONFIG(cursor)
            waylandWindow()->restoreMouseCursor(inputDevice);
#endif
            startMove(inputDevice,b);
        }
    }
}

void QGnomePlatformDecoration::processMouseBottom(QWaylandInputDevice *inputDevice, const QPointF &local, Qt::MouseButtons b, Qt::KeyboardModifiers mods)
{
    Q_UNUSED(mods);

    if ((window()->windowStates() & Qt::WindowMaximized))
        return;

    if (local.x() <= margins().left() ) {
        //bottom left bit
#if QT_CONFIG(cursor)
        waylandWindow()->setMouseCursor(inputDevice, Qt::SizeBDiagCursor);
#endif
        startResize(inputDevice, WL_SHELL_SURFACE_RESIZE_BOTTOM_LEFT,b);
    } else if (local.x() > window()->width() + margins().right()) {
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

    if ((window()->windowStates() & Qt::WindowMaximized))
        return;

#if QT_CONFIG(cursor)
    waylandWindow()->setMouseCursor(inputDevice, Qt::SplitHCursor);
#endif
    startResize(inputDevice,WL_SHELL_SURFACE_RESIZE_LEFT,b);
}

void QGnomePlatformDecoration::processMouseRight(QWaylandInputDevice *inputDevice, const QPointF &local, Qt::MouseButtons b, Qt::KeyboardModifiers mods)
{
    Q_UNUSED(local);
    Q_UNUSED(mods);

    if ((window()->windowStates() & Qt::WindowMaximized))
        return;

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
