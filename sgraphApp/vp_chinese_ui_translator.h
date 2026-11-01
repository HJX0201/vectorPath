#pragma once

#include <QTranslator>

namespace Vp
{

// 为 Qt 标准界面文案提供简体中文翻译。
class VpChineseUiTranslator final : public QTranslator
{
  public:
    // 构造中文翻译器。
    explicit VpChineseUiTranslator(QObject* parent = nullptr);

    // 重写翻译入口，按上下文返回中文文案。
    QString translate(const char* context, const char* source_text,
                      const char* disambiguation = nullptr, int count = -1) const override;
};

} // namespace Vp
