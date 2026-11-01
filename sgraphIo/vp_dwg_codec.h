#pragma once

#include "vp_i_file_codec.h"

namespace Vp
{

// DWG 读写编解码器，通过 libredwg 外部进程转换。
class VpDwgCodec final : public VpIFileCodec
{
  public:
    // 返回编解码器标识。
    QString id() const override;
    // 返回可读的格式名称。
    QString displayName() const override;
    // 返回支持的文件扩展名。
    QStringList extensions() const override;
    // 读取 DWG 文件并导入文档。
    VpResult<VpFileCompatibilityReport> read(const QString& file_path,
                                             VpCadDocument& document) const override;
    // 把文档导出为 DWG 文件。
    VpResult<VpFileCompatibilityReport> write(const QString& file_path,
                                              const VpCadDocument& document) const override;

    // 判断 libredwg 工具是否可用。
    bool isAvailable() const;
    // 返回 libredwg 的版本信息。
    QString toolVersion() const;
};

} // namespace Vp
