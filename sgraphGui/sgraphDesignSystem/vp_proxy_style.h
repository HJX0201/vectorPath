#pragma once

#include <QProxyStyle>

namespace Vp
{

// 在系统样式之上微调度量与尺寸，统一界面留白。
class VpProxyStyle final : public QProxyStyle
{
  public:
    // 构造样式代理。
    VpProxyStyle();

    // 重写度量值，统一控件尺寸。
    int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr,
                    const QWidget* widget = nullptr) const override;
    // 重写内容尺寸，统一控件留白。
    QSize sizeFromContents(ContentsType type, const QStyleOption* option, const QSize& size,
                           const QWidget* widget) const override;
};

} // namespace Vp
