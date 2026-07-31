#pragma once

#include <QTranslator>

namespace vectorPath
{

class SChineseUiTranslator final : public QTranslator
{
  public:
    explicit SChineseUiTranslator(QObject* parent = nullptr);

    QString translate(const char* context, const char* source_text,
                      const char* disambiguation = nullptr, int count = -1) const override;
};

} // namespace vectorPath
