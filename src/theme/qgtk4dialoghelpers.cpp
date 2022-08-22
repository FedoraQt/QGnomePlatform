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

#include "qgtk4dialoghelpers.h"

#include <qcolor.h>
#include <qdebug.h>
#include <qeventloop.h>
#include <qfileinfo.h>
#include <qfont.h>
#include <qwindow.h>

#include <private/qguiapplication_p.h>
#include <qpa/qplatformfontdatabase.h>

#undef signals
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <pango/pango.h>

QT_BEGIN_NAMESPACE

// GTK file chooser image preview: thanks to Chromium

// The size of the preview we display for selected image files. We set height
// larger than width because generally there is more free space vertically
// than horiztonally (setting the preview image will alway expand the width of
// the dialog, but usually not the height). The image's aspect ratio will always
// be preserved.
#define PREVIEW_WIDTH 256
#define PREVIEW_HEIGHT 512

class QGtk4Dialog : public QWindow
{
    Q_OBJECT

public:
    QGtk4Dialog(GtkWidget *gtkWidget);
    ~QGtk4Dialog();

    GtkDialog *gtkDialog() const;

    void exec();
    bool show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent);
    void hide();

Q_SIGNALS:
    void accept();
    void reject();

protected:
    static void onResponse(QGtk4Dialog *dialog, int response);

private slots:
    void onParentWindowDestroyed();

private:
    GtkWidget *gtkWidget;
};

QGtk4Dialog::QGtk4Dialog(GtkWidget *gtkWidget)
    : gtkWidget(gtkWidget)
{
    g_signal_connect_swapped(G_OBJECT(gtkWidget), "response", G_CALLBACK(onResponse), this);
}

QGtk4Dialog::~QGtk4Dialog()
{
    gtk_clipboard_store(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
    g_object_unref(gtkWidget);
}

GtkDialog *QGtk4Dialog::gtkDialog() const
{
    return GTK_DIALOG(gtkWidget);
}

void QGtk4Dialog::exec()
{
    if (modality() == Qt::ApplicationModal) {
        // block input to the whole app, including other GTK dialogs
        gtk_window_set_modal(GTK_WINDOW(gtkWidget));
    } else {
        // block input to the window, allow input to other GTK dialogs
        QEventLoop loop;
        connect(this, SIGNAL(accept()), &loop, SLOT(quit()));
        connect(this, SIGNAL(reject()), &loop, SLOT(quit()));
        loop.exec();
    }
}

bool QGtk4Dialog::show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent)
{
    if (parent) {
        connect(parent, &QWindow::destroyed, this, &QGtk4Dialog::onParentWindowDestroyed, Qt::UniqueConnection);
    }
    setParent(parent);
    setFlags(flags);
    setModality(modality);

    gtk_widget_realize(gtkWidget); // creates X window

    GdkWindow *gdkWindow = gtk_widget_get_window(gtkWidget);
#ifdef GDK_WINDOWING_X11
    if (parent) {
        GdkDisplay *gdkDisplay = gdk_window_get_display(gdkWindow);
        if (GDK_IS_X11_DISPLAY(gdkDisplay)) {
            XSetTransientForHint(gdk_x11_display_get_xdisplay(gdkDisplay), gdk_x11_window_get_xid(gdkWindow), parent->winId());
        }
    }
#endif

    if (modality != Qt::NonModal) {
        gdk_window_set_modal_hint(gdkWindow, true);
        QGuiApplicationPrivate::showModalWindow(this);
    }

    gtk_widget_show(gtkWidget);
    gdk_window_focus(gdkWindow, GDK_CURRENT_TIME);
    return true;
}

void QGtk4Dialog::hide()
{
    QGuiApplicationPrivate::hideModalWindow(this);
    gtk_widget_hide(gtkWidget);
}

void QGtk4Dialog::onResponse(QGtk4Dialog *dialog, int response)
{
    if (response == GTK_RESPONSE_OK)
        emit dialog->accept();
    else
        emit dialog->reject();
}

void QGtk4Dialog::onParentWindowDestroyed()
{
    // The QGtk4*DialogHelper classes own this object. Make sure the parent doesn't delete it.
    setParent(0);
}

QGtk4ColorDialogHelper::QGtk4ColorDialogHelper()
{
    d.reset(new QGtk4Dialog(gtk_color_chooser_dialog_new("", 0)));
    connect(d.data(), SIGNAL(accept()), this, SLOT(onAccepted()));
    connect(d.data(), SIGNAL(reject()), this, SIGNAL(reject()));

    g_signal_connect_swapped(d->gtkDialog(), "color-activated", G_CALLBACK(onColorChanged), this);
}

QGtk4ColorDialogHelper::~QGtk4ColorDialogHelper()
{
}

bool QGtk4ColorDialogHelper::show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent)
{
    applyOptions();
    return d->show(flags, modality, parent);
}

void QGtk4ColorDialogHelper::exec()
{
    d->exec();
}

void QGtk4ColorDialogHelper::hide()
{
    d->hide();
}

void QGtk4ColorDialogHelper::setCurrentColor(const QColor &color)
{
    GtkDialog *gtkDialog = d->gtkDialog();
    if (color.alpha() < 255)
        gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(gtkDialog), true);
    GdkRGBA gdkColor;
    gdkColor.red = color.redF();
    gdkColor.green = color.greenF();
    gdkColor.blue = color.blueF();
    gdkColor.alpha = color.alphaF();
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(gtkDialog), &gdkColor);
}

QColor QGtk4ColorDialogHelper::currentColor() const
{
    GtkDialog *gtkDialog = d->gtkDialog();
    GdkRGBA gdkColor;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(gtkDialog), &gdkColor);
    return QColor::fromRgbF(gdkColor.red, gdkColor.green, gdkColor.blue, gdkColor.alpha);
}

void QGtk4ColorDialogHelper::onAccepted()
{
    const QColor color = currentColor();
    emit currentColorChanged(color);
    emit accept();
}

void QGtk4ColorDialogHelper::onColorChanged(QGtk4ColorDialogHelper *dialog)
{
    emit dialog->currentColorChanged(dialog->currentColor());
}

void QGtk4ColorDialogHelper::applyOptions()
{
    GtkDialog *gtkDialog = d->gtkDialog();
    gtk_window_set_title(GTK_WINDOW(gtkDialog), options()->windowTitle().toUtf8());

    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(gtkDialog), options()->testOption(QColorDialogOptions::ShowAlphaChannel));
}

QGtk4FileDialogHelper::QGtk4FileDialogHelper()
{
    d.reset(new QGtk4Dialog(
        gtk_file_chooser_dialog_new("", 0, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL)));
    connect(d.data(), SIGNAL(accept()), this, SLOT(onAccepted()));
    connect(d.data(), SIGNAL(reject()), this, SIGNAL(reject()));

    g_signal_connect(GTK_FILE_CHOOSER(d->gtkDialog()), "selection-changed", G_CALLBACK(onSelectionChanged), this);
    g_signal_connect_swapped(GTK_FILE_CHOOSER(d->gtkDialog()), "current-folder-changed", G_CALLBACK(onCurrentFolderChanged), this);

    previewWidget = gtk_image_new();
    g_signal_connect(G_OBJECT(d->gtkDialog()), "update-preview", G_CALLBACK(onUpdatePreview), this);
    gtk_file_chooser_set_preview_widget(GTK_FILE_CHOOSER(d->gtkDialog()), previewWidget);
}

QGtk4FileDialogHelper::~QGtk4FileDialogHelper()
{
}

GtkImage *QGtk4FileDialogHelper::previewImage() const
{
    return GTK_IMAGE(previewWidget);
}

bool QGtk4FileDialogHelper::show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent)
{
    _dir.clear();
    _selection.clear();

    applyOptions();
    return d->show(flags, modality, parent);
}

void QGtk4FileDialogHelper::exec()
{
    d->exec();
}

void QGtk4FileDialogHelper::hide()
{
    // After GtkFileChooserDialog has been hidden, gtk_file_chooser_get_current_folder()
    // & gtk_file_chooser_get_filenames() will return bogus values -> cache the actual
    // values before hiding the dialog
    _dir = directory();
    _selection = selectedFiles();

    d->hide();
}

bool QGtk4FileDialogHelper::defaultNameFilterDisables() const
{
    return false;
}

void QGtk4FileDialogHelper::setDirectory(const QUrl &directory)
{
    GtkDialog *gtkDialog = d->gtkDialog();
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(gtkDialog), directory.toLocalFile().toUtf8());
}

QUrl QGtk4FileDialogHelper::directory() const
{
    // While GtkFileChooserDialog is hidden, gtk_file_chooser_get_current_folder()
    // returns a bogus value -> return the cached value before hiding
    if (!_dir.isEmpty())
        return _dir;

    QString ret;
    GtkDialog *gtkDialog = d->gtkDialog();
    gchar *folder = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(gtkDialog));
    if (folder) {
        ret = QString::fromUtf8(folder);
        g_free(folder);
    }
    return QUrl::fromLocalFile(ret);
}

void QGtk4FileDialogHelper::selectFile(const QUrl &filename)
{
    GtkDialog *gtkDialog = d->gtkDialog();
    if (options()->acceptMode() == QFileDialogOptions::AcceptSave) {
        QFileInfo fi(filename.toLocalFile());
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(gtkDialog), fi.path().toUtf8());
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(gtkDialog), fi.fileName().toUtf8());
    } else {
        gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(gtkDialog), filename.toLocalFile().toUtf8());
    }
}

QList<QUrl> QGtk4FileDialogHelper::selectedFiles() const
{
    // While GtkFileChooserDialog is hidden, gtk_file_chooser_get_filenames()
    // returns a bogus value -> return the cached value before hiding
    if (!_selection.isEmpty())
        return _selection;

    QList<QUrl> selection;
    GtkDialog *gtkDialog = d->gtkDialog();
    GSList *filenames = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(gtkDialog));
    for (GSList *it = filenames; it; it = it->next)
        selection += QUrl::fromLocalFile(QString::fromUtf8((const char *)it->data));
    g_slist_free(filenames);
    return selection;
}

void QGtk4FileDialogHelper::setFilter()
{
    applyOptions();
}

void QGtk4FileDialogHelper::selectNameFilter(const QString &filter)
{
    GtkFileFilter *gtkFilter = _filters.value(filter);
    if (gtkFilter) {
        GtkDialog *gtkDialog = d->gtkDialog();
        gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(gtkDialog), gtkFilter);
    }
}

QString QGtk4FileDialogHelper::selectedNameFilter() const
{
    GtkDialog *gtkDialog = d->gtkDialog();
    GtkFileFilter *gtkFilter = gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(gtkDialog));
    return _filterNames.value(gtkFilter);
}

void QGtk4FileDialogHelper::onAccepted()
{
    emit accept();

    QString filter = selectedNameFilter();
    if (filter.isEmpty())
        emit filterSelected(filter);

    // These signals are emitted by QFileDialog::accept(), don't emit them twice
    /*
    QList<QUrl> files = selectedFiles();
    emit filesSelected(files);
    if (files.count() == 1)
        emit fileSelected(files.first());
    */
}

void QGtk4FileDialogHelper::onSelectionChanged(GtkDialog *gtkDialog, QGtk4FileDialogHelper *helper)
{
    QString selection;
    gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(gtkDialog));
    if (filename) {
        selection = QString::fromUtf8(filename);
        g_free(filename);
    }
    emit helper->currentChanged(QUrl::fromLocalFile(selection));
}

void QGtk4FileDialogHelper::onCurrentFolderChanged(QGtk4FileDialogHelper *dialog)
{
    emit dialog->directoryEntered(dialog->directory());
}

void QGtk4FileDialogHelper::onUpdatePreview(GtkDialog *gtkDialog, QGtk4FileDialogHelper *helper)
{
    gchar *filename = gtk_file_chooser_get_preview_filename(GTK_FILE_CHOOSER(gtkDialog));
    if (!filename) {
        gtk_file_chooser_set_preview_widget_active(GTK_FILE_CHOOSER(gtkDialog), false);
        return;
    }

    // Don't attempt to open anything which isn't a regular file. If a named pipe,
    // this may hang. See https://crbug.com/534754.
    QFileInfo fileinfo(filename);
    if (!fileinfo.exists() || !fileinfo.isFile()) {
        g_free(filename);
        gtk_file_chooser_set_preview_widget_active(GTK_FILE_CHOOSER(gtkDialog), false);
        return;
    }

    // This will preserve the image's aspect ratio.
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_size(filename, PREVIEW_WIDTH, PREVIEW_HEIGHT, 0);
    g_free(filename);
    if (pixbuf) {
        gtk_image_set_from_pixbuf(helper->previewImage(), pixbuf);
        g_object_unref(pixbuf);
    }
    gtk_file_chooser_set_preview_widget_active(GTK_FILE_CHOOSER(gtkDialog), pixbuf ? true : false);
}

static GtkFileChooserAction gtkFileChooserAction(const QSharedPointer<QFileDialogOptions> &options)
{
    switch (options->fileMode()) {
    case QFileDialogOptions::AnyFile:
    case QFileDialogOptions::ExistingFile:
    case QFileDialogOptions::ExistingFiles:
        if (options->acceptMode() == QFileDialogOptions::AcceptOpen)
            return GTK_FILE_CHOOSER_ACTION_OPEN;
        else
            return GTK_FILE_CHOOSER_ACTION_SAVE;
    case QFileDialogOptions::Directory:
    case QFileDialogOptions::DirectoryOnly:
    default:
        if (options->acceptMode() == QFileDialogOptions::AcceptOpen)
            return GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
        else
            return GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER;
    }
}

void QGtk4FileDialogHelper::applyOptions()
{
    GtkDialog *gtkDialog = d->gtkDialog();
    const QSharedPointer<QFileDialogOptions> &opts = options();

    gtk_window_set_title(GTK_WINDOW(gtkDialog), opts->windowTitle().toUtf8());
    gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(gtkDialog), true);

    const GtkFileChooserAction action = gtkFileChooserAction(opts);
    gtk_file_chooser_set_action(GTK_FILE_CHOOSER(gtkDialog), action);

    const bool selectMultiple = opts->fileMode() == QFileDialogOptions::ExistingFiles;
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(gtkDialog), selectMultiple);

    const bool confirmOverwrite = !opts->testOption(QFileDialogOptions::DontConfirmOverwrite);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(gtkDialog), confirmOverwrite);

    const bool readOnly = opts->testOption(QFileDialogOptions::ReadOnly);
    gtk_file_chooser_set_create_folders(GTK_FILE_CHOOSER(gtkDialog), !readOnly);

    const QStringList nameFilters = opts->nameFilters();
    if (!nameFilters.isEmpty())
        setNameFilters(nameFilters);

    if (opts->initialDirectory().isLocalFile())
        setDirectory(opts->initialDirectory());

    foreach (const QUrl &filename, opts->initiallySelectedFiles())
        selectFile(filename);

    const QString initialNameFilter = opts->initiallySelectedNameFilter();
    if (!initialNameFilter.isEmpty())
        selectNameFilter(initialNameFilter);

    GtkWidget *acceptButton = gtk_dialog_get_widget_for_response(gtkDialog, GTK_RESPONSE_OK);
    if (acceptButton) {
        if (opts->isLabelExplicitlySet(QFileDialogOptions::Accept))
            gtk_button_set_label(GTK_BUTTON(acceptButton), opts->labelText(QFileDialogOptions::Accept).toUtf8());
        else if (opts->acceptMode() == QFileDialogOptions::AcceptOpen)
            gtk_button_set_label(GTK_BUTTON(acceptButton), GTK_STOCK_OPEN);
        else
            gtk_button_set_label(GTK_BUTTON(acceptButton), GTK_STOCK_SAVE);
    }

    GtkWidget *rejectButton = gtk_dialog_get_widget_for_response(gtkDialog, GTK_RESPONSE_CANCEL);
    if (rejectButton) {
        if (opts->isLabelExplicitlySet(QFileDialogOptions::Reject))
            gtk_button_set_label(GTK_BUTTON(rejectButton), opts->labelText(QFileDialogOptions::Reject).toUtf8());
        else
            gtk_button_set_label(GTK_BUTTON(rejectButton), GTK_STOCK_CANCEL);
    }
}

void QGtk4FileDialogHelper::setNameFilters(const QStringList &filters)
{
    GtkDialog *gtkDialog = d->gtkDialog();
    foreach (GtkFileFilter *filter, _filters)
        gtk_file_chooser_remove_filter(GTK_FILE_CHOOSER(gtkDialog), filter);

    _filters.clear();
    _filterNames.clear();

    foreach (const QString &filter, filters) {
        GtkFileFilter *gtkFilter = gtk_file_filter_new();
        const QString name = filter.left(filter.indexOf(QLatin1Char('(')));
        const QStringList extensions = cleanFilterList(filter);

        gtk_file_filter_set_name(gtkFilter, name.isEmpty() ? extensions.join(QStringLiteral(", ")).toUtf8() : name.toUtf8());
        foreach (const QString &ext, extensions)
            gtk_file_filter_add_pattern(gtkFilter, ext.toUtf8());

        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(gtkDialog), gtkFilter);

        _filters.insert(filter, gtkFilter);
        _filterNames.insert(gtkFilter, filter);
    }
}

QGtk4FontDialogHelper::QGtk4FontDialogHelper()
{
    d.reset(new QGtk4Dialog(gtk_font_chooser_dialog_new("", 0)));
    connect(d.data(), SIGNAL(accept()), this, SLOT(onAccepted()));
    connect(d.data(), SIGNAL(reject()), this, SIGNAL(reject()));
}

QGtk4FontDialogHelper::~QGtk4FontDialogHelper()
{
}

bool QGtk4FontDialogHelper::show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent)
{
    applyOptions();
    return d->show(flags, modality, parent);
}

void QGtk4FontDialogHelper::exec()
{
    d->exec();
}

void QGtk4FontDialogHelper::hide()
{
    d->hide();
}

static QString qt_fontToString(const QFont &font)
{
    PangoFontDescription *desc = pango_font_description_new();
    pango_font_description_set_size(desc, (font.pointSizeF() > 0.0 ? font.pointSizeF() : QFontInfo(font).pointSizeF()) * PANGO_SCALE);
    pango_font_description_set_family(desc, QFontInfo(font).family().toUtf8());

    int weight = font.weight();
    if (weight >= QFont::Black)
        pango_font_description_set_weight(desc, PANGO_WEIGHT_HEAVY);
    else if (weight >= QFont::ExtraBold)
        pango_font_description_set_weight(desc, PANGO_WEIGHT_ULTRABOLD);
    else if (weight >= QFont::Bold)
        pango_font_description_set_weight(desc, PANGO_WEIGHT_BOLD);
    else if (weight >= QFont::DemiBold)
        pango_font_description_set_weight(desc, PANGO_WEIGHT_SEMIBOLD);
    else if (weight >= QFont::Medium)
        pango_font_description_set_weight(desc, PANGO_WEIGHT_MEDIUM);
    else if (weight >= QFont::Normal)
        pango_font_description_set_weight(desc, PANGO_WEIGHT_NORMAL);
    else if (weight >= QFont::Light)
        pango_font_description_set_weight(desc, PANGO_WEIGHT_LIGHT);
    else if (weight >= QFont::ExtraLight)
        pango_font_description_set_weight(desc, PANGO_WEIGHT_ULTRALIGHT);
    else
        pango_font_description_set_weight(desc, PANGO_WEIGHT_THIN);

    int style = font.style();
    if (style == QFont::StyleItalic)
        pango_font_description_set_style(desc, PANGO_STYLE_ITALIC);
    else if (style == QFont::StyleOblique)
        pango_font_description_set_style(desc, PANGO_STYLE_OBLIQUE);
    else
        pango_font_description_set_style(desc, PANGO_STYLE_NORMAL);

    char *str = pango_font_description_to_string(desc);
    QString name = QString::fromUtf8(str);
    pango_font_description_free(desc);
    g_free(str);
    return name;
}

static QFont qt_fontFromString(const QString &name)
{
    QFont font;
    PangoFontDescription *desc = pango_font_description_from_string(name.toUtf8());
    font.setPointSizeF(static_cast<float>(pango_font_description_get_size(desc)) / PANGO_SCALE);

    QString family = QString::fromUtf8(pango_font_description_get_family(desc));
    if (!family.isEmpty())
        font.setFamily(family);

    const int weight = pango_font_description_get_weight(desc);
    if (weight >= PANGO_WEIGHT_HEAVY)
        font.setWeight(QFont::Black);
    else if (weight >= PANGO_WEIGHT_ULTRABOLD)
        font.setWeight(QFont::ExtraBold);
    else if (weight >= PANGO_WEIGHT_BOLD)
        font.setWeight(QFont::Bold);
    else if (weight >= PANGO_WEIGHT_SEMIBOLD)
        font.setWeight(QFont::DemiBold);
    else if (weight >= PANGO_WEIGHT_MEDIUM)
        font.setWeight(QFont::Medium);
    else if (weight >= PANGO_WEIGHT_NORMAL)
        font.setWeight(QFont::Normal);
    else if (weight >= PANGO_WEIGHT_LIGHT)
        font.setWeight(QFont::Light);
    else if (weight >= PANGO_WEIGHT_ULTRALIGHT)
        font.setWeight(QFont::ExtraLight);
    else
        font.setWeight(QFont::Thin);

    PangoStyle style = pango_font_description_get_style(desc);
    if (style == PANGO_STYLE_ITALIC)
        font.setStyle(QFont::StyleItalic);
    else if (style == PANGO_STYLE_OBLIQUE)
        font.setStyle(QFont::StyleOblique);
    else
        font.setStyle(QFont::StyleNormal);

    pango_font_description_free(desc);
    return font;
}

void QGtk4FontDialogHelper::setCurrentFont(const QFont &font)
{
    GtkFontChooser *gtkDialog = GTK_FONT_CHOOSER(d->gtkDialog());
    gtk_font_chooser_set_font(gtkDialog, qt_fontToString(font).toUtf8());
}

QFont QGtk4FontDialogHelper::currentFont() const
{
    GtkFontChooser *gtkDialog = GTK_FONT_CHOOSER(d->gtkDialog());
    gchar *name = gtk_font_chooser_get_font(gtkDialog);
    QFont font = qt_fontFromString(QString::fromUtf8(name));
    g_free(name);
    return font;
}

void QGtk4FontDialogHelper::onAccepted()
{
    const QFont font = currentFont();
    emit currentFontChanged(font);
    emit accept();
}

void QGtk4FontDialogHelper::applyOptions()
{
    GtkDialog *gtkDialog = d->gtkDialog();
    const QSharedPointer<QFontDialogOptions> &opts = options();

    gtk_window_set_title(GTK_WINDOW(gtkDialog), opts->windowTitle().toUtf8());
}

QT_END_NAMESPACE

#include "qgtk4dialoghelpers.moc"
