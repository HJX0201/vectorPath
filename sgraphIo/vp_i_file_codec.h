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
    virtual ~VpIFileCodec() = default;
    virtual QString id() const = 0;
    virtual QString displayName() const = 0;
    virtual QStringList extensions() const = 0;
    virtual VpResult<VpFileCompatibilityReport> read(const QString& file_path,
                                                     VpCadDocument& document) const = 0;
    virtual VpResult<VpFileCompatibilityReport> write(const QString& file_path,
                                                      const VpCadDocument& document) const = 0;
};

} // namespace Vp
