#pragma once

#include "s_file_compatibility_report.h"
#include "s_result.h"

#include <QString>
#include <QStringList>

namespace vectorPath
{

class SCadDocument;

class SIFileCodec
{
  public:
    virtual ~SIFileCodec() = default;
    virtual QString id() const = 0;
    virtual QString displayName() const = 0;
    virtual QStringList extensions() const = 0;
    virtual SResult<SFileCompatibilityReport> read(const QString& file_path,
                                                   SCadDocument& document) const = 0;
    virtual SResult<SFileCompatibilityReport> write(const QString& file_path,
                                                    const SCadDocument& document) const = 0;
};

} // namespace vectorPath
