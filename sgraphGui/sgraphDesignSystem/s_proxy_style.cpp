#include "s_proxy_style.h"

#include <QStyleFactory>
#include <algorithm>

namespace vectorPath
{

SProxyStyle::SProxyStyle() : QProxyStyle(QStyleFactory::create(QStringLiteral("Fusion")))
{
}

int SProxyStyle::pixelMetric(PixelMetric metric, const QStyleOption* option,
                             const QWidget* widget) const
{
    switch (metric)
    {
    case PM_SmallIconSize:
        return 24;
    case PM_ButtonIconSize:
    case PM_TabBarIconSize:
        return 22;
    case PM_LargeIconSize:
    case PM_MessageBoxIconSize:
        return 44;
    case PM_ToolBarIconSize:
        return 28;
    case PM_ScrollBarExtent:
        return 12;
    case PM_DefaultFrameWidth:
        return 1;
    default:
        return QProxyStyle::pixelMetric(metric, option, widget);
    }
}

QSize SProxyStyle::sizeFromContents(ContentsType type, const QStyleOption* option,
                                    const QSize& size, const QWidget* widget) const
{
    QSize result = QProxyStyle::sizeFromContents(type, option, size, widget);
    if (type == CT_PushButton)
    {
        result.setHeight(std::max(result.height(), 28));
    }
    else if (type == CT_ComboBox || type == CT_LineEdit)
    {
        result.setHeight(std::max(result.height(), 26));
    }
    return result;
}

} // namespace vectorPath
