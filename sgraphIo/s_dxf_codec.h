#pragma once

#include "s_i_file_codec.h"

namespace smartGraphics
{

class SDxfCodec final : public SIFileCodec
{
  public:
    QString id() const override;
    QString displayName() const override;
    QStringList extensions() const override;
    SResult<SFileCompatibilityReport> read(const QString& file_path,
                                           SCadDocument& document) const override;
    SResult<SFileCompatibilityReport> write(const QString& file_path,
                                            const SCadDocument& document) const override;
};

} // namespace smartGraphics
