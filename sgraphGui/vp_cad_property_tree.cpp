#include "vp_cad_property_tree.h"

#include "vp_cad_document.h"
#include "vp_cad_property_ui.h"
#include "vp_dimension_geometry.h"
#include "vp_entity.h"
#include "vp_layer_record.h"
#include "vp_qt_geometry.h"
#include "vp_spline_geometry.h"

#include <QFont>
#include <QObject>
#include <QTreeWidget>
#include <algorithm>
#include <cmath>
#include <vector>

namespace Vp
{
namespace
{

constexpr double kPi = 3.14159265358979323846;

QString entityTypeName(VpEntityType entity_type)
{
    switch (entity_type)
    {
    case VpEntityType::Line:
        return QObject::tr("直线");
    case VpEntityType::Circle:
        return QObject::tr("圆");
    case VpEntityType::Arc:
        return QObject::tr("圆弧");
    case VpEntityType::Polyline:
        return QObject::tr("多段线");
    case VpEntityType::Text:
        return QObject::tr("文字");
    case VpEntityType::LinearDimension:
        return QObject::tr("线性标注");
    case VpEntityType::Hatch:
        return QObject::tr("填充");
    case VpEntityType::Spline:
        return QObject::tr("样条曲线");
    case VpEntityType::Ellipse:
        return QObject::tr("椭圆");
    case VpEntityType::MText:
        return QObject::tr("多行文字");
    case VpEntityType::Leader:
        return QObject::tr("多重引线");
    }
    return QObject::tr("未知实体");
}

void addProperty(QTreeWidgetItem* parent_item, const QString& name, const QString& value)
{
    auto* property_item = new QTreeWidgetItem(parent_item, {name, value});
    property_item->setToolTip(0, name);
    property_item->setToolTip(1, value);
}

double polygonArea(const std::vector<VpPoint2d>& vertices)
{
    if (vertices.size() < 3)
    {
        return 0.0;
    }
    double twice_area = 0.0;
    for (std::size_t index = 0; index < vertices.size(); ++index)
    {
        const VpPoint2d& current = vertices[index];
        const VpPoint2d& next = vertices[(index + 1) % vertices.size()];
        twice_area += (current.x * next.y) - (next.x * current.y);
    }
    return std::abs(twice_area) * 0.5;
}

} // namespace

QTreeWidgetItem* addEntityPropertyTree(QTreeWidgetItem* parent_item, const VpEntityRecord& entity,
                                       const VpCadDocument& document, const VpDesignToken& tokens)
{
    auto* entity_item = new QTreeWidgetItem(
        parent_item,
        {QStringLiteral("%1 #%2").arg(entityTypeName(entity.type)).arg(entity.id), QString()});
    entity_item->setData(0, Qt::UserRole, static_cast<qulonglong>(entity.id));
    QFont entity_font = entity_item->font(0);
    entity_font.setBold(true);
    entity_item->setFont(0, entity_font);
    entity_item->setBackground(0, tokens.elevated_surface);
    entity_item->setBackground(1, tokens.elevated_surface);

    auto* general_item = new QTreeWidgetItem(entity_item, {QObject::tr("常规"), QString()});
    QFont section_font = general_item->font(0);
    section_font.setBold(true);
    general_item->setFont(0, section_font);
    general_item->setForeground(0, tokens.text_secondary);
    addProperty(general_item, QObject::tr("ID"), QString::number(entity.id));
    addProperty(general_item, QObject::tr("类型"), entityTypeName(entity.type));
    const VpLayerRecord* layer_record = document.layer(entity.layer_name);
    addProperty(general_item, QObject::tr("图层"),
                layer_record ? QObject::tr("[%1] %2").arg(layer_record->id).arg(layer_record->name)
                             : entity.layer_name);
    addProperty(
        general_item, QObject::tr("颜色"),
        QObject::tr("随图层 (%1)").arg(document.layerColor(entity.layer_name).name().toUpper()));
    addProperty(general_item, QObject::tr("线宽"),
                entity.line_width_mm < 0.0
                    ? QObject::tr("ByLayer (%1)")
                          .arg(lineWidthText(document.layerLineWidth(entity.layer_name), false))
                    : lineWidthText(entity.line_width_mm, false));
    if (entity.associative_array)
    {
        const VpAssociativeArrayData& array_data = *entity.associative_array;
        auto* array_item = new QTreeWidgetItem(entity_item, {QObject::tr("关联阵列"), QString()});
        array_item->setFont(0, section_font);
        array_item->setForeground(0, tokens.text_secondary);
        const QString type_name =
            array_data.array_type == VpArrayType::Rectangular
                ? QObject::tr("矩形")
                : (array_data.array_type == VpArrayType::Polar ? QObject::tr("环形")
                                                               : QObject::tr("路径"));
        addProperty(array_item, QObject::tr("阵列 ID"), QString::number(array_data.array_id));
        addProperty(array_item, QObject::tr("阵列类型"), type_name);
        addProperty(array_item, QObject::tr("源索引"),
                    QString::number(array_data.source_index + 1));
        addProperty(array_item, QObject::tr("项目索引"),
                    QString::number(array_data.item_index + 1));
        addProperty(array_item, QObject::tr("项目数"), QString::number(array_data.item_count));
        if (array_data.array_type == VpArrayType::Rectangular)
        {
            addProperty(
                array_item, QObject::tr("列 × 行"),
                QObject::tr("%1 × %2").arg(array_data.column_count).arg(array_data.row_count));
            addProperty(array_item, QObject::tr("列间距"),
                        QString::number(array_data.column_spacing, 'f', 3));
            addProperty(array_item, QObject::tr("行间距"),
                        QString::number(array_data.row_spacing, 'f', 3));
        }
        else if (array_data.array_type == VpArrayType::Polar)
        {
            addProperty(array_item, QObject::tr("中心"), formatPoint(array_data.center));
            addProperty(array_item, QObject::tr("填充角"),
                        QString::number(array_data.fill_angle, 'f', 3));
        }
        else
        {
            addProperty(array_item, QObject::tr("路径实体 ID"),
                        QString::number(array_data.path_entity_id));
            addProperty(array_item, QObject::tr("沿路径对齐"),
                        array_data.align_to_path ? QObject::tr("是") : QObject::tr("否"));
        }
    }

    auto* geometry_item = new QTreeWidgetItem(entity_item, {QObject::tr("几何"), QString()});
    geometry_item->setFont(0, section_font);
    geometry_item->setForeground(0, tokens.text_secondary);
    if (entity.type == VpEntityType::Line)
    {
        const auto& line = std::get<VpLineEntity>(entity.geometry);
        addProperty(geometry_item, QObject::tr("起点"), formatPoint(line.start_point));
        addProperty(geometry_item, QObject::tr("端点"), formatPoint(line.end_point));
        addProperty(geometry_item, QObject::tr("长度"),
                    QString::number(distance(line.start_point, line.end_point), 'f', 3));
        addProperty(geometry_item, QObject::tr("角度"),
                    QString::number(std::atan2(line.end_point.y - line.start_point.y,
                                               line.end_point.x - line.start_point.x) *
                                        180.0 / kPi,
                                    'f', 3));
    }
    else if (entity.type == VpEntityType::Circle)
    {
        const auto& circle = std::get<VpCircleEntity>(entity.geometry);
        addProperty(geometry_item, QObject::tr("圆心"), formatPoint(circle.center));
        addProperty(geometry_item, QObject::tr("半径"), QString::number(circle.radius, 'f', 3));
        addProperty(geometry_item, QObject::tr("直径"),
                    QString::number(circle.radius * 2.0, 'f', 3));
        addProperty(geometry_item, QObject::tr("周长"),
                    QString::number(circle.radius * 2.0 * kPi, 'f', 3));
        addProperty(geometry_item, QObject::tr("面积"),
                    QString::number(circle.radius * circle.radius * kPi, 'f', 3));
    }
    else if (entity.type == VpEntityType::Arc)
    {
        const auto& arc = std::get<VpArcEntity>(entity.geometry);
        double span_angle = arc.end_angle - arc.start_angle;
        if (span_angle <= 0.0)
        {
            span_angle += 360.0;
        }
        addProperty(geometry_item, QObject::tr("圆心"), formatPoint(arc.center));
        addProperty(geometry_item, QObject::tr("半径"), QString::number(arc.radius, 'f', 3));
        addProperty(geometry_item, QObject::tr("起始角"), QString::number(arc.start_angle, 'f', 3));
        addProperty(geometry_item, QObject::tr("终止角"), QString::number(arc.end_angle, 'f', 3));
        addProperty(geometry_item, QObject::tr("圆弧长度"),
                    QString::number(arc.radius * span_angle * kPi / 180.0, 'f', 3));
    }
    else if (entity.type == VpEntityType::Polyline)
    {
        const auto& polyline = std::get<VpPolylineEntity>(entity.geometry);
        double total_length = 0.0;
        for (std::size_t index = 1; index < polyline.vertices.size(); ++index)
        {
            total_length += distance(polyline.vertices[index - 1], polyline.vertices[index]);
        }
        if (polyline.is_closed && polyline.vertices.size() > 2)
        {
            total_length += distance(polyline.vertices.back(), polyline.vertices.front());
        }
        addProperty(geometry_item, QObject::tr("顶点数"),
                    QString::number(polyline.vertices.size()));
        addProperty(geometry_item, QObject::tr("闭合"),
                    polyline.is_closed ? QObject::tr("是") : QObject::tr("否"));
        addProperty(geometry_item, QObject::tr("总长度"), QString::number(total_length, 'f', 3));
        const double maximum_width = std::max(
            polyline.start_widths.empty()
                ? 0.0
                : *std::max_element(polyline.start_widths.begin(), polyline.start_widths.end()),
            polyline.end_widths.empty()
                ? 0.0
                : *std::max_element(polyline.end_widths.begin(), polyline.end_widths.end()));
        addProperty(geometry_item, QObject::tr("最大几何宽度"),
                    QString::number(maximum_width, 'f', 3));
        if (polyline.is_closed)
        {
            addProperty(geometry_item, QObject::tr("面积"),
                        QString::number(polygonArea(polyline.vertices), 'f', 3));
        }
        auto* vertices_item = new QTreeWidgetItem(geometry_item, {QObject::tr("顶点"), QString()});
        for (std::size_t index = 0; index < polyline.vertices.size(); ++index)
        {
            addProperty(vertices_item, QObject::tr("顶点 %1").arg(index + 1),
                        formatPoint(polyline.vertices[index]));
        }
    }
    else if (entity.type == VpEntityType::Text)
    {
        const auto& text = std::get<VpTextEntity>(entity.geometry);
        addProperty(geometry_item, QObject::tr("内容"), text.text);
        addProperty(geometry_item, QObject::tr("插入点"), formatPoint(text.position));
        addProperty(geometry_item, QObject::tr("高度"), QString::number(text.height, 'f', 3));
        addProperty(geometry_item, QObject::tr("旋转"), QString::number(text.rotation, 'f', 3));
        addProperty(geometry_item, QObject::tr("文字样式"), text.style_name);
    }
    else if (entity.type == VpEntityType::LinearDimension)
    {
        const auto& dimension = std::get<VpLinearDimensionEntity>(entity.geometry);
        addProperty(geometry_item, QObject::tr("标注类型"),
                    dimensionTypeName(dimension.dimension_type));
        addProperty(geometry_item, QObject::tr("第一原点"), formatPoint(dimension.first_point));
        addProperty(geometry_item, QObject::tr("第二原点"), formatPoint(dimension.second_point));
        addProperty(geometry_item, QObject::tr("尺寸线点"),
                    formatPoint(dimension.dimension_line_point));
        if (dimension.dimension_type != VpDimensionType::Linear &&
            dimension.dimension_type != VpDimensionType::Aligned)
        {
            addProperty(geometry_item, QObject::tr("中心/原点"),
                        formatPoint(dimension.center_point));
        }
        addProperty(geometry_item, QObject::tr("标注样式"), dimension.style_name);
        addProperty(geometry_item, QObject::tr("测量值"),
                    QString::number(dimensionMeasurement(dimension), 'f', 3));
    }
    else if (entity.type == VpEntityType::Hatch)
    {
        const auto& hatch = std::get<VpHatchEntity>(entity.geometry);
        QString fill_type = QObject::tr("实体");
        if (hatch.fill_type == VpHatchFillType::Pattern)
        {
            fill_type = QObject::tr("图案");
        }
        else if (hatch.fill_type == VpHatchFillType::Gradient)
        {
            fill_type = QObject::tr("渐变");
        }
        addProperty(geometry_item, QObject::tr("填充类型"), fill_type);
        addProperty(geometry_item, QObject::tr("图案"), hatch.pattern_name);
        addProperty(geometry_item, QObject::tr("图案比例"),
                    QString::number(hatch.pattern_scale, 'f', 3));
        addProperty(geometry_item, QObject::tr("图案角度"),
                    QString::number(hatch.pattern_angle, 'f', 2));
        addProperty(geometry_item, QObject::tr("岛数量"),
                    QString::number(hatch.island_boundaries.size()));
        addProperty(geometry_item, QObject::tr("关联边界 ID"),
                    QString::number(hatch.associative_boundary_id));
        addProperty(geometry_item, QObject::tr("边界顶点"), QString::number(hatch.boundary.size()));
        double hatch_area = polygonArea(hatch.boundary);
        for (const std::vector<VpPoint2d>& island : hatch.island_boundaries)
        {
            hatch_area -= polygonArea(island);
        }
        addProperty(geometry_item, QObject::tr("净面积"), QString::number(hatch_area, 'f', 3));
        auto* vertices_item = new QTreeWidgetItem(geometry_item, {QObject::tr("边界"), QString()});
        for (std::size_t index = 0; index < hatch.boundary.size(); ++index)
        {
            addProperty(vertices_item, QObject::tr("顶点 %1").arg(index + 1),
                        formatPoint(hatch.boundary[index]));
        }
    }
    else if (entity.type == VpEntityType::Spline)
    {
        const auto& spline = std::get<VpSplineEntity>(entity.geometry);
        addProperty(geometry_item, QObject::tr("次数"), QStringLiteral("3"));
        addProperty(geometry_item, QObject::tr("近似长度"),
                    QString::number(splineApproximateLength(spline), 'f', 3));
        auto* controls_item =
            new QTreeWidgetItem(geometry_item, {QObject::tr("控制点"), QString()});
        for (std::size_t index = 0; index < spline.control_points.size(); ++index)
        {
            addProperty(controls_item, QObject::tr("控制点 %1").arg(index + 1),
                        formatPoint(spline.control_points[index]));
        }
    }
    else if (entity.type == VpEntityType::Ellipse)
    {
        const auto& ellipse = std::get<VpEllipseEntity>(entity.geometry);
        const double major_length = std::hypot(ellipse.major_axis.x, ellipse.major_axis.y);
        const double minor_length = std::hypot(ellipse.minor_axis.x, ellipse.minor_axis.y);
        addProperty(geometry_item, QObject::tr("中心"), formatPoint(ellipse.center));
        addProperty(geometry_item, QObject::tr("长半轴"), QString::number(major_length, 'f', 3));
        addProperty(geometry_item, QObject::tr("短半轴"), QString::number(minor_length, 'f', 3));
        addProperty(
            geometry_item, QObject::tr("旋转角"),
            QString::number(std::atan2(ellipse.major_axis.y, ellipse.major_axis.x) * 180.0 / kPi,
                            'f', 3));
        addProperty(geometry_item, QObject::tr("面积"),
                    QString::number(kPi * major_length * minor_length, 'f', 3));
    }
    else if (entity.type == VpEntityType::MText)
    {
        const auto& text = std::get<VpMTextEntity>(entity.geometry);
        addProperty(geometry_item, QObject::tr("内容"), text.rich_text);
        addProperty(geometry_item, QObject::tr("插入点"), formatPoint(text.position));
        addProperty(geometry_item, QObject::tr("边界宽度"), QString::number(text.width, 'f', 3));
        addProperty(geometry_item, QObject::tr("文字高度"), QString::number(text.height, 'f', 3));
        addProperty(geometry_item, QObject::tr("旋转"), QString::number(text.rotation, 'f', 3));
        addProperty(geometry_item, QObject::tr("文字样式"), text.style_name);
    }
    else if (entity.type == VpEntityType::Leader)
    {
        const auto& leader = std::get<VpLeaderEntity>(entity.geometry);
        addProperty(geometry_item, QObject::tr("内容"), leader.text);
        addProperty(geometry_item, QObject::tr("顶点数"), QString::number(leader.vertices.size()));
        addProperty(geometry_item, QObject::tr("文字高度"),
                    QString::number(leader.text_height, 'f', 3));
        addProperty(geometry_item, QObject::tr("箭头大小"),
                    QString::number(leader.arrow_size, 'f', 3));
        addProperty(geometry_item, QObject::tr("文字样式"), leader.style_name);
        auto* vertices_item = new QTreeWidgetItem(geometry_item, {QObject::tr("引线顶点"), {}});
        for (std::size_t index = 0; index < leader.vertices.size(); ++index)
        {
            addProperty(vertices_item, QObject::tr("顶点 %1").arg(index + 1),
                        formatPoint(leader.vertices[index]));
        }
    }
    return entity_item;
}

} // namespace Vp
