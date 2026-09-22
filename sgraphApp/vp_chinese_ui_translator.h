#pragma once

#include <QTranslator>

namespace Vp
{

class VpChineseUiTranslator final : public QTranslator
{
  public:
    explicit VpChineseUiTranslator(QObject* parent = nullptr);

    QString translate(const char* context, const char* source_text,
                      const char* disambiguation = nullptr, int count = -1) const override;
};

} // namespace Vp
