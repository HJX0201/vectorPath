#include "vp_entity_binary_io.h"

#include "vp_associative_array_io.h"
#include "vp_dimension_binary_io.h"
#include "vp_hatch_binary_io.h"
#include "vp_qt_text.h"

#include <QDataStream>
#include <cmath>

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

VpResult<VpPolylineEntity> readPolyline(QDataStream& stream, quint32 format_version)
{
    quint32 vertex_count = 0;
    bool is_closed = false;
    stream >> vertex_count >> is_closed;
    if (vertex_count > 1'000'000U)
    {
        return VpResult<VpPolylineEntity>::failure(
            toCoreText(QStringLiteral("多段线顶点数量超过安全限制。")));
    }
    VpPolylineEntity polyline;
    polyline.vertices.resize(vertex_count);
    polyline.is_closed = is_closed;
    for (VpPoint2d& vertex : polyline.vertices)
    {
        readPoint(stream, vertex);
    }
    if (format_version >= 8)
    {
        quint32 bulge_count = 0;
        stream >> bulge_count;
        if (bulge_count != 0U && bulge_count != vertex_count)
        {
            return VpResult<VpPolylineEntity>::failure(
                toCoreText(QStringLiteral("多段线 bulge 数量与顶点数量不一致。")));
        }
        polyline.bulges.resize(bulge_count);
        for (double& bulge : polyline.bulges)
        {
            stream >> bulge;
            if (!std::isfinite(bulge))
            {
                return VpResult<VpPolylineEntity>::failure(
                    toCoreText(QStringLiteral("多段线包含无效 bulge。")));
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
            return VpResult<VpPolylineEntity>::failure(
                toCoreText(QStringLiteral("多段线起始宽度数量与顶点数量不一致。")));
        }
        polyline.start_widths.resize(start_width_count);
        for (double& width : polyline.start_widths)
        {
            stream >> width;
            if (!std::isfinite(width) || width < 0.0 || width > 1.0e9)
            {
                return VpResult<VpPolylineEntity>::failure(
                    toCoreText(QStringLiteral("多段线包含无效起始宽度。")));
            }
        }
        quint32 end_width_count = 0;
        stream >> end_width_count;
        if ((end_width_count != 0U && end_width_count != vertex_count) ||
            end_width_count > 1'000'000U)
        {
            return VpResult<VpPolylineEntity>::failure(
                toCoreText(QStringLiteral("多段线终止宽度数量与顶点数量不一致。")));
        }
        polyline.end_widths.resize(end_width_count);
        for (double& width : polyline.end_widths)
        {
            stream >> width;
            if (!std::isfinite(width) || width < 0.0 || width > 1.0e9)
            {
                return VpResult<VpPolylineEntity>::failure(
                    toCoreText(QStringLiteral("多段线包含无效终止宽度。")));
            }
        }
    }
    return VpResult<VpPolylineEntity>::success(std::move(polyline));
}

VpResult<VpEntityGeometry> readGeometry(QDataStream& stream, VpEntityType entity_type,
                                        quint32 format_version)
{
    if (entity_type == VpEntityType::Line)
    {
        VpLineEntity line;
        readPoint(stream, line.start_point);
        readPoint(stream, line.end_point);
        return VpResult<VpEntityGeometry>::success(line);
    }
    if (entity_type == VpEntityType::Circle)
    {
        VpCircleEntity circle;
        readPoint(stream, circle.center);
        stream >> circle.radius;
        return VpResult<VpEntityGeometry>::success(circle);
    }
    if (entity_type == VpEntityType::Arc && format_version >= 2)
    {
        VpArcEntity arc;
        readPoint(stream, arc.center);
        stream >> arc.radius >> arc.start_angle >> arc.end_angle;
        if (format_version >= 24)
        {
            stream >> arc.is_clockwise;
        }
        return VpResult<VpEntityGeometry>::success(arc);
    }
    if (entity_type == VpEntityType::Polyline && format_version >= 2)
    {
        VpResult<VpPolylineEntity> polyline = readPolyline(stream, format_version);
        if (!polyline)
        {
            return VpResult<VpEntityGeometry>::failure(toCoreText(toQtError(polyline)));
        }
        return VpResult<VpEntityGeometry>::success(std::move(polyline.value()));
    }
    if (entity_type == VpEntityType::Text && format_version >= 3)
    {
        VpTextEntity text;
        readPoint(stream, text.position);
        stream >> text.text >> text.height >> text.rotation;
        if (format_version >= 17)
        {
            quint8 horizontal_alignment = 0;
            quint8 vertical_alignment = 0;
            stream >> text.style_name >> horizontal_alignment >> vertical_alignment;
            if (horizontal_alignment > static_cast<quint8>(VpTextHorizontalAlignment::Justified) ||
                vertical_alignment > static_cast<quint8>(VpTextVerticalAlignment::Bottom))
            {
                return VpResult<VpEntityGeometry>::failure(
                    toCoreText(QStringLiteral("文件文字对齐方式无效。")));
            }
            text.horizontal_alignment =
                static_cast<VpTextHorizontalAlignment>(horizontal_alignment);
            text.vertical_alignment = static_cast<VpTextVerticalAlignment>(vertical_alignment);
        }
        return VpResult<VpEntityGeometry>::success(std::move(text));
    }
    if (entity_type == VpEntityType::LinearDimension && format_version >= 3)
    {
        VpResult<VpLinearDimensionEntity> dimension = readDimensionData(stream, format_version);
        if (!dimension)
        {
            return VpResult<VpEntityGeometry>::failure(toCoreText(toQtError(dimension)));
        }
        return VpResult<VpEntityGeometry>::success(std::move(dimension.value()));
    }
    if (entity_type == VpEntityType::Hatch && format_version >= 3)
    {
        VpResult<VpHatchEntity> hatch = readHatchData(stream, format_version);
        if (!hatch)
        {
            return VpResult<VpEntityGeometry>::failure(toCoreText(toQtError(hatch)));
        }
        return VpResult<VpEntityGeometry>::success(std::move(hatch.value()));
    }
    if (entity_type == VpEntityType::Spline && format_version >= 13)
    {
        VpSplineEntity spline;
        for (VpPoint2d& control_point : spline.control_points)
        {
            readPoint(stream, control_point);
        }
        return VpResult<VpEntityGeometry>::success(spline);
    }
    if (entity_type == VpEntityType::Ellipse && format_version >= 15)
    {
        VpEllipseEntity ellipse;
        readPoint(stream, ellipse.center);
        readPoint(stream, ellipse.major_axis);
        readPoint(stream, ellipse.minor_axis);
        if (std::hypot(ellipse.major_axis.x, ellipse.major_axis.y) <= 1.0e-9 ||
            std::hypot(ellipse.minor_axis.x, ellipse.minor_axis.y) <= 1.0e-9)
        {
            return VpResult<VpEntityGeometry>::failure(
                toCoreText(QStringLiteral("文件包含无效椭圆。")));
        }
        return VpResult<VpEntityGeometry>::success(ellipse);
    }
    if (entity_type == VpEntityType::MText && format_version >= 17)
    {
        VpMTextEntity text;
        quint8 horizontal_alignment = 0;
        readPoint(stream, text.position);
        stream >> text.rich_text >> text.width >> text.height >> text.rotation >> text.style_name >>
            horizontal_alignment;
        if (text.rich_text.isEmpty() || !std::isfinite(text.width) || text.width <= 0.0 ||
            !std::isfinite(text.height) || text.height <= 0.0 || !std::isfinite(text.rotation) ||
            horizontal_alignment > static_cast<quint8>(VpTextHorizontalAlignment::Justified))
        {
            return VpResult<VpEntityGeometry>::failure(
                toCoreText(QStringLiteral("文件多行文字数据无效。")));
        }
        text.horizontal_alignment = static_cast<VpTextHorizontalAlignment>(horizontal_alignment);
        return VpResult<VpEntityGeometry>::success(std::move(text));
    }
    if (entity_type == VpEntityType::Leader && format_version >= 17)
    {
        quint32 vertex_count = 0;
        stream >> vertex_count;
        if (vertex_count < 2 || vertex_count > 1'000'000U)
        {
            return VpResult<VpEntityGeometry>::failure(
                toCoreText(QStringLiteral("文件引线顶点数量无效。")));
        }
        VpLeaderEntity leader;
        leader.vertices.resize(vertex_count);
        for (VpPoint2d& vertex : leader.vertices)
        {
            readPoint(stream, vertex);
        }
        stream >> leader.text >> leader.text_height >> leader.arrow_size >> leader.style_name;
        if (leader.text.isEmpty() || !std::isfinite(leader.text_height) ||
            leader.text_height <= 0.0 || !std::isfinite(leader.arrow_size) ||
            leader.arrow_size <= 0.0)
        {
            return VpResult<VpEntityGeometry>::failure(
                toCoreText(QStringLiteral("文件引线数据无效。")));
        }
        return VpResult<VpEntityGeometry>::success(std::move(leader));
    }
    return VpResult<VpEntityGeometry>::failure(
        toCoreText(QStringLiteral("文件包含未知实体类型。")));
}

} // namespace

void writeEntityRecord(QDataStream& stream, const VpEntityRecord& entity)
{
    stream << static_cast<quint64>(entity.id) << static_cast<quint8>(entity.type)
           << entity.layer_name << entity.line_width_mm << entity.space_name;
    if (entity.type == VpEntityType::Line)
    {
        const auto& line = std::get<VpLineEntity>(entity.geometry);
        writePoint(stream, line.start_point);
        writePoint(stream, line.end_point);
    }
    else if (entity.type == VpEntityType::Circle)
    {
        const auto& circle = std::get<VpCircleEntity>(entity.geometry);
        writePoint(stream, circle.center);
        stream << circle.radius;
    }
    else if (entity.type == VpEntityType::Arc)
    {
        const auto& arc = std::get<VpArcEntity>(entity.geometry);
        writePoint(stream, arc.center);
        stream << arc.radius << arc.start_angle << arc.end_angle << arc.is_clockwise;
    }
    else if (entity.type == VpEntityType::Polyline)
    {
        const auto& polyline = std::get<VpPolylineEntity>(entity.geometry);
        stream << static_cast<quint32>(polyline.vertices.size()) << polyline.is_closed;
        for (const VpPoint2d& vertex : polyline.vertices)
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
    else if (entity.type == VpEntityType::Text)
    {
        const auto& text = std::get<VpTextEntity>(entity.geometry);
        writePoint(stream, text.position);
        stream << text.text << text.height << text.rotation << text.style_name
               << static_cast<quint8>(text.horizontal_alignment)
               << static_cast<quint8>(text.vertical_alignment);
    }
    else if (entity.type == VpEntityType::LinearDimension)
    {
        writeDimensionData(stream, std::get<VpLinearDimensionEntity>(entity.geometry));
    }
    else if (entity.type == VpEntityType::Hatch)
    {
        writeHatchData(stream, std::get<VpHatchEntity>(entity.geometry));
    }
    else if (entity.type == VpEntityType::Spline)
    {
        for (const VpPoint2d& point : std::get<VpSplineEntity>(entity.geometry).control_points)
        {
            writePoint(stream, point);
        }
    }
    else if (entity.type == VpEntityType::Ellipse)
    {
        const auto& ellipse = std::get<VpEllipseEntity>(entity.geometry);
        writePoint(stream, ellipse.center);
        writePoint(stream, ellipse.major_axis);
        writePoint(stream, ellipse.minor_axis);
    }
    else if (entity.type == VpEntityType::MText)
    {
        const auto& text = std::get<VpMTextEntity>(entity.geometry);
        writePoint(stream, text.position);
        stream << text.rich_text << text.width << text.height << text.rotation << text.style_name
               << static_cast<quint8>(text.horizontal_alignment);
    }
    else if (entity.type == VpEntityType::Leader)
    {
        const auto& leader = std::get<VpLeaderEntity>(entity.geometry);
        stream << static_cast<quint32>(leader.vertices.size());
        for (const VpPoint2d& vertex : leader.vertices)
        {
            writePoint(stream, vertex);
        }
        stream << leader.text << leader.text_height << leader.arrow_size << leader.style_name;
    }
    writeAssociativeArrayData(stream, entity.associative_array);
}

VpResult<VpEntityRecord> readEntityRecord(QDataStream& stream, quint32 format_version)
{
    quint64 id = 0;
    quint8 type_value = 0;
    stream >> id >> type_value;
    if (format_version <= 6)
    {
        quint32 legacy_entity_rgba = 0;
        stream >> legacy_entity_rgba;
    }
    VpEntityRecord entity;
    entity.id = id;
    entity.type = static_cast<VpEntityType>(type_value);
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
            return VpResult<VpEntityRecord>::failure(
                toCoreText(QStringLiteral("文件实体线宽无效。")));
        }
    }
    if (format_version >= 22)
    {
        stream >> entity.space_name;
    }
    VpResult<VpEntityGeometry> geometry = readGeometry(stream, entity.type, format_version);
    if (!geometry)
    {
        return VpResult<VpEntityRecord>::failure(toCoreText(toQtError(geometry)));
    }
    entity.geometry = std::move(geometry.value());
    if (format_version >= 16 &&
        !readAssociativeArrayData(stream, entity.associative_array, format_version))
    {
        return VpResult<VpEntityRecord>::failure(
            toCoreText(QStringLiteral("文件包含无效关联阵列数据。")));
    }
    if (stream.status() != QDataStream::Ok)
    {
        return VpResult<VpEntityRecord>::failure(
            toCoreText(QStringLiteral("文件实体数据不完整。")));
    }
    return VpResult<VpEntityRecord>::success(std::move(entity));
}

} // namespace Vp
