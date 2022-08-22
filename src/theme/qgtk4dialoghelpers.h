/****************************************************************************
**
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

#ifndef QGTK4DIALOGHELPERS_H
#define QGTK4DIALOGHELPERS_H

#include <QtCore/qhash.h>
#include <QtCore/qlist.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qstring.h>
#include <QtCore/qurl.h>
#include <qpa/qplatformdialoghelper.h>

typedef struct _GtkWidget GtkWidget;
typedef struct _GtkImage GtkImage;
typedef struct _GtkDialog GtkDialog;
typedef struct _GtkFileFilter GtkFileFilter;

QT_BEGIN_NAMESPACE

class QGtk4Dialog;
class QColor;

class QGtk4ColorDialogHelper : public QPlatformColorDialogHelper
{
    Q_OBJECT

public:
    QGtk4ColorDialogHelper();
    ~QGtk4ColorDialogHelper();

    bool show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent) Q_DECL_OVERRIDE;
    void exec() Q_DECL_OVERRIDE;
    void hide() Q_DECL_OVERRIDE;

    void setCurrentColor(const QColor &color) Q_DECL_OVERRIDE;
    QColor currentColor() const Q_DECL_OVERRIDE;

private Q_SLOTS:
    void onAccepted();

private:
    static void onColorChanged(QGtk4ColorDialogHelper *helper);
    void applyOptions();

    QScopedPointer<QGtk4Dialog> d;
};

class QGtk4FileDialogHelper : public QPlatformFileDialogHelper
{
    Q_OBJECT

public:
    QGtk4FileDialogHelper();
    ~QGtk4FileDialogHelper();

    GtkImage *previewImage() const;

    bool show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent) Q_DECL_OVERRIDE;
    void exec() Q_DECL_OVERRIDE;
    void hide() Q_DECL_OVERRIDE;

    bool defaultNameFilterDisables() const Q_DECL_OVERRIDE;
    void setDirectory(const QUrl &directory) Q_DECL_OVERRIDE;
    QUrl directory() const Q_DECL_OVERRIDE;
    void selectFile(const QUrl &filename) Q_DECL_OVERRIDE;
    QList<QUrl> selectedFiles() const Q_DECL_OVERRIDE;
    void setFilter() Q_DECL_OVERRIDE;
    void selectNameFilter(const QString &filter) Q_DECL_OVERRIDE;
    QString selectedNameFilter() const Q_DECL_OVERRIDE;

private Q_SLOTS:
    void onAccepted();

private:
    static void onSelectionChanged(GtkDialog *dialog, QGtk4FileDialogHelper *helper);
    static void onCurrentFolderChanged(QGtk4FileDialogHelper *helper);
    static void onUpdatePreview(GtkDialog *dialog, QGtk4FileDialogHelper *helper);
    void applyOptions();
    void setNameFilters(const QStringList &filters);

    QUrl _dir;
    QList<QUrl> _selection;
    QHash<QString, GtkFileFilter *> _filters;
    QHash<GtkFileFilter *, QString> _filterNames;
    QScopedPointer<QGtk4Dialog> d;
    GtkWidget *previewWidget;
};

class QGtk4FontDialogHelper : public QPlatformFontDialogHelper
{
    Q_OBJECT

public:
    QGtk4FontDialogHelper();
    ~QGtk4FontDialogHelper();

    bool show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent) Q_DECL_OVERRIDE;
    void exec() Q_DECL_OVERRIDE;
    void hide() Q_DECL_OVERRIDE;

    void setCurrentFont(const QFont &font) Q_DECL_OVERRIDE;
    QFont currentFont() const Q_DECL_OVERRIDE;

private Q_SLOTS:
    void onAccepted();

private:
    void applyOptions();

    QScopedPointer<QGtk4Dialog> d;
};

QT_END_NAMESPACE

#endif // QGTK4DIALOGHELPERS_H
