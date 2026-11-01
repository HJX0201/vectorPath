#pragma once

#include <QProxyStyle>

namespace Vp
{

// 在系统样式之上微调度量与尺寸，统一界面留白。
class VpProxyStyle final : public QProxyStyle
{
  public:
    VpProxyStyle();

    int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr,
                    const QWidget* widget = nullptr) const override;
    QSize sizeFromContents(ContentsType type, const QStyleOption* option, const QSize& size,
                           const QWidget* widget) const override;
};

} // namespace Vp
