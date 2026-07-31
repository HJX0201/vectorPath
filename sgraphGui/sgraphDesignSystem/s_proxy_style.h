#pragma once

#include <QProxyStyle>

namespace smartCam
{

class SProxyStyle final : public QProxyStyle
{
  public:
    SProxyStyle();

    int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr,
                    const QWidget* widget = nullptr) const override;
    QSize sizeFromContents(ContentsType type, const QStyleOption* option, const QSize& size,
                           const QWidget* widget) const override;
};

} // namespace smartCam
