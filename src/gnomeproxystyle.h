#ifndef GNOMEPROXYSTYLE_H
#define GNOMEPROXYSTYLE_H

#include <QProxyStyle>

class GnomeProxyStyle : public QProxyStyle
{
    Q_OBJECT
public:
    explicit GnomeProxyStyle(const QString &key);
    virtual ~GnomeProxyStyle();

    int styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget, QStyleHintReturn *returnData) const;
};

#endif // GNOMEPROXYSTYLE_H
