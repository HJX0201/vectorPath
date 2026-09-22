#include "vp_hatch_binary_io.h"

#include "vp_hatch_geometry.h"
#include "vp_qt_text.h"

#include <QDataStream>

namespace Vp
{
namespace
{

constexpr quint32 kMaximumPointCount = 1'000'000U;

void writePoint(QDataStream& stream, const VpPoint2d& point)
{
    stream << point.x << point.y;
}

void writeLoop(QDataStream& stream, const std::vector<VpPoint2d>& loop)
{
    stream << static_cast<quint32>(loop.size());
    for (const VpPoint2d& point : loop)
    {
        writePoint(stream, point);
    }
}

bool readLoop(QDataStream& stream, std::vector<VpPoint2d>& loop)
{
    quint32 point_count = 0;
    stream >> point_count;
    if (point_count > kMaximumPointCount)
    {
        return false;
    }
    loop.resize(point_count);
    for (VpPoint2d& point : loop)
    {
        stream >> point.x >> point.y;
    }
    return stream.status() == QDataStream::Ok;
}

} // namespace

void writeHatchData(QDataStream& stream, const VpHatchEntity& hatch)
{
    writeLoop(stream, hatch.boundary);
    stream << static_cast<quint32>(hatch.island_boundaries.size());
    for (const std::vector<VpPoint2d>& island : hatch.island_boundaries)
    {
        writeLoop(stream, island);
    }
    stream << static_cast<quint8>(hatch.fill_type) << hatch.pattern_name << hatch.pattern_scale
           << hatch.pattern_angle << static_cast<quint32>(hatch.gradient_start.rgba())
           << static_cast<quint32>(hatch.gradient_end.rgba())
           << static_cast<quint64>(hatch.associative_boundary_id);
}

VpResult<VpHatchEntity> readHatchData(QDataStream& stream, quint32 format_version)
{
    VpHatchEntity hatch;
    if (!readLoop(stream, hatch.boundary))
    {
        return VpResult<VpHatchEntity>::failure(toCoreText(QStringLiteral("填充边界数据无效。")));
    }
    if (format_version >= 20)
    {
        quint32 island_count = 0;
        quint8 fill_type = 0;
        quint32 gradient_start = 0;
        quint32 gradient_end = 0;
        quint64 boundary_id = 0;
        stream >> island_count;
        if (island_count > 100'000U)
        {
            return VpResult<VpHatchEntity>::failure(
                toCoreText(QStringLiteral("填充岛数量超过安全限制。")));
        }
        hatch.island_boundaries.resize(island_count);
        for (std::vector<VpPoint2d>& island : hatch.island_boundaries)
        {
            if (!readLoop(stream, island))
            {
                return VpResult<VpHatchEntity>::failure(
                    toCoreText(QStringLiteral("填充岛数据无效。")));
            }
        }
        stream >> fill_type >> hatch.pattern_name >> hatch.pattern_scale >> hatch.pattern_angle >>
            gradient_start >> gradient_end >> boundary_id;
        if (fill_type > static_cast<quint8>(VpHatchFillType::Gradient))
        {
            return VpResult<VpHatchEntity>::failure(toCoreText(QStringLiteral("填充类型无效。")));
        }
        hatch.fill_type = static_cast<VpHatchFillType>(fill_type);
        hatch.gradient_start = QColor::fromRgba(gradient_start);
        hatch.gradient_end = QColor::fromRgba(gradient_end);
        hatch.associative_boundary_id = boundary_id;
    }
    if (!isHatchValid(hatch))
    {
        return VpResult<VpHatchEntity>::failure(toCoreText(QStringLiteral("文件填充数据无效。")));
    }
    return VpResult<VpHatchEntity>::success(std::move(hatch));
}

} // namespace Vp
