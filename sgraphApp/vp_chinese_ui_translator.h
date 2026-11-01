#pragma once

#include <QTranslator>

namespace Vp
{

// 为 Qt 标准界面文案提供简体中文翻译。
class VpChineseUiTranslator final : public QTranslator
{
  public:
    explicit VpChineseUiTranslator(QObject* parent = nullptr);

    QString translate(const char* context, const char* source_text,
                      const char* disambiguation = nullptr, int count = -1) const override;
};

} // namespace Vp
