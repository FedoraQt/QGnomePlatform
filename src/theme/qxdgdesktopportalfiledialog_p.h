/*
 * Copyright (C) 2017-2021 Red Hat, Inc
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

#ifndef QXDGDESKTOPPORTALFILEDIALOG_P_H
#define QXDGDESKTOPPORTALFILEDIALOG_P_H

#include <qpa/qplatformdialoghelper.h>
#include <QVector>

QT_BEGIN_NAMESPACE

class QXdgDesktopPortalFileDialogPrivate;

class QXdgDesktopPortalFileDialog : public QPlatformFileDialogHelper
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QXdgDesktopPortalFileDialog)
public:
    enum ConditionType : uint {
        GlobalPattern = 0,
        MimeType = 1
    };
    // Filters a(sa(us))
    // Example: [('Images', [(0, '*.ico'), (1, 'image/png')]), ('Text', [(0, '*.txt')])]
    struct FilterCondition {
        ConditionType type;
        QString pattern; // E.g. '*ico' or 'image/png'
    };
    typedef QVector<FilterCondition> FilterConditionList;

    struct Filter {
        QString name; // E.g. 'Images' or 'Text
        FilterConditionList filterConditions;; // E.g. [(0, '*.ico'), (1, 'image/png')] or [(0, '*.txt')]
    };
    typedef QVector<Filter> FilterList;

    QXdgDesktopPortalFileDialog(QPlatformFileDialogHelper *nativeFileDialog = nullptr);
    ~QXdgDesktopPortalFileDialog();

    bool defaultNameFilterDisables() const override;
    QUrl directory() const override;
    void setDirectory(const QUrl &directory) override;
    void selectFile(const QUrl &filename) override;
    QList<QUrl> selectedFiles() const override;
    void setFilter() override;
    void selectNameFilter(const QString &filter) override;
    QString selectedNameFilter() const override;
    void selectMimeTypeFilter(const QString &filter) override;
    QString selectedMimeTypeFilter() const override;

    void exec() override;
    bool show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent) override;
    void hide() override;

private Q_SLOTS:
    void gotResponse(uint response, const QVariantMap &results);

private:
    void initializeDialog();
    void openPortal();

    QScopedPointer<QXdgDesktopPortalFileDialogPrivate> d_ptr;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QXdgDesktopPortalFileDialog::FilterCondition);
Q_DECLARE_METATYPE(QXdgDesktopPortalFileDialog::FilterConditionList);
Q_DECLARE_METATYPE(QXdgDesktopPortalFileDialog::Filter);
Q_DECLARE_METATYPE(QXdgDesktopPortalFileDialog::FilterList);

#endif // QXDGDESKTOPPORTALFILEDIALOG_P_H

