#include "vp_dimension_binary_io.h"

#include "vp_dimension_geometry.h"
#include "vp_qt_text.h"

#include <QDataStream>

namespace Vp
{
namespace
{

void writePoint(QDataStream& stream, const VpPoint2d& point)
{
    stream << point.x << point.y;
}

void readPoint(QDataStream& stream, VpPoint2d& point)
{
    stream >> point.x >> point.y;
}

} // namespace

void writeDimensionData(QDataStream& stream, const VpLinearDimensionEntity& dimension)
{
    writePoint(stream, dimension.first_point);
    writePoint(stream, dimension.second_point);
    writePoint(stream, dimension.dimension_line_point);
    writePoint(stream, dimension.center_point);
    stream << static_cast<quint8>(dimension.dimension_type) << dimension.style_name
           << dimension.text_override;
}

VpResult<VpLinearDimensionEntity> readDimensionData(QDataStream& stream, quint32 format_version)
{
    VpLinearDimensionEntity dimension;
    readPoint(stream, dimension.first_point);
    readPoint(stream, dimension.second_point);
    readPoint(stream, dimension.dimension_line_point);
    if (format_version >= 18)
    {
        quint8 dimension_type = 0;
        readPoint(stream, dimension.center_point);
        stream >> dimension_type >> dimension.style_name >> dimension.text_override;
        if (dimension_type > static_cast<quint8>(VpDimensionType::Ordinate))
        {
            return VpResult<VpLinearDimensionEntity>::failure(
                toCoreText(QStringLiteral("文件标注类型无效。")));
        }
        dimension.dimension_type = static_cast<VpDimensionType>(dimension_type);
        if (dimension.style_name.trimmed().isEmpty())
        {
            dimension.style_name = QStringLiteral("Standard");
        }
    }
    else
    {
        dimension.style_name = QStringLiteral("Standard");
    }
    if (stream.status() != QDataStream::Ok || !isDimensionValid(dimension))
    {
        return VpResult<VpLinearDimensionEntity>::failure(
            toCoreText(QStringLiteral("文件标注数据无效。")));
    }
    return VpResult<VpLinearDimensionEntity>::success(std::move(dimension));
}

} // namespace Vp
