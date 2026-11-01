#pragma once

#include "vp_i_file_codec.h"

namespace Vp
{

// DXF 读写编解码器。
class VpDxfCodec final : public VpIFileCodec
{
  public:
    QString id() const override;
    QString displayName() const override;
    QStringList extensions() const override;
    VpResult<VpFileCompatibilityReport> read(const QString& file_path,
                                             VpCadDocument& document) const override;
    VpResult<VpFileCompatibilityReport> write(const QString& file_path,
                                              const VpCadDocument& document) const override;
};

} // namespace Vp
