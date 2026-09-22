#include "vp_document_style_io.h"

#include "vp_qt_text.h"

#include <QDataStream>
#include <algorithm>
#include <cmath>

namespace Vp
{
namespace
{

template <typename SStyle>
bool hasStyle(const std::vector<SStyle>& styles, const QString& style_name)
{
    return std::any_of(styles.begin(), styles.end(),
                       [&style_name](const SStyle& style)
                       {
                           return style.name.compare(style_name, Qt::CaseInsensitive) == 0;
                       });
}

bool validTextStyle(const VpTextStyleRecord& style)
{
    return !style.name.trimmed().isEmpty() && !style.font_family.trimmed().isEmpty() &&
           std::isfinite(style.fixed_height) && style.fixed_height >= 0.0 &&
           std::isfinite(style.width_factor) && style.width_factor >= 0.01 &&
           style.width_factor <= 100.0 && std::isfinite(style.oblique_angle) &&
           std::abs(style.oblique_angle) < 85.0;
}

bool validDimensionStyle(const VpDimensionStyleRecord& style)
{
    return !style.name.trimmed().isEmpty() && std::isfinite(style.text_height) &&
           style.text_height > 0.0 && std::isfinite(style.arrow_size) && style.arrow_size > 0.0 &&
           std::isfinite(style.overall_scale) && style.overall_scale > 0.0 &&
           std::isfinite(style.linear_scale) && style.linear_scale > 0.0 &&
           style.linear_precision >= 0 && style.linear_precision <= 8 &&
           style.angular_precision >= 0 && style.angular_precision <= 8;
}

} // namespace

void writeDocumentStyles(QDataStream& stream, const std::vector<VpTextStyleRecord>& text_styles,
                         const QString& current_text_style,
                         const std::vector<VpDimensionStyleRecord>& dimension_styles,
                         const QString& current_dimension_style)
{
    stream << static_cast<quint32>(text_styles.size());
    for (const VpTextStyleRecord& style : text_styles)
    {
        stream << style.name << style.font_family << style.fixed_height << style.width_factor
               << style.oblique_angle << style.is_bold << style.is_italic;
    }
    stream << current_text_style << static_cast<quint32>(dimension_styles.size());
    for (const VpDimensionStyleRecord& style : dimension_styles)
    {
        stream << style.name << style.text_height << style.arrow_size << style.overall_scale
               << style.linear_scale << static_cast<qint32>(style.linear_precision)
               << static_cast<qint32>(style.angular_precision) << style.prefix << style.suffix
               << style.suppress_trailing_zeros;
    }
    stream << current_dimension_style;
}

VpResult<VpLoadedDocumentStyles> readDocumentStyles(QDataStream& stream, quint32 format_version)
{
    VpLoadedDocumentStyles loaded;
    if (format_version < 17)
    {
        return VpResult<VpLoadedDocumentStyles>::success(std::move(loaded));
    }
    quint32 text_style_count = 0;
    stream >> text_style_count;
    if (text_style_count == 0 || text_style_count > 10'000U)
    {
        return VpResult<VpLoadedDocumentStyles>::failure(
            toCoreText(QStringLiteral("文件文字样式表无效。")));
    }
    loaded.text_styles.clear();
    loaded.text_styles.reserve(text_style_count);
    for (quint32 index = 0; index < text_style_count; ++index)
    {
        VpTextStyleRecord style;
        stream >> style.name >> style.font_family >> style.fixed_height >> style.width_factor >>
            style.oblique_angle >> style.is_bold >> style.is_italic;
        if (!validTextStyle(style) || hasStyle(loaded.text_styles, style.name))
        {
            return VpResult<VpLoadedDocumentStyles>::failure(
                toCoreText(QStringLiteral("文件文字样式数据无效。")));
        }
        loaded.text_styles.push_back(std::move(style));
    }
    stream >> loaded.current_text_style;
    if (!hasStyle(loaded.text_styles, loaded.current_text_style))
    {
        return VpResult<VpLoadedDocumentStyles>::failure(
            toCoreText(QStringLiteral("文件当前文字样式无效。")));
    }
    if (format_version < 19)
    {
        return VpResult<VpLoadedDocumentStyles>::success(std::move(loaded));
    }

    quint32 dimension_style_count = 0;
    stream >> dimension_style_count;
    if (dimension_style_count == 0 || dimension_style_count > 10'000U)
    {
        return VpResult<VpLoadedDocumentStyles>::failure(
            toCoreText(QStringLiteral("文件标注样式表无效。")));
    }
    loaded.dimension_styles.clear();
    loaded.dimension_styles.reserve(dimension_style_count);
    for (quint32 index = 0; index < dimension_style_count; ++index)
    {
        VpDimensionStyleRecord style;
        qint32 linear_precision = 0;
        qint32 angular_precision = 0;
        stream >> style.name >> style.text_height >> style.arrow_size >> style.overall_scale >>
            style.linear_scale >> linear_precision >> angular_precision >> style.prefix >>
            style.suffix >> style.suppress_trailing_zeros;
        style.linear_precision = linear_precision;
        style.angular_precision = angular_precision;
        if (!validDimensionStyle(style) || hasStyle(loaded.dimension_styles, style.name))
        {
            return VpResult<VpLoadedDocumentStyles>::failure(
                toCoreText(QStringLiteral("文件标注样式数据无效。")));
        }
        loaded.dimension_styles.push_back(std::move(style));
    }
    stream >> loaded.current_dimension_style;
    if (!hasStyle(loaded.dimension_styles, loaded.current_dimension_style) ||
        stream.status() != QDataStream::Ok)
    {
        return VpResult<VpLoadedDocumentStyles>::failure(
            toCoreText(QStringLiteral("文件当前标注样式无效。")));
    }
    return VpResult<VpLoadedDocumentStyles>::success(std::move(loaded));
}

} // namespace Vp
