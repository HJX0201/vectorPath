#include "vp_associative_array_io.h"

#include <cmath>

namespace Vp
{
namespace
{

constexpr quint32 kMaximumPointCount = 1'000'000U;

void writePoint(QDataStream& stream, const VpPoint2d& point)
{
    stream << point.x << point.y;
}

void readPoint(QDataStream& stream, VpPoint2d& point)
{
    stream >> point.x >> point.y;
}

void writeGeometry(QDataStream& stream, VpEntityType type, const VpEntityGeometry& geometry)
{
    if (type == VpEntityType::Line)
    {
        const auto& line = std::get<VpLineEntity>(geometry);
        writePoint(stream, line.start_point);
        writePoint(stream, line.end_point);
    }
    else if (type == VpEntityType::Circle)
    {
        const auto& circle = std::get<VpCircleEntity>(geometry);
        writePoint(stream, circle.center);
        stream << circle.radius;
    }
    else if (type == VpEntityType::Arc)
    {
        const auto& arc = std::get<VpArcEntity>(geometry);
        writePoint(stream, arc.center);
        stream << arc.radius << arc.start_angle << arc.end_angle;
    }
    else if (type == VpEntityType::Polyline)
    {
        const auto& polyline = std::get<VpPolylineEntity>(geometry);
        stream << static_cast<quint32>(polyline.vertices.size()) << polyline.is_closed;
        for (const VpPoint2d& point : polyline.vertices)
        {
            writePoint(stream, point);
        }
        stream << static_cast<quint32>(polyline.bulges.size());
        for (double value : polyline.bulges)
        {
            stream << value;
        }
        stream << static_cast<quint32>(polyline.start_widths.size());
        for (double value : polyline.start_widths)
        {
            stream << value;
        }
        stream << static_cast<quint32>(polyline.end_widths.size());
        for (double value : polyline.end_widths)
        {
            stream << value;
        }
    }
    else if (type == VpEntityType::Text)
    {
        const auto& text = std::get<VpTextEntity>(geometry);
        writePoint(stream, text.position);
        stream << text.text << text.height << text.rotation;
    }
    else if (type == VpEntityType::LinearDimension)
    {
        const auto& dimension = std::get<VpLinearDimensionEntity>(geometry);
        writePoint(stream, dimension.first_point);
        writePoint(stream, dimension.second_point);
        writePoint(stream, dimension.dimension_line_point);
        writePoint(stream, dimension.center_point);
        stream << static_cast<quint8>(dimension.dimension_type) << dimension.style_name
               << dimension.text_override;
    }
    else if (type == VpEntityType::Hatch)
    {
        const auto& hatch = std::get<VpHatchEntity>(geometry);
        stream << static_cast<quint32>(hatch.boundary.size());
        for (const VpPoint2d& point : hatch.boundary)
        {
            writePoint(stream, point);
        }
        stream << static_cast<quint32>(hatch.island_boundaries.size());
        for (const std::vector<VpPoint2d>& island : hatch.island_boundaries)
        {
            stream << static_cast<quint32>(island.size());
            for (const VpPoint2d& point : island)
            {
                writePoint(stream, point);
            }
        }
        stream << static_cast<quint8>(hatch.fill_type) << hatch.pattern_name << hatch.pattern_scale
               << hatch.pattern_angle << static_cast<quint32>(hatch.gradient_start.rgba())
               << static_cast<quint32>(hatch.gradient_end.rgba())
               << static_cast<quint64>(hatch.associative_boundary_id);
    }
    else if (type == VpEntityType::Spline)
    {
        for (const VpPoint2d& point : std::get<VpSplineEntity>(geometry).control_points)
        {
            writePoint(stream, point);
        }
    }
    else if (type == VpEntityType::Ellipse)
    {
        const auto& ellipse = std::get<VpEllipseEntity>(geometry);
        writePoint(stream, ellipse.center);
        writePoint(stream, ellipse.major_axis);
        writePoint(stream, ellipse.minor_axis);
    }
    else if (type == VpEntityType::MText)
    {
        const auto& text = std::get<VpMTextEntity>(geometry);
        writePoint(stream, text.position);
        stream << text.rich_text << text.width << text.height << text.rotation << text.style_name
               << static_cast<quint8>(text.horizontal_alignment);
    }
    else if (type == VpEntityType::Leader)
    {
        const auto& leader = std::get<VpLeaderEntity>(geometry);
        stream << static_cast<quint32>(leader.vertices.size());
        for (const VpPoint2d& point : leader.vertices)
        {
            writePoint(stream, point);
        }
        stream << leader.text << leader.text_height << leader.arrow_size << leader.style_name;
    }
}

bool readCount(QDataStream& stream, quint32& count)
{
    stream >> count;
    return count <= kMaximumPointCount;
}

bool readGeometry(QDataStream& stream, VpEntityType type, VpEntityGeometry& geometry,
                  quint32 format_version)
{
    if (type == VpEntityType::Line)
    {
        VpLineEntity line;
        readPoint(stream, line.start_point);
        readPoint(stream, line.end_point);
        geometry = line;
    }
    else if (type == VpEntityType::Circle)
    {
        VpCircleEntity circle;
        readPoint(stream, circle.center);
        stream >> circle.radius;
        geometry = circle;
    }
    else if (type == VpEntityType::Arc)
    {
        VpArcEntity arc;
        readPoint(stream, arc.center);
        stream >> arc.radius >> arc.start_angle >> arc.end_angle;
        geometry = arc;
    }
    else if (type == VpEntityType::Polyline)
    {
        VpPolylineEntity polyline;
        quint32 vertex_count = 0;
        quint32 bulge_count = 0;
        quint32 start_width_count = 0;
        quint32 end_width_count = 0;
        if (!readCount(stream, vertex_count))
        {
            return false;
        }
        stream >> polyline.is_closed;
        polyline.vertices.resize(vertex_count);
        for (VpPoint2d& point : polyline.vertices)
        {
            readPoint(stream, point);
        }
        if (!readCount(stream, bulge_count))
        {
            return false;
        }
        polyline.bulges.resize(bulge_count);
        for (double& value : polyline.bulges)
        {
            stream >> value;
        }
        if (!readCount(stream, start_width_count))
        {
            return false;
        }
        polyline.start_widths.resize(start_width_count);
        for (double& value : polyline.start_widths)
        {
            stream >> value;
        }
        if (!readCount(stream, end_width_count))
        {
            return false;
        }
        polyline.end_widths.resize(end_width_count);
        for (double& value : polyline.end_widths)
        {
            stream >> value;
        }
        if ((bulge_count != 0U && bulge_count != vertex_count) ||
            (start_width_count != 0U && start_width_count != vertex_count) ||
            (end_width_count != 0U && end_width_count != vertex_count))
        {
            return false;
        }
        geometry = std::move(polyline);
    }
    else if (type == VpEntityType::Text)
    {
        VpTextEntity text;
        readPoint(stream, text.position);
        stream >> text.text >> text.height >> text.rotation;
        geometry = std::move(text);
    }
    else if (type == VpEntityType::LinearDimension)
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
                return false;
            }
            dimension.dimension_type = static_cast<VpDimensionType>(dimension_type);
        }
        else
        {
            dimension.style_name = QStringLiteral("Standard");
        }
        geometry = dimension;
    }
    else if (type == VpEntityType::Hatch)
    {
        VpHatchEntity hatch;
        quint32 point_count = 0;
        if (!readCount(stream, point_count))
        {
            return false;
        }
        hatch.boundary.resize(point_count);
        for (VpPoint2d& point : hatch.boundary)
        {
            readPoint(stream, point);
        }
        if (format_version >= 20)
        {
            quint32 island_count = 0;
            quint8 fill_type = 0;
            quint32 gradient_start = 0;
            quint32 gradient_end = 0;
            quint64 boundary_id = 0;
            if (!readCount(stream, island_count))
            {
                return false;
            }
            hatch.island_boundaries.resize(island_count);
            for (std::vector<VpPoint2d>& island : hatch.island_boundaries)
            {
                quint32 island_point_count = 0;
                if (!readCount(stream, island_point_count))
                {
                    return false;
                }
                island.resize(island_point_count);
                for (VpPoint2d& point : island)
                {
                    readPoint(stream, point);
                }
            }
            stream >> fill_type >> hatch.pattern_name >> hatch.pattern_scale >>
                hatch.pattern_angle >> gradient_start >> gradient_end >> boundary_id;
            if (fill_type > static_cast<quint8>(VpHatchFillType::Gradient))
            {
                return false;
            }
            hatch.fill_type = static_cast<VpHatchFillType>(fill_type);
            hatch.gradient_start = QColor::fromRgba(gradient_start);
            hatch.gradient_end = QColor::fromRgba(gradient_end);
            hatch.associative_boundary_id = boundary_id;
        }
        geometry = std::move(hatch);
    }
    else if (type == VpEntityType::Spline)
    {
        VpSplineEntity spline;
        for (VpPoint2d& point : spline.control_points)
        {
            readPoint(stream, point);
        }
        geometry = spline;
    }
    else if (type == VpEntityType::Ellipse)
    {
        VpEllipseEntity ellipse;
        readPoint(stream, ellipse.center);
        readPoint(stream, ellipse.major_axis);
        readPoint(stream, ellipse.minor_axis);
        geometry = ellipse;
    }
    else if (type == VpEntityType::MText)
    {
        VpMTextEntity text;
        quint8 alignment = 0;
        readPoint(stream, text.position);
        stream >> text.rich_text >> text.width >> text.height >> text.rotation >> text.style_name >>
            alignment;
        if (alignment > static_cast<quint8>(VpTextHorizontalAlignment::Justified))
        {
            return false;
        }
        text.horizontal_alignment = static_cast<VpTextHorizontalAlignment>(alignment);
        geometry = std::move(text);
    }
    else if (type == VpEntityType::Leader)
    {
        VpLeaderEntity leader;
        quint32 point_count = 0;
        if (!readCount(stream, point_count) || point_count < 2)
        {
            return false;
        }
        leader.vertices.resize(point_count);
        for (VpPoint2d& point : leader.vertices)
        {
            readPoint(stream, point);
        }
        stream >> leader.text >> leader.text_height >> leader.arrow_size >> leader.style_name;
        geometry = std::move(leader);
    }
    else
    {
        return false;
    }
    return stream.status() == QDataStream::Ok;
}

} // namespace

void writeAssociativeArrayData(QDataStream& stream,
                               const std::optional<VpAssociativeArrayData>& array_data)
{
    stream << array_data.has_value();
    if (!array_data)
    {
        return;
    }
    stream << static_cast<quint64>(array_data->array_id)
           << static_cast<quint8>(array_data->array_type)
           << static_cast<quint32>(array_data->source_index)
           << static_cast<quint32>(array_data->item_index)
           << static_cast<qint32>(array_data->column_count)
           << static_cast<qint32>(array_data->row_count)
           << static_cast<qint32>(array_data->item_count) << array_data->column_spacing
           << array_data->row_spacing << array_data->fill_angle;
    writePoint(stream, array_data->center);
    writePoint(stream, array_data->source_base_point);
    stream << static_cast<quint64>(array_data->path_entity_id) << array_data->align_to_path
           << static_cast<quint8>(array_data->source_type);
    writeGeometry(stream, array_data->source_type, array_data->source_geometry);
}

bool readAssociativeArrayData(QDataStream& stream,
                              std::optional<VpAssociativeArrayData>& array_data,
                              quint32 format_version)
{
    bool has_array_data = false;
    stream >> has_array_data;
    if (!has_array_data)
    {
        array_data.reset();
        return stream.status() == QDataStream::Ok;
    }
    quint64 array_id = 0;
    quint8 array_type = 0;
    quint32 source_index = 0;
    quint32 item_index = 0;
    qint32 column_count = 0;
    qint32 row_count = 0;
    qint32 item_count = 0;
    quint64 path_entity_id = 0;
    quint8 source_type = 0;
    VpAssociativeArrayData value;
    stream >> array_id >> array_type >> source_index >> item_index >> column_count >> row_count >>
        item_count >> value.column_spacing >> value.row_spacing >> value.fill_angle;
    readPoint(stream, value.center);
    readPoint(stream, value.source_base_point);
    stream >> path_entity_id >> value.align_to_path >> source_type;
    value.array_id = array_id;
    value.array_type = static_cast<VpArrayType>(array_type);
    value.source_index = source_index;
    value.item_index = item_index;
    value.column_count = column_count;
    value.row_count = row_count;
    value.item_count = item_count;
    value.path_entity_id = path_entity_id;
    value.source_type = static_cast<VpEntityType>(source_type);
    if (value.array_id == 0 || array_type < 1 || array_type > 3 || source_type < 1 ||
        source_type > 11 || value.source_index > 10000U || value.item_index > 10000U ||
        value.column_count < 1 || value.row_count < 1 || value.item_count < 1 ||
        value.item_count > 10000 || !std::isfinite(value.column_spacing) ||
        !std::isfinite(value.row_spacing) || !std::isfinite(value.fill_angle) ||
        !readGeometry(stream, value.source_type, value.source_geometry, format_version))
    {
        return false;
    }
    array_data = std::move(value);
    return true;
}

} // namespace Vp
