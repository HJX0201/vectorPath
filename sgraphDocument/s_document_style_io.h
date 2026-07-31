#pragma once

#include "s_dimension_style_record.h"
#include "s_result.h"
#include "s_text_style_record.h"

#include <QString>
#include <vector>

class QDataStream;

namespace smartCam
{

struct SLoadedDocumentStyles
{
    std::vector<STextStyleRecord> text_styles{{}};
    QString current_text_style = QStringLiteral("Standard");
    std::vector<SDimensionStyleRecord> dimension_styles{{}};
    QString current_dimension_style = QStringLiteral("Standard");
};

void writeDocumentStyles(QDataStream& stream, const std::vector<STextStyleRecord>& text_styles,
                         const QString& current_text_style,
                         const std::vector<SDimensionStyleRecord>& dimension_styles,
                         const QString& current_dimension_style);
SResult<SLoadedDocumentStyles> readDocumentStyles(QDataStream& stream, quint32 format_version);

} // namespace smartCam
