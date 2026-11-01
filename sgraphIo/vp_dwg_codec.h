#pragma once

#include "vp_i_file_codec.h"

namespace Vp
{

// DWG 读写编解码器，通过 libredwg 外部进程转换。
class VpDwgCodec final : public VpIFileCodec
{
  public:
    QString id() const override;
    QString displayName() const override;
    QStringList extensions() const override;
    VpResult<VpFileCompatibilityReport> read(const QString& file_path,
                                             VpCadDocument& document) const override;
    VpResult<VpFileCompatibilityReport> write(const QString& file_path,
                                              const VpCadDocument& document) const override;

    bool isAvailable() const;
    QString toolVersion() const;
};

} // namespace Vp
