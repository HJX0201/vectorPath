#pragma once

#include "vp_file_compatibility_report.h"
#include "vp_result.h"

#include <QString>
#include <QStringList>

namespace Vp
{

class VpCadDocument;

// 文件编解码器接口，DWG/DXF 等格式实现同一契约。
class VpIFileCodec
{
  public:
    // 虚析构，保证派生类正确释放。
    virtual ~VpIFileCodec() = default;
    // 返回编解码器标识。
    virtual QString id() const = 0;
    // 返回可读的格式名称。
    virtual QString displayName() const = 0;
    // 返回支持的文件扩展名。
    virtual QStringList extensions() const = 0;
    // 读取文件并导入文档。
    virtual VpResult<VpFileCompatibilityReport> read(const QString& file_path,
                                                     VpCadDocument& document) const = 0;
    // 把文档导出为文件。
    virtual VpResult<VpFileCompatibilityReport> write(const QString& file_path,
                                                      const VpCadDocument& document) const = 0;
};

} // namespace Vp
