#include "s_entity_binary_io.h"

#include "s_associative_array_io.h"
#include "s_dimension_binary_io.h"
#include "s_hatch_binary_io.h"

#include <QDataStream>
#include <cmath>

namespace smartCam
{
namespace
{

void writePoint(QDataStream& stream, const SPoint2d& point)
{
    stream << point.x << point.y;
}

void readPoint(QDataStream& stream, SPoint2d& point)
{
    stream >> point.x >> point.y;
}

SResult<SPolylineEntity> readPolyline(QDataStream& stream, quint32 format_version)
{
    quint32 vertex_count = 0;
    bool is_closed = false;
    stream >> vertex_count >> is_closed;
    if (vertex_count > 1'000'000U)
    {
        return SResult<SPolylineEntity>::failure(QStringLiteral("多段线顶点数量超过安全限制。"));
    }
    SPolylineEntity polyline;
    polyline.vertices.resize(vertex_count);
    polyline.is_closed = is_closed;
    for (SPoint2d& vertex : polyline.vertices)
    {
        readPoint(stream, vertex);
    }
    if (format_version >= 8)
    {
        quint32 bulge_count = 0;
        stream >> bulge_count;
        if (bulge_count != 0U && bulge_count != vertex_count)
        {
            return SResult<SPolylineEntity>::failure(
                QStringLiteral("多段线 bulge 数量与顶点数量不一致。"));
        }
        polyline.bulges.resize(bulge_count);
        for (double& bulge : polyline.bulges)
        {
            stream >> bulge;
            if (!std::isfinite(bulge))
            {
                return SResult<SPolylineEntity>::failure(QStringLiteral("多段线包含无效 bulge。"));
            }
        }
    }
    if (format_version >= 14)
    {
        quint32 start_width_count = 0;
        stream >> start_width_count;
        if ((start_width_count != 0U && start_width_count != vertex_count) ||
            start_width_count > 1'000'000U)
        {
            return SResult<SPolylineEntity>::failure(
                QStringLiteral("多段线起始宽度数量与顶点数量不一致。"));
        }
        polyline.start_widths.resize(start_width_count);
        for (double& width : polyline.start_widths)
        {
            stream >> width;
            if (!std::isfinite(width) || width < 0.0 || width > 1.0e9)
            {
                return SResult<SPolylineEntity>::failure(
                    QStringLiteral("多段线包含无效起始宽度。"));
            }
        }
        quint32 end_width_count = 0;
        stream >> end_width_count;
        if ((end_width_count != 0U && end_width_count != vertex_count) ||
            end_width_count > 1'000'000U)
        {
            return SResult<SPolylineEntity>::failure(
                QStringLiteral("多段线终止宽度数量与顶点数量不一致。"));
        }
        polyline.end_widths.resize(end_width_count);
        for (double& width : polyline.end_widths)
        {
            stream >> width;
            if (!std::isfinite(width) || width < 0.0 || width > 1.0e9)
            {
                return SResult<SPolylineEntity>::failure(
                    QStringLiteral("多段线包含无效终止宽度。"));
            }
        }
    }
    return SResult<SPolylineEntity>::success(std::move(polyline));
}

SResult<SEntityGeometry> readGeometry(QDataStream& stream, SEntityType entity_type,
                                      quint32 format_version)
{
    if (entity_type == SEntityType::Line)
    {
        SLineEntity line;
        readPoint(stream, line.start_point);
        readPoint(stream, line.end_point);
        return SResult<SEntityGeometry>::success(line);
    }
    if (entity_type == SEntityType::Circle)
    {
        SCircleEntity circle;
        readPoint(stream, circle.center);
        stream >> circle.radius;
        return SResult<SEntityGeometry>::success(circle);
    }
    if (entity_type == SEntityType::Arc && format_version >= 2)
    {
        SArcEntity arc;
        readPoint(stream, arc.center);
        stream >> arc.radius >> arc.start_angle >> arc.end_angle;
        if (format_version >= 24)
        {
            stream >> arc.is_clockwise;
        }
        return SResult<SEntityGeometry>::success(arc);
    }
    if (entity_type == SEntityType::Polyline && format_version >= 2)
    {
        SResult<SPolylineEntity> polyline = readPolyline(stream, format_version);
        if (!polyline)
        {
            return SResult<SEntityGeometry>::failure(polyline.errorMessage());
        }
        return SResult<SEntityGeometry>::success(std::move(polyline.value()));
    }
    if (entity_type == SEntityType::Text && format_version >= 3)
    {
        STextEntity text;
        readPoint(stream, text.position);
        stream >> text.text >> text.height >> text.rotation;
        if (format_version >= 17)
        {
            quint8 horizontal_alignment = 0;
            quint8 vertical_alignment = 0;
            stream >> text.style_name >> horizontal_alignment >> vertical_alignment;
            if (horizontal_alignment > static_cast<quint8>(STextHorizontalAlignment::Justified) ||
                vertical_alignment > static_cast<quint8>(STextVerticalAlignment::Bottom))
            {
                return SResult<SEntityGeometry>::failure(QStringLiteral("文件文字对齐方式无效。"));
            }
            text.horizontal_alignment = static_cast<STextHorizontalAlignment>(horizontal_alignment);
            text.vertical_alignment = static_cast<STextVerticalAlignment>(vertical_alignment);
        }
        return SResult<SEntityGeometry>::success(std::move(text));
    }
    if (entity_type == SEntityType::LinearDimension && format_version >= 3)
    {
        SResult<SLinearDimensionEntity> dimension = readDimensionData(stream, format_version);
        if (!dimension)
        {
            return SResult<SEntityGeometry>::failure(dimension.errorMessage());
        }
        return SResult<SEntityGeometry>::success(std::move(dimension.value()));
    }
    if (entity_type == SEntityType::Hatch && format_version >= 3)
    {
        SResult<SHatchEntity> hatch = readHatchData(stream, format_version);
        if (!hatch)
        {
            return SResult<SEntityGeometry>::failure(hatch.errorMessage());
        }
        return SResult<SEntityGeometry>::success(std::move(hatch.value()));
    }
    if (entity_type == SEntityType::Spline && format_version >= 13)
    {
        SSplineEntity spline;
        for (SPoint2d& control_point : spline.control_points)
        {
            readPoint(stream, control_point);
        }
        return SResult<SEntityGeometry>::success(spline);
    }
    if (entity_type == SEntityType::Ellipse && format_version >= 15)
    {
        SEllipseEntity ellipse;
        readPoint(stream, ellipse.center);
        readPoint(stream, ellipse.major_axis);
        readPoint(stream, ellipse.minor_axis);
        if (std::hypot(ellipse.major_axis.x, ellipse.major_axis.y) <= 1.0e-9 ||
            std::hypot(ellipse.minor_axis.x, ellipse.minor_axis.y) <= 1.0e-9)
        {
            return SResult<SEntityGeometry>::failure(QStringLiteral("文件包含无效椭圆。"));
        }
        return SResult<SEntityGeometry>::success(ellipse);
    }
    if (entity_type == SEntityType::MText && format_version >= 17)
    {
        SMTextEntity text;
        quint8 horizontal_alignment = 0;
        readPoint(stream, text.position);
        stream >> text.rich_text >> text.width >> text.height >> text.rotation >> text.style_name >>
            horizontal_alignment;
        if (text.rich_text.isEmpty() || !std::isfinite(text.width) || text.width <= 0.0 ||
            !std::isfinite(text.height) || text.height <= 0.0 || !std::isfinite(text.rotation) ||
            horizontal_alignment > static_cast<quint8>(STextHorizontalAlignment::Justified))
        {
            return SResult<SEntityGeometry>::failure(QStringLiteral("文件多行文字数据无效。"));
        }
        text.horizontal_alignment = static_cast<STextHorizontalAlignment>(horizontal_alignment);
        return SResult<SEntityGeometry>::success(std::move(text));
    }
    if (entity_type == SEntityType::Leader && format_version >= 17)
    {
        quint32 vertex_count = 0;
        stream >> vertex_count;
        if (vertex_count < 2 || vertex_count > 1'000'000U)
        {
            return SResult<SEntityGeometry>::failure(QStringLiteral("文件引线顶点数量无效。"));
        }
        SLeaderEntity leader;
        leader.vertices.resize(vertex_count);
        for (SPoint2d& vertex : leader.vertices)
        {
            readPoint(stream, vertex);
        }
        stream >> leader.text >> leader.text_height >> leader.arrow_size >> leader.style_name;
        if (leader.text.isEmpty() || !std::isfinite(leader.text_height) ||
            leader.text_height <= 0.0 || !std::isfinite(leader.arrow_size) ||
            leader.arrow_size <= 0.0)
        {
            return SResult<SEntityGeometry>::failure(QStringLiteral("文件引线数据无效。"));
        }
        return SResult<SEntityGeometry>::success(std::move(leader));
    }
    return SResult<SEntityGeometry>::failure(QStringLiteral("文件包含未知实体类型。"));
}

} // namespace

void writeEntityRecord(QDataStream& stream, const SEntityRecord& entity)
{
    stream << static_cast<quint64>(entity.id) << static_cast<quint8>(entity.type)
           << entity.layer_name << entity.line_width_mm << entity.space_name;
    if (entity.type == SEntityType::Line)
    {
        const auto& line = std::get<SLineEntity>(entity.geometry);
        writePoint(stream, line.start_point);
        writePoint(stream, line.end_point);
    }
    else if (entity.type == SEntityType::Circle)
    {
        const auto& circle = std::get<SCircleEntity>(entity.geometry);
        writePoint(stream, circle.center);
        stream << circle.radius;
    }
    else if (entity.type == SEntityType::Arc)
    {
        const auto& arc = std::get<SArcEntity>(entity.geometry);
        writePoint(stream, arc.center);
        stream << arc.radius << arc.start_angle << arc.end_angle << arc.is_clockwise;
    }
    else if (entity.type == SEntityType::Polyline)
    {
        const auto& polyline = std::get<SPolylineEntity>(entity.geometry);
        stream << static_cast<quint32>(polyline.vertices.size()) << polyline.is_closed;
        for (const SPoint2d& vertex : polyline.vertices)
        {
            writePoint(stream, vertex);
        }
        stream << static_cast<quint32>(polyline.bulges.size());
        for (double bulge : polyline.bulges)
        {
            stream << bulge;
        }
        stream << static_cast<quint32>(polyline.start_widths.size());
        for (double width : polyline.start_widths)
        {
            stream << width;
        }
        stream << static_cast<quint32>(polyline.end_widths.size());
        for (double width : polyline.end_widths)
        {
            stream << width;
        }
    }
    else if (entity.type == SEntityType::Text)
    {
        const auto& text = std::get<STextEntity>(entity.geometry);
        writePoint(stream, text.position);
        stream << text.text << text.height << text.rotation << text.style_name
               << static_cast<quint8>(text.horizontal_alignment)
               << static_cast<quint8>(text.vertical_alignment);
    }
    else if (entity.type == SEntityType::LinearDimension)
    {
        writeDimensionData(stream, std::get<SLinearDimensionEntity>(entity.geometry));
    }
    else if (entity.type == SEntityType::Hatch)
    {
        writeHatchData(stream, std::get<SHatchEntity>(entity.geometry));
    }
    else if (entity.type == SEntityType::Spline)
    {
        for (const SPoint2d& point : std::get<SSplineEntity>(entity.geometry).control_points)
        {
            writePoint(stream, point);
        }
    }
    else if (entity.type == SEntityType::Ellipse)
    {
        const auto& ellipse = std::get<SEllipseEntity>(entity.geometry);
        writePoint(stream, ellipse.center);
        writePoint(stream, ellipse.major_axis);
        writePoint(stream, ellipse.minor_axis);
    }
    else if (entity.type == SEntityType::MText)
    {
        const auto& text = std::get<SMTextEntity>(entity.geometry);
        writePoint(stream, text.position);
        stream << text.rich_text << text.width << text.height << text.rotation << text.style_name
               << static_cast<quint8>(text.horizontal_alignment);
    }
    else if (entity.type == SEntityType::Leader)
    {
        const auto& leader = std::get<SLeaderEntity>(entity.geometry);
        stream << static_cast<quint32>(leader.vertices.size());
        for (const SPoint2d& vertex : leader.vertices)
        {
            writePoint(stream, vertex);
        }
        stream << leader.text << leader.text_height << leader.arrow_size << leader.style_name;
    }
    writeAssociativeArrayData(stream, entity.associative_array);
}

SResult<SEntityRecord> readEntityRecord(QDataStream& stream, quint32 format_version)
{
    quint64 id = 0;
    quint8 type_value = 0;
    stream >> id >> type_value;
    if (format_version <= 6)
    {
        quint32 legacy_entity_rgba = 0;
        stream >> legacy_entity_rgba;
    }
    SEntityRecord entity;
    entity.id = id;
    entity.type = static_cast<SEntityType>(type_value);
    if (format_version >= 4)
    {
        stream >> entity.layer_name;
    }
    if (format_version >= 6)
    {
        stream >> entity.line_width_mm;
        if (!std::isfinite(entity.line_width_mm) ||
            (entity.line_width_mm < 0.0 && entity.line_width_mm != -1.0) ||
            entity.line_width_mm > 2.11)
        {
            return SResult<SEntityRecord>::failure(QStringLiteral("文件实体线宽无效。"));
        }
    }
    if (format_version >= 22)
    {
        stream >> entity.space_name;
    }
    SResult<SEntityGeometry> geometry = readGeometry(stream, entity.type, format_version);
    if (!geometry)
    {
        return SResult<SEntityRecord>::failure(geometry.errorMessage());
    }
    entity.geometry = std::move(geometry.value());
    if (format_version >= 16 &&
        !readAssociativeArrayData(stream, entity.associative_array, format_version))
    {
        return SResult<SEntityRecord>::failure(QStringLiteral("文件包含无效关联阵列数据。"));
    }
    if (stream.status() != QDataStream::Ok)
    {
        return SResult<SEntityRecord>::failure(QStringLiteral("文件实体数据不完整。"));
    }
    return SResult<SEntityRecord>::success(std::move(entity));
}

} // namespace smartCam
