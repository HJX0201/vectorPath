#pragma once

#include "s_i_file_codec.h"

namespace smartCam
{

class SDwgCodec final : public SIFileCodec
{
  public:
    QString id() const override;
    QString displayName() const override;
    QStringList extensions() const override;
    SResult<SFileCompatibilityReport> read(const QString& file_path,
                                           SCadDocument& document) const override;
    SResult<SFileCompatibilityReport> write(const QString& file_path,
                                            const SCadDocument& document) const override;

    bool isAvailable() const;
    QString toolVersion() const;
};

} // namespace smartCam
