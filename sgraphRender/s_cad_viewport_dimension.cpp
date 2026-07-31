#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_dimension_geometry.h"
#include "s_dimension_style_record.h"
#include "s_document_transaction.h"

#include <QPainter>
#include <QPolygonF>
#include <algorithm>
#include <cmath>
#include <utility>

namespace smartCam
{
namespace
{

constexpr double kPi = 3.14159265358979323846;

double pointDistance(const SPoint2d& first_point, const SPoint2d& second_point)
{
    return std::hypot(second_point.x - first_point.x, second_point.y - first_point.y);
}

double shortestSweep(double start_angle, double end_angle)
{
    double sweep = end_angle - start_angle;
    while (sweep <= -kPi)
    {
        sweep += 2.0 * kPi;
    }
    while (sweep > kPi)
    {
        sweep -= 2.0 * kPi;
    }
    return sweep;
}

class SPainterStateGuard final
{
  public:
    explicit SPainterStateGuard(QPainter& painter) : m_painter(painter)
    {
        m_painter.save();
    }

    ~SPainterStateGuard()
    {
        m_painter.restore();
    }

  private:
    QPainter& m_painter;
};

void drawArrow(QPainter& painter, const QPointF& tip, const QPointF& toward, double arrow_size)
{
    const QPointF direction = toward - tip;
    const double length = std::hypot(direction.x(), direction.y());
    if (length <= 1.0e-6)
    {
        return;
    }
    const QPointF unit = direction / length;
    const QPointF normal{-unit.y(), unit.x()};
    QPolygonF arrow;
    arrow << tip << tip + (unit * arrow_size) + (normal * arrow_size * 0.44)
          << tip + (unit * arrow_size) - (normal * arrow_size * 0.44);
    painter.save();
    painter.setBrush(painter.pen().color());
    painter.drawPolygon(arrow);
    painter.restore();
}

QString suppressTrailingZeros(QString value)
{
    if (value.contains(QLatin1Char('.')))
    {
        while (value.endsWith(QLatin1Char('0')))
        {
            value.chop(1);
        }
        if (value.endsWith(QLatin1Char('.')))
        {
            value.chop(1);
        }
    }
    return value;
}

QString styledDimensionText(const SLinearDimensionEntity& dimension,
                            const SDimensionStyleRecord& style)
{
    if (!dimension.text_override.isEmpty())
    {
        return dimension.text_override;
    }
    double value = dimensionMeasurement(dimension);
    if (dimension.dimension_type != SDimensionType::Angular)
    {
        value *= style.linear_scale;
    }
    const int precision = dimension.dimension_type == SDimensionType::Angular
                              ? style.angular_precision
                              : style.linear_precision;
    QString measurement = QString::number(value, 'f', precision);
    if (style.suppress_trailing_zeros)
    {
        measurement = suppressTrailingZeros(std::move(measurement));
    }
    if (dimension.dimension_type == SDimensionType::Angular)
    {
        measurement += QChar(0x00B0);
    }
    else if (dimension.dimension_type == SDimensionType::Radius)
    {
        measurement.prepend(QLatin1Char('R'));
    }
    else if (dimension.dimension_type == SDimensionType::Diameter)
    {
        measurement.prepend(QChar(0x2300));
    }
    else if (dimension.dimension_type == SDimensionType::ArcLength)
    {
        measurement.prepend(QChar(0x2312));
    }
    return style.prefix + measurement + style.suffix;
}

SLinearDimensionEntity previewDimension(SDimensionType dimension_type,
                                        const std::vector<SPoint2d>& input_points,
                                        const SPoint2d& cursor)
{
    SLinearDimensionEntity dimension;
    dimension.dimension_type = dimension_type;
    if (dimension_type == SDimensionType::Angular || dimension_type == SDimensionType::ArcLength)
    {
        dimension.center_point = input_points[0];
        dimension.first_point = input_points.size() > 1 ? input_points[1] : cursor;
        dimension.second_point = input_points.size() > 2 ? input_points[2] : cursor;
        dimension.dimension_line_point = input_points.size() > 3 ? input_points[3] : cursor;
    }
    else if (dimension_type == SDimensionType::Radius ||
             dimension_type == SDimensionType::Diameter ||
             dimension_type == SDimensionType::Ordinate)
    {
        dimension.center_point = input_points[0];
        dimension.first_point = input_points.size() > 1 ? input_points[1] : cursor;
        dimension.second_point = dimension.first_point;
        dimension.dimension_line_point = input_points.size() > 2 ? input_points[2] : cursor;
    }
    else
    {
        dimension.first_point = input_points[0];
        dimension.second_point = input_points.size() > 1 ? input_points[1] : cursor;
        dimension.dimension_line_point = input_points.size() > 2 ? input_points[2] : cursor;
    }
    return dimension;
}

QString firstDimensionPrompt(SDimensionType dimension_type)
{
    switch (dimension_type)
    {
    case SDimensionType::Linear:
        return QStringLiteral("DIMLINEAR 指定第一条尺寸界线原点：");
    case SDimensionType::Aligned:
        return QStringLiteral("DIMALIGNED 指定第一条尺寸界线原点：");
    case SDimensionType::Angular:
        return QStringLiteral("DIMANGULAR 指定角点：");
    case SDimensionType::Radius:
        return QStringLiteral("DIMRADIUS 指定圆心：");
    case SDimensionType::Diameter:
        return QStringLiteral("DIMDIAMETER 指定圆心：");
    case SDimensionType::ArcLength:
        return QStringLiteral("DIMARC 指定圆心：");
    case SDimensionType::Ordinate:
        return QStringLiteral("DIMORDINATE 指定原点：");
    }
    return {};
}

QString nextDimensionPrompt(SDimensionType dimension_type, std::size_t point_count)
{
    if (dimension_type == SDimensionType::Angular || dimension_type == SDimensionType::ArcLength)
    {
        if (point_count == 1)
        {
            return dimension_type == SDimensionType::Angular
                       ? QStringLiteral("指定第一条射线上的点：")
                       : QStringLiteral("指定圆弧起点：");
        }
        if (point_count == 2)
        {
            return dimension_type == SDimensionType::Angular
                       ? QStringLiteral("指定第二条射线上的点：")
                       : QStringLiteral("指定圆弧端点：");
        }
        return QStringLiteral("指定标注弧线位置：");
    }
    if (point_count == 1)
    {
        if (dimension_type == SDimensionType::Radius || dimension_type == SDimensionType::Diameter)
        {
            return QStringLiteral("指定圆或圆弧上的点：");
        }
        if (dimension_type == SDimensionType::Ordinate)
        {
            return QStringLiteral("指定要标注的特征点：");
        }
        return QStringLiteral("指定第二条尺寸界线原点：");
    }
    return dimension_type == SDimensionType::Ordinate ? QStringLiteral("指定引线端点：")
                                                      : QStringLiteral("指定尺寸线位置：");
}

} // namespace

std::optional<SDimensionType> dimensionTypeForToolMode(SToolMode tool_mode) noexcept
{
    switch (tool_mode)
    {
    case SToolMode::LinearDimension:
        return SDimensionType::Linear;
    case SToolMode::AlignedDimension:
        return SDimensionType::Aligned;
    case SToolMode::AngularDimension:
        return SDimensionType::Angular;
    case SToolMode::RadiusDimension:
        return SDimensionType::Radius;
    case SToolMode::DiameterDimension:
        return SDimensionType::Diameter;
    case SToolMode::ArcLengthDimension:
        return SDimensionType::ArcLength;
    case SToolMode::OrdinateDimension:
        return SDimensionType::Ordinate;
    default:
        return std::nullopt;
    }
}

void SCadViewport::acceptDimensionPoint(const SPoint2d& world_point)
{
    const std::optional<SDimensionType> dimension_type = dimensionTypeForToolMode(m_tool_mode);
    if (!m_document || !dimension_type)
    {
        return;
    }
    m_input_points.push_back(world_point);
    const std::size_t required_count =
        (*dimension_type == SDimensionType::Angular || *dimension_type == SDimensionType::ArcLength)
            ? 4U
            : 3U;
    if (m_input_points.size() < required_count)
    {
        emit commandMessage(nextDimensionPrompt(*dimension_type, m_input_points.size()));
        update();
        return;
    }

    SLinearDimensionEntity dimension =
        previewDimension(*dimension_type, m_input_points, m_input_points.back());
    if (!isDimensionValid(dimension))
    {
        m_input_points.pop_back();
        emit commandMessage(tr("标注定义点无效，请重新指定："));
        update();
        return;
    }
    auto transaction =
        m_document->beginTransaction(tr("创建%1标注").arg(dimensionTypeName(*dimension_type)));
    transaction->addDimension(std::move(dimension));
    transaction->commit();
    m_input_points.clear();
    emit commandMessage(firstDimensionPrompt(*dimension_type));
    update();
}

void SCadViewport::drawDimensionPreview(QPainter& painter)
{
    const std::optional<SDimensionType> dimension_type = dimensionTypeForToolMode(m_tool_mode);
    if (!dimension_type || m_input_points.empty())
    {
        return;
    }
    SLinearDimensionEntity dimension =
        previewDimension(*dimension_type, m_input_points, m_cursor_world);
    if (m_document)
    {
        dimension.style_name = m_document->currentDimensionStyleName();
    }
    painter.save();
    painter.setPen(QPen(QColor(92, 214, 255), 1.4, Qt::DashLine));
    if (m_input_points.size() == 1 &&
        (*dimension_type == SDimensionType::Linear || *dimension_type == SDimensionType::Aligned))
    {
        painter.drawLine(worldToScreen(m_input_points.front()), worldToScreen(m_cursor_world));
    }
    else
    {
        drawDimensionEntity(painter, dimension);
    }
    painter.restore();
}

void SCadViewport::drawDimensionEntity(QPainter& painter, const SLinearDimensionEntity& dimension)
{
    SPainterStateGuard painter_guard(painter);
    SDimensionStyleRecord fallback_style;
    const SDimensionStyleRecord* style =
        m_document ? m_document->dimensionStyle(dimension.style_name) : nullptr;
    if (!style)
    {
        style = &fallback_style;
    }
    const QString text = styledDimensionText(dimension, *style);
    QFont dimension_font = painter.font();
    dimension_font.setPixelSize(static_cast<int>(
        std::clamp(style->text_height * style->overall_scale * m_zoom, 8.0, 96.0)));
    painter.setFont(dimension_font);
    const double arrow_size =
        std::clamp(style->arrow_size * style->overall_scale * m_zoom, 4.0, 24.0);

    if (dimension.dimension_type == SDimensionType::Linear ||
        dimension.dimension_type == SDimensionType::Aligned)
    {
        SPoint2d first_dimension_point;
        SPoint2d second_dimension_point;
        if (dimension.dimension_type == SDimensionType::Linear)
        {
            const SPoint2d midpoint{(dimension.first_point.x + dimension.second_point.x) * 0.5,
                                    (dimension.first_point.y + dimension.second_point.y) * 0.5};
            const bool is_vertical = std::abs(dimension.dimension_line_point.x - midpoint.x) >
                                     std::abs(dimension.dimension_line_point.y - midpoint.y);
            first_dimension_point =
                is_vertical ? SPoint2d{dimension.dimension_line_point.x, dimension.first_point.y}
                            : SPoint2d{dimension.first_point.x, dimension.dimension_line_point.y};
            second_dimension_point =
                is_vertical ? SPoint2d{dimension.dimension_line_point.x, dimension.second_point.y}
                            : SPoint2d{dimension.second_point.x, dimension.dimension_line_point.y};
        }
        else
        {
            const double delta_x = dimension.second_point.x - dimension.first_point.x;
            const double delta_y = dimension.second_point.y - dimension.first_point.y;
            const double length = std::hypot(delta_x, delta_y);
            if (length <= 1.0e-9)
            {
                return;
            }
            const double normal_x = -delta_y / length;
            const double normal_y = delta_x / length;
            const double offset =
                ((dimension.dimension_line_point.x - dimension.first_point.x) * normal_x) +
                ((dimension.dimension_line_point.y - dimension.first_point.y) * normal_y);
            first_dimension_point = {dimension.first_point.x + (normal_x * offset),
                                     dimension.first_point.y + (normal_y * offset)};
            second_dimension_point = {dimension.second_point.x + (normal_x * offset),
                                      dimension.second_point.y + (normal_y * offset)};
        }
        const QPointF first_screen = worldToScreen(first_dimension_point);
        const QPointF second_screen = worldToScreen(second_dimension_point);
        painter.drawLine(worldToScreen(dimension.first_point), first_screen);
        painter.drawLine(worldToScreen(dimension.second_point), second_screen);
        painter.drawLine(first_screen, second_screen);
        drawArrow(painter, first_screen, second_screen, arrow_size);
        drawArrow(painter, second_screen, first_screen, arrow_size);
        painter.drawText((first_screen + second_screen) * 0.5 + QPointF(4.0, -5.0), text);
        return;
    }

    if (dimension.dimension_type == SDimensionType::Radius ||
        dimension.dimension_type == SDimensionType::Diameter)
    {
        const SPoint2d center = dimension.center_point;
        const double radius = pointDistance(center, dimension.first_point);
        if (radius <= 1.0e-9)
        {
            return;
        }
        const double unit_x = (dimension.first_point.x - center.x) / radius;
        const double unit_y = (dimension.first_point.y - center.y) / radius;
        const SPoint2d start =
            dimension.dimension_type == SDimensionType::Diameter
                ? SPoint2d{center.x - (unit_x * radius), center.y - (unit_y * radius)}
                : center;
        painter.drawLine(worldToScreen(start), worldToScreen(dimension.first_point));
        painter.drawLine(worldToScreen(dimension.first_point),
                         worldToScreen(dimension.dimension_line_point));
        drawArrow(painter, worldToScreen(dimension.first_point), worldToScreen(center), arrow_size);
        painter.drawText(worldToScreen(dimension.dimension_line_point) + QPointF(5.0, -5.0), text);
        return;
    }

    if (dimension.dimension_type == SDimensionType::Ordinate)
    {
        const QPointF feature = worldToScreen(dimension.first_point);
        const QPointF leader = worldToScreen(dimension.dimension_line_point);
        const bool horizontal =
            std::abs(leader.x() - feature.x()) > std::abs(leader.y() - feature.y());
        const QPointF elbow = horizontal ? QPointF(leader.x() - 12.0, feature.y())
                                         : QPointF(feature.x(), leader.y() + 12.0);
        painter.drawLine(feature, elbow);
        painter.drawLine(elbow, leader);
        painter.drawText(leader + QPointF(5.0, -5.0), text);
        return;
    }

    const double radius = pointDistance(dimension.center_point, dimension.dimension_line_point);
    if (radius <= 1.0e-9)
    {
        return;
    }
    const double start_angle = std::atan2(dimension.first_point.y - dimension.center_point.y,
                                          dimension.first_point.x - dimension.center_point.x);
    const double end_angle = std::atan2(dimension.second_point.y - dimension.center_point.y,
                                        dimension.second_point.x - dimension.center_point.x);
    const double sweep = shortestSweep(start_angle, end_angle);
    QPolygonF arc_points;
    constexpr int kSegmentCount = 48;
    for (int index = 0; index <= kSegmentCount; ++index)
    {
        const double angle = start_angle + (sweep * index / kSegmentCount);
        arc_points << worldToScreen({dimension.center_point.x + (std::cos(angle) * radius),
                                     dimension.center_point.y + (std::sin(angle) * radius)});
    }
    painter.drawLine(worldToScreen(dimension.center_point), worldToScreen(dimension.first_point));
    painter.drawLine(worldToScreen(dimension.center_point), worldToScreen(dimension.second_point));
    painter.drawPolyline(arc_points);
    if (arc_points.size() >= 2)
    {
        drawArrow(painter, arc_points.front(), arc_points[1], arrow_size);
        drawArrow(painter, arc_points.back(), arc_points[arc_points.size() - 2], arrow_size);
        painter.drawText(arc_points[arc_points.size() / 2] + QPointF(5.0, -5.0), text);
    }
}

} // namespace smartCam
