#include "vp_dxf_entity_io.h"

#include "vp_dxf_hatch_io.h"
#include "vp_qt_text.h"

#include <QTextDocument>
#include <QTextStream>
#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <utility>

namespace Vp
{
namespace
{

void writeEntityCommon(QTextStream& stream, const VpEntityRecord& entity)
{
    writeDxfPair(stream, 8, entity.layer_name);
    const int dxf_line_width = entity.line_width_mm < 0.0
                                   ? -1
                                   : static_cast<int>(std::round(entity.line_width_mm * 100.0));
    writeDxfPair(stream, 370, QString::number(dxf_line_width));
    if (!entity.space_name.isEmpty())
    {
        writeDxfPair(stream, 67, QStringLiteral("1"));
        writeDxfPair(stream, 410, entity.space_name);
    }
}

VpEntityRecord commonRecord(const std::vector<VpDxfPair>& pairs)
{
    VpEntityRecord entity;
    entity.layer_name = findDxfString(pairs, 8).value_or(QStringLiteral("0"));
    const double dxf_line_width = findDxfDouble(pairs, 370).value_or(-100.0);
    entity.line_width_mm = dxf_line_width < 0.0 ? -1.0 : dxf_line_width / 100.0;
    entity.space_name = findDxfString(pairs, 410).value_or(QString());
    return entity;
}

std::vector<VpPoint2d> readRepeatedPoints(const std::vector<VpDxfPair>& pairs, int x_code,
                                          int y_code)
{
    std::vector<VpPoint2d> points;
    std::optional<double> pending_x;
    for (const VpDxfPair& pair : pairs)
    {
        if (pair.group_code == x_code)
        {
            bool is_valid = false;
            const double value = pair.value.toDouble(&is_valid);
            pending_x = is_valid ? std::optional<double>(value) : std::nullopt;
        }
        else if (pair.group_code == y_code && pending_x)
        {
            bool is_valid = false;
            const double value = pair.value.toDouble(&is_valid);
            if (is_valid)
            {
                points.push_back({*pending_x, value});
            }
            pending_x.reset();
        }
    }
    return points;
}

VpResult<VpEntityRecord> invalidEntity(const QString& entity_name)
{
    return VpResult<VpEntityRecord>::failure(
        toCoreText(QObject::tr("字段不完整或几何无效的 %1 实体。").arg(entity_name)));
}

void countExport(VpFileCompatibilityReport& report, bool count_entity)
{
    if (count_entity)
    {
        ++report.exported_entity_count;
    }
}

} // namespace

VpResult<VpEntityRecord> readDxfEntity(const QString& entity_name,
                                       const std::vector<VpDxfPair>& entity_pairs)
{
    VpEntityRecord entity = commonRecord(entity_pairs);
    if (entity_name == QLatin1String("LINE"))
    {
        const auto x1 = findDxfDouble(entity_pairs, 10);
        const auto y1 = findDxfDouble(entity_pairs, 20);
        const auto x2 = findDxfDouble(entity_pairs, 11);
        const auto y2 = findDxfDouble(entity_pairs, 21);
        if (!x1 || !y1 || !x2 || !y2)
        {
            return invalidEntity(entity_name);
        }
        entity.type = VpEntityType::Line;
        entity.geometry = VpLineEntity{{*x1, *y1}, {*x2, *y2}};
    }
    else if (entity_name == QLatin1String("CIRCLE"))
    {
        const auto center_x = findDxfDouble(entity_pairs, 10);
        const auto center_y = findDxfDouble(entity_pairs, 20);
        const auto radius = findDxfDouble(entity_pairs, 40);
        if (!center_x || !center_y || !radius || *radius <= 0.0)
        {
            return invalidEntity(entity_name);
        }
        entity.type = VpEntityType::Circle;
        entity.geometry = VpCircleEntity{{*center_x, *center_y}, *radius};
    }
    else if (entity_name == QLatin1String("ARC"))
    {
        const auto center_x = findDxfDouble(entity_pairs, 10);
        const auto center_y = findDxfDouble(entity_pairs, 20);
        const auto radius = findDxfDouble(entity_pairs, 40);
        const auto start_angle = findDxfDouble(entity_pairs, 50);
        const auto end_angle = findDxfDouble(entity_pairs, 51);
        if (!center_x || !center_y || !radius || !start_angle || !end_angle || *radius <= 0.0)
        {
            return invalidEntity(entity_name);
        }
        entity.type = VpEntityType::Arc;
        entity.geometry = VpArcEntity{{*center_x, *center_y}, *radius, *start_angle, *end_angle};
    }
    else if (entity_name == QLatin1String("ELLIPSE"))
    {
        const auto center_x = findDxfDouble(entity_pairs, 10);
        const auto center_y = findDxfDouble(entity_pairs, 20);
        const auto major_x = findDxfDouble(entity_pairs, 11);
        const auto major_y = findDxfDouble(entity_pairs, 21);
        const auto ratio = findDxfDouble(entity_pairs, 40);
        if (!center_x || !center_y || !major_x || !major_y || !ratio || *ratio <= 0.0 ||
            *ratio > 1.0 || std::hypot(*major_x, *major_y) <= 1.0e-9)
        {
            return invalidEntity(entity_name);
        }
        entity.type = VpEntityType::Ellipse;
        entity.geometry = VpEllipseEntity{{*center_x, *center_y},
                                          {*major_x, *major_y},
                                          {-(*major_y) * (*ratio), (*major_x) * (*ratio)}};
    }
    else if (entity_name == QLatin1String("LWPOLYLINE"))
    {
        VpPolylineEntity polyline;
        polyline.is_closed =
            (static_cast<int>(findDxfDouble(entity_pairs, 70).value_or(0.0)) & 1) != 0;
        std::optional<double> pending_x;
        for (const VpDxfPair& pair : entity_pairs)
        {
            bool is_valid = false;
            const double value = pair.value.toDouble(&is_valid);
            if (pair.group_code == 10)
            {
                pending_x = is_valid ? std::optional<double>(value) : std::nullopt;
            }
            else if (pair.group_code == 20 && pending_x)
            {
                if (is_valid)
                {
                    polyline.vertices.push_back({*pending_x, value});
                    polyline.bulges.push_back(0.0);
                    polyline.start_widths.push_back(0.0);
                    polyline.end_widths.push_back(0.0);
                }
                pending_x.reset();
            }
            else if (is_valid && pair.group_code == 42 && !polyline.bulges.empty())
            {
                polyline.bulges.back() = value;
            }
            else if (is_valid && pair.group_code == 40 && value >= 0.0 &&
                     !polyline.start_widths.empty())
            {
                polyline.start_widths.back() = value;
            }
            else if (is_valid && pair.group_code == 41 && value >= 0.0 &&
                     !polyline.end_widths.empty())
            {
                polyline.end_widths.back() = value;
            }
        }
        if (polyline.vertices.size() < 2)
        {
            return invalidEntity(entity_name);
        }
        entity.type = VpEntityType::Polyline;
        entity.geometry = std::move(polyline);
    }
    else if (entity_name == QLatin1String("TEXT"))
    {
        const auto position_x = findDxfDouble(entity_pairs, 10);
        const auto position_y = findDxfDouble(entity_pairs, 20);
        const auto text = findDxfString(entity_pairs, 1);
        const double height = findDxfDouble(entity_pairs, 40).value_or(2.5);
        if (!position_x || !position_y || !text || text->isEmpty() || height <= 0.0)
        {
            return invalidEntity(entity_name);
        }
        VpTextEntity text_entity;
        text_entity.position = {*position_x, *position_y};
        text_entity.text = *text;
        text_entity.height = height;
        text_entity.rotation = findDxfDouble(entity_pairs, 50).value_or(0.0);
        text_entity.style_name =
            findDxfString(entity_pairs, 7).value_or(QStringLiteral("Standard"));
        entity.type = VpEntityType::Text;
        entity.geometry = std::move(text_entity);
    }
    else if (entity_name == QLatin1String("MTEXT"))
    {
        const auto position_x = findDxfDouble(entity_pairs, 10);
        const auto position_y = findDxfDouble(entity_pairs, 20);
        QString contents;
        for (const VpDxfPair& pair : entity_pairs)
        {
            if (pair.group_code == 1 || pair.group_code == 3)
            {
                contents += pair.value;
            }
        }
        const double height = findDxfDouble(entity_pairs, 40).value_or(2.5);
        const double width = findDxfDouble(entity_pairs, 41).value_or(40.0);
        if (!position_x || !position_y || contents.isEmpty() || height <= 0.0 || width <= 0.0)
        {
            return invalidEntity(entity_name);
        }
        VpMTextEntity text;
        text.position = {*position_x, *position_y};
        text.rich_text = contents;
        text.width = width;
        text.height = height;
        text.rotation = findDxfDouble(entity_pairs, 50).value_or(0.0);
        text.style_name = findDxfString(entity_pairs, 7).value_or(QStringLiteral("Standard"));
        entity.type = VpEntityType::MText;
        entity.geometry = std::move(text);
    }
    else if (entity_name == QLatin1String("DIMENSION"))
    {
        const auto dimension_x = findDxfDouble(entity_pairs, 10);
        const auto dimension_y = findDxfDouble(entity_pairs, 20);
        const auto first_x = findDxfDouble(entity_pairs, 13);
        const auto first_y = findDxfDouble(entity_pairs, 23);
        const auto second_x = findDxfDouble(entity_pairs, 14);
        const auto second_y = findDxfDouble(entity_pairs, 24);
        if (!dimension_x || !dimension_y || !first_x || !first_y || !second_x || !second_y)
        {
            return invalidEntity(entity_name);
        }
        VpLinearDimensionEntity dimension;
        dimension.dimension_line_point = {*dimension_x, *dimension_y};
        dimension.first_point = {*first_x, *first_y};
        dimension.second_point = {*second_x, *second_y};
        dimension.center_point = {findDxfDouble(entity_pairs, 15).value_or(0.0),
                                  findDxfDouble(entity_pairs, 25).value_or(0.0)};
        const std::optional<double> smartcad_type = findDxfDouble(entity_pairs, 1070);
        const int raw_dxf_type = static_cast<int>(findDxfDouble(entity_pairs, 70).value_or(0.0));
        int dimension_type = smartcad_type ? static_cast<int>(*smartcad_type) : (raw_dxf_type & 7);
        if (!smartcad_type && dimension_type == 3)
        {
            dimension_type = static_cast<int>(VpDimensionType::Diameter);
        }
        else if (!smartcad_type && dimension_type == 4)
        {
            dimension_type = static_cast<int>(VpDimensionType::Radius);
        }
        else if (!smartcad_type && dimension_type == 6)
        {
            dimension_type = static_cast<int>(VpDimensionType::Ordinate);
        }
        if (dimension_type >= 0 && dimension_type <= static_cast<int>(VpDimensionType::Ordinate))
        {
            dimension.dimension_type = static_cast<VpDimensionType>(dimension_type);
        }
        dimension.style_name = findDxfString(entity_pairs, 3).value_or(QStringLiteral("Standard"));
        dimension.text_override = findDxfString(entity_pairs, 1).value_or(QString());
        if (dimension.text_override == QLatin1String("<>"))
        {
            dimension.text_override.clear();
        }
        entity.type = VpEntityType::LinearDimension;
        entity.geometry = std::move(dimension);
    }
    else if (entity_name == QLatin1String("HATCH"))
    {
        VpResult<VpHatchEntity> hatch = readDxfHatch(entity_pairs);
        if (!hatch)
        {
            return VpResult<VpEntityRecord>::failure(toCoreText(toQtError(hatch)));
        }
        entity.type = VpEntityType::Hatch;
        entity.geometry = std::move(hatch.value());
    }
    else if (entity_name == QLatin1String("SPLINE"))
    {
        const std::vector<VpPoint2d> control_points = readRepeatedPoints(entity_pairs, 10, 20);
        const int degree = static_cast<int>(findDxfDouble(entity_pairs, 71).value_or(3.0));
        if (degree != 3 || control_points.size() != 4)
        {
            return VpResult<VpEntityRecord>::failure(
                toCoreText(QObject::tr("仅支持四控制点三次 SPLINE，当前记录无法无损导入。")));
        }
        std::array<VpPoint2d, 4> points;
        std::copy(control_points.begin(), control_points.end(), points.begin());
        entity.type = VpEntityType::Spline;
        entity.geometry = VpSplineEntity{points};
    }
    else
    {
        return VpResult<VpEntityRecord>::failure(
            toCoreText(QObject::tr("暂不支持 DXF 实体：%1").arg(entity_name)));
    }
    return VpResult<VpEntityRecord>::success(std::move(entity));
}

bool writeDxfEntity(QTextStream& stream, const VpEntityRecord& entity,
                    VpFileCompatibilityReport& report, bool count_entity)
{
    if (entity.type == VpEntityType::Line)
    {
        const auto& line = std::get<VpLineEntity>(entity.geometry);
        writeDxfPair(stream, 0, QStringLiteral("LINE"));
        writeEntityCommon(stream, entity);
        writeDxfPair(stream, 10, line.start_point.x);
        writeDxfPair(stream, 20, line.start_point.y);
        writeDxfPair(stream, 30, 0.0);
        writeDxfPair(stream, 11, line.end_point.x);
        writeDxfPair(stream, 21, line.end_point.y);
        writeDxfPair(stream, 31, 0.0);
    }
    else if (entity.type == VpEntityType::Circle)
    {
        const auto& circle = std::get<VpCircleEntity>(entity.geometry);
        writeDxfPair(stream, 0, QStringLiteral("CIRCLE"));
        writeEntityCommon(stream, entity);
        writeDxfPair(stream, 10, circle.center.x);
        writeDxfPair(stream, 20, circle.center.y);
        writeDxfPair(stream, 30, 0.0);
        writeDxfPair(stream, 40, circle.radius);
    }
    else if (entity.type == VpEntityType::Arc)
    {
        const auto& arc = std::get<VpArcEntity>(entity.geometry);
        writeDxfPair(stream, 0, QStringLiteral("ARC"));
        writeEntityCommon(stream, entity);
        writeDxfPair(stream, 10, arc.center.x);
        writeDxfPair(stream, 20, arc.center.y);
        writeDxfPair(stream, 30, 0.0);
        writeDxfPair(stream, 40, arc.radius);
        writeDxfPair(stream, 50, arc.is_clockwise ? arc.end_angle : arc.start_angle);
        writeDxfPair(stream, 51, arc.is_clockwise ? arc.start_angle : arc.end_angle);
        if (count_entity && arc.is_clockwise)
        {
            report.warnings.append(
                QObject::tr("圆弧 ID %1 的加工方向无法写入 DXF；可见弧段已保留，方向信息已退化。")
                    .arg(entity.id));
        }
    }
    else if (entity.type == VpEntityType::Ellipse)
    {
        const auto& ellipse = std::get<VpEllipseEntity>(entity.geometry);
        const double major_length = std::hypot(ellipse.major_axis.x, ellipse.major_axis.y);
        const double minor_length = std::hypot(ellipse.minor_axis.x, ellipse.minor_axis.y);
        if (major_length <= 1.0e-12)
        {
            return false;
        }
        writeDxfPair(stream, 0, QStringLiteral("ELLIPSE"));
        writeEntityCommon(stream, entity);
        writeDxfPair(stream, 10, ellipse.center.x);
        writeDxfPair(stream, 20, ellipse.center.y);
        writeDxfPair(stream, 30, 0.0);
        writeDxfPair(stream, 11, ellipse.major_axis.x);
        writeDxfPair(stream, 21, ellipse.major_axis.y);
        writeDxfPair(stream, 31, 0.0);
        writeDxfPair(stream, 40, minor_length / major_length);
        writeDxfPair(stream, 41, 0.0);
        writeDxfPair(stream, 42, 6.28318530717958647692);
    }
    else if (entity.type == VpEntityType::Polyline)
    {
        const auto& polyline = std::get<VpPolylineEntity>(entity.geometry);
        writeDxfPair(stream, 0, QStringLiteral("LWPOLYLINE"));
        writeEntityCommon(stream, entity);
        writeDxfPair(stream, 90, QString::number(polyline.vertices.size()));
        writeDxfPair(stream, 70, polyline.is_closed ? QStringLiteral("1") : QStringLiteral("0"));
        for (std::size_t index = 0; index < polyline.vertices.size(); ++index)
        {
            writeDxfPair(stream, 10, polyline.vertices[index].x);
            writeDxfPair(stream, 20, polyline.vertices[index].y);
            if (index < polyline.bulges.size() && std::abs(polyline.bulges[index]) > 1.0e-12)
            {
                writeDxfPair(stream, 42, polyline.bulges[index]);
            }
            if (index < polyline.start_widths.size() && polyline.start_widths[index] > 0.0)
            {
                writeDxfPair(stream, 40, polyline.start_widths[index]);
            }
            if (index < polyline.end_widths.size() && polyline.end_widths[index] > 0.0)
            {
                writeDxfPair(stream, 41, polyline.end_widths[index]);
            }
        }
    }
    else if (entity.type == VpEntityType::Text)
    {
        const auto& text = std::get<VpTextEntity>(entity.geometry);
        writeDxfPair(stream, 0, QStringLiteral("TEXT"));
        writeEntityCommon(stream, entity);
        writeDxfPair(stream, 10, text.position.x);
        writeDxfPair(stream, 20, text.position.y);
        writeDxfPair(stream, 30, 0.0);
        writeDxfPair(stream, 40, text.height);
        writeDxfPair(stream, 1, text.text);
        writeDxfPair(stream, 50, text.rotation);
        writeDxfPair(stream, 7, text.style_name);
    }
    else if (entity.type == VpEntityType::MText)
    {
        const auto& text = std::get<VpMTextEntity>(entity.geometry);
        QTextDocument rich_document;
        rich_document.setHtml(text.rich_text);
        writeDxfPair(stream, 0, QStringLiteral("MTEXT"));
        writeEntityCommon(stream, entity);
        writeDxfPair(stream, 10, text.position.x);
        writeDxfPair(stream, 20, text.position.y);
        writeDxfPair(stream, 30, 0.0);
        writeDxfPair(stream, 40, text.height);
        writeDxfPair(stream, 41, text.width);
        writeDxfPair(stream, 1, rich_document.toPlainText());
        writeDxfPair(stream, 50, text.rotation);
        writeDxfPair(stream, 7, text.style_name);
        if (count_entity && Qt::mightBeRichText(text.rich_text))
        {
            report.warnings.append(
                QObject::tr("多行文字 ID %1 的富文本格式已降级为 DXF 纯文字。").arg(entity.id));
        }
    }
    else if (entity.type == VpEntityType::Leader)
    {
        if (count_entity)
        {
            report.warnings.append(
                QObject::tr("多重引线 ID %1 保留在 .smartcad 中；DXF MLEADER 映射待完成。")
                    .arg(entity.id));
        }
        return false;
    }
    else if (entity.type == VpEntityType::LinearDimension)
    {
        const auto& dimension = std::get<VpLinearDimensionEntity>(entity.geometry);
        writeDxfPair(stream, 0, QStringLiteral("DIMENSION"));
        writeEntityCommon(stream, entity);
        writeDxfPair(stream, 100, QStringLiteral("AcDbDimension"));
        writeDxfPair(stream, 2, QStringLiteral("*D%1").arg(entity.id));
        writeDxfPair(stream, 3, dimension.style_name);
        writeDxfPair(stream, 10, dimension.dimension_line_point.x);
        writeDxfPair(stream, 20, dimension.dimension_line_point.y);
        writeDxfPair(stream, 30, 0.0);
        writeDxfPair(stream, 13, dimension.first_point.x);
        writeDxfPair(stream, 23, dimension.first_point.y);
        writeDxfPair(stream, 14, dimension.second_point.x);
        writeDxfPair(stream, 24, dimension.second_point.y);
        writeDxfPair(stream, 15, dimension.center_point.x);
        writeDxfPair(stream, 25, dimension.center_point.y);
        writeDxfPair(stream, 70, QString::number(static_cast<int>(dimension.dimension_type)));
        writeDxfPair(stream, 1,
                     dimension.text_override.isEmpty() ? QStringLiteral("<>")
                                                       : dimension.text_override);
        writeDxfPair(stream, 1001, QStringLiteral("SMARTCAD"));
        writeDxfPair(stream, 1070, QString::number(static_cast<int>(dimension.dimension_type)));
    }
    else if (entity.type == VpEntityType::Hatch)
    {
        writeDxfHatch(stream, entity);
    }
    else if (entity.type == VpEntityType::Spline)
    {
        const auto& spline = std::get<VpSplineEntity>(entity.geometry);
        writeDxfPair(stream, 0, QStringLiteral("SPLINE"));
        writeEntityCommon(stream, entity);
        writeDxfPair(stream, 70, QStringLiteral("8"));
        writeDxfPair(stream, 71, QStringLiteral("3"));
        writeDxfPair(stream, 72, QStringLiteral("8"));
        writeDxfPair(stream, 73, QStringLiteral("4"));
        writeDxfPair(stream, 74, QStringLiteral("0"));
        for (int index = 0; index < 4; ++index)
        {
            writeDxfPair(stream, 40, index < 4 ? 0.0 : 1.0);
        }
        for (int index = 0; index < 4; ++index)
        {
            writeDxfPair(stream, 40, 1.0);
        }
        for (const VpPoint2d& point : spline.control_points)
        {
            writeDxfPair(stream, 10, point.x);
            writeDxfPair(stream, 20, point.y);
            writeDxfPair(stream, 30, 0.0);
        }
    }
    else
    {
        return false;
    }
    countExport(report, count_entity);
    return true;
}

} // namespace Vp
