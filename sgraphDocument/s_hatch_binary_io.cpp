#include "s_hatch_binary_io.h"

#include "s_hatch_geometry.h"

#include <QDataStream>

namespace smartGraphics
{
namespace
{

constexpr quint32 kMaximumPointCount = 1'000'000U;

void writePoint(QDataStream& stream, const SPoint2d& point)
{
    stream << point.x << point.y;
}

void writeLoop(QDataStream& stream, const std::vector<SPoint2d>& loop)
{
    stream << static_cast<quint32>(loop.size());
    for (const SPoint2d& point : loop)
    {
        writePoint(stream, point);
    }
}

bool readLoop(QDataStream& stream, std::vector<SPoint2d>& loop)
{
    quint32 point_count = 0;
    stream >> point_count;
    if (point_count > kMaximumPointCount)
    {
        return false;
    }
    loop.resize(point_count);
    for (SPoint2d& point : loop)
    {
        stream >> point.x >> point.y;
    }
    return stream.status() == QDataStream::Ok;
}

} // namespace

void writeHatchData(QDataStream& stream, const SHatchEntity& hatch)
{
    writeLoop(stream, hatch.boundary);
    stream << static_cast<quint32>(hatch.island_boundaries.size());
    for (const std::vector<SPoint2d>& island : hatch.island_boundaries)
    {
        writeLoop(stream, island);
    }
    stream << static_cast<quint8>(hatch.fill_type) << hatch.pattern_name << hatch.pattern_scale
           << hatch.pattern_angle << static_cast<quint32>(hatch.gradient_start.rgba())
           << static_cast<quint32>(hatch.gradient_end.rgba())
           << static_cast<quint64>(hatch.associative_boundary_id);
}

SResult<SHatchEntity> readHatchData(QDataStream& stream, quint32 format_version)
{
    SHatchEntity hatch;
    if (!readLoop(stream, hatch.boundary))
    {
        return SResult<SHatchEntity>::failure(QStringLiteral("填充边界数据无效。"));
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
            return SResult<SHatchEntity>::failure(QStringLiteral("填充岛数量超过安全限制。"));
        }
        hatch.island_boundaries.resize(island_count);
        for (std::vector<SPoint2d>& island : hatch.island_boundaries)
        {
            if (!readLoop(stream, island))
            {
                return SResult<SHatchEntity>::failure(QStringLiteral("填充岛数据无效。"));
            }
        }
        stream >> fill_type >> hatch.pattern_name >> hatch.pattern_scale >> hatch.pattern_angle >>
            gradient_start >> gradient_end >> boundary_id;
        if (fill_type > static_cast<quint8>(SHatchFillType::Gradient))
        {
            return SResult<SHatchEntity>::failure(QStringLiteral("填充类型无效。"));
        }
        hatch.fill_type = static_cast<SHatchFillType>(fill_type);
        hatch.gradient_start = QColor::fromRgba(gradient_start);
        hatch.gradient_end = QColor::fromRgba(gradient_end);
        hatch.associative_boundary_id = boundary_id;
    }
    if (!isHatchValid(hatch))
    {
        return SResult<SHatchEntity>::failure(QStringLiteral("文件填充数据无效。"));
    }
    return SResult<SHatchEntity>::success(std::move(hatch));
}

} // namespace smartGraphics
