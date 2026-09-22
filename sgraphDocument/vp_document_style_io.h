#pragma once

#include "vp_dimension_style_record.h"
#include "vp_result.h"
#include "vp_text_style_record.h"

#include <QString>
#include <vector>

class QDataStream;

namespace Vp
{

struct VpLoadedDocumentStyles
{
    std::vector<VpTextStyleRecord> text_styles{{}};
    QString current_text_style = QStringLiteral("Standard");
    std::vector<VpDimensionStyleRecord> dimension_styles{{}};
    QString current_dimension_style = QStringLiteral("Standard");
};

void writeDocumentStyles(QDataStream& stream, const std::vector<VpTextStyleRecord>& text_styles,
                         const QString& current_text_style,
                         const std::vector<VpDimensionStyleRecord>& dimension_styles,
                         const QString& current_dimension_style);
VpResult<VpLoadedDocumentStyles> readDocumentStyles(QDataStream& stream, quint32 format_version);

} // namespace Vp
