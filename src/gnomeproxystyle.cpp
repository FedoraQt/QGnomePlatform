#include "gnomeproxystyle.h"

GnomeProxyStyle::GnomeProxyStyle(const QString &key) :
    QProxyStyle(key)
{
}

GnomeProxyStyle::~GnomeProxyStyle()
{
}

int GnomeProxyStyle::styleHint(QStyle::StyleHint hint, const QStyleOption *option,
                               const QWidget *widget, QStyleHintReturn *returnData) const
{
    if (hint == QStyle::SH_DialogButtonBox_ButtonsHaveIcons) {
        return 0;
    }

    return QProxyStyle::styleHint(hint, option, widget, returnData);
}
