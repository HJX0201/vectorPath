#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_ellipse_geometry.h"
#include "s_spline_geometry.h"

#include <algorithm>
#include <limits>

namespace smartGraphics
{
namespace
{

constexpr double kMinimumZoom = 0.01;
constexpr double kMaximumZoom = 10000.0;

} // namespace

void SCadViewport::zoomExtents()
{
    if (!m_document || m_document->entities().empty())
    {
        m_zoom = 1.0;
        m_pan_offset = {};
        update();
        return;
    }

    double minimum_x = std::numeric_limits<double>::max();
    double minimum_y = std::numeric_limits<double>::max();
    double maximum_x = std::numeric_limits<double>::lowest();
    double maximum_y = std::numeric_limits<double>::lowest();
    for (const SEntityRecord& entity : m_document->entities())
    {
        if (!m_document->isLayerVisible(entity.layer_name))
        {
            continue;
        }
        if (entity.type == SEntityType::Line)
        {
            const auto& line = std::get<SLineEntity>(entity.geometry);
            minimum_x = std::min({minimum_x, line.start_point.x, line.end_point.x});
            minimum_y = std::min({minimum_y, line.start_point.y, line.end_point.y});
            maximum_x = std::max({maximum_x, line.start_point.x, line.end_point.x});
            maximum_y = std::max({maximum_y, line.start_point.y, line.end_point.y});
        }
        else if (entity.type == SEntityType::Circle)
        {
            const auto& circle = std::get<SCircleEntity>(entity.geometry);
            minimum_x = std::min(minimum_x, circle.center.x - circle.radius);
            minimum_y = std::min(minimum_y, circle.center.y - circle.radius);
            maximum_x = std::max(maximum_x, circle.center.x + circle.radius);
            maximum_y = std::max(maximum_y, circle.center.y + circle.radius);
        }
        else if (entity.type == SEntityType::Arc)
        {
            const auto& arc = std::get<SArcEntity>(entity.geometry);
            minimum_x = std::min(minimum_x, arc.center.x - arc.radius);
            minimum_y = std::min(minimum_y, arc.center.y - arc.radius);
            maximum_x = std::max(maximum_x, arc.center.x + arc.radius);
            maximum_y = std::max(maximum_y, arc.center.y + arc.radius);
        }
        else if (entity.type == SEntityType::Polyline)
        {
            const auto& polyline = std::get<SPolylineEntity>(entity.geometry);
            const double half_width =
                std::max(
                    polyline.start_widths.empty() ? 0.0
                                                  : *std::max_element(polyline.start_widths.begin(),
                                                                      polyline.start_widths.end()),
                    polyline.end_widths.empty() ? 0.0
                                                : *std::max_element(polyline.end_widths.begin(),
                                                                    polyline.end_widths.end())) *
                0.5;
            for (const SPoint2d& vertex : polyline.vertices)
            {
                minimum_x = std::min(minimum_x, vertex.x - half_width);
                minimum_y = std::min(minimum_y, vertex.y - half_width);
                maximum_x = std::max(maximum_x, vertex.x + half_width);
                maximum_y = std::max(maximum_y, vertex.y + half_width);
            }
        }
        else if (entity.type == SEntityType::Text)
        {
            const auto& text = std::get<STextEntity>(entity.geometry);
            minimum_x = std::min(minimum_x, text.position.x);
            minimum_y = std::min(minimum_y, text.position.y);
            maximum_x = std::max(maximum_x, text.position.x + text.height * text.text.size());
            maximum_y = std::max(maximum_y, text.position.y + text.height);
        }
        else if (entity.type == SEntityType::LinearDimension)
        {
            const auto& dimension = std::get<SLinearDimensionEntity>(entity.geometry);
            for (const SPoint2d& point :
                 {dimension.first_point, dimension.second_point, dimension.dimension_line_point})
            {
                minimum_x = std::min(minimum_x, point.x);
                minimum_y = std::min(minimum_y, point.y);
                maximum_x = std::max(maximum_x, point.x);
                maximum_y = std::max(maximum_y, point.y);
            }
        }
        else if (entity.type == SEntityType::Hatch)
        {
            for (const SPoint2d& vertex : std::get<SHatchEntity>(entity.geometry).boundary)
            {
                minimum_x = std::min(minimum_x, vertex.x);
                minimum_y = std::min(minimum_y, vertex.y);
                maximum_x = std::max(maximum_x, vertex.x);
                maximum_y = std::max(maximum_y, vertex.y);
            }
        }
        else if (entity.type == SEntityType::Spline)
        {
            for (const SPoint2d& point :
                 splineApproximation(std::get<SSplineEntity>(entity.geometry)))
            {
                minimum_x = std::min(minimum_x, point.x);
                minimum_y = std::min(minimum_y, point.y);
                maximum_x = std::max(maximum_x, point.x);
                maximum_y = std::max(maximum_y, point.y);
            }
        }
        else if (entity.type == SEntityType::Ellipse)
        {
            for (const SPoint2d& point :
                 ellipseApproximation(std::get<SEllipseEntity>(entity.geometry)))
            {
                minimum_x = std::min(minimum_x, point.x);
                minimum_y = std::min(minimum_y, point.y);
                maximum_x = std::max(maximum_x, point.x);
                maximum_y = std::max(maximum_y, point.y);
            }
        }
        else if (entity.type == SEntityType::MText)
        {
            const auto& text = std::get<SMTextEntity>(entity.geometry);
            minimum_x = std::min(minimum_x, text.position.x);
            minimum_y = std::min(minimum_y, text.position.y - (text.height * 3.0));
            maximum_x = std::max(maximum_x, text.position.x + text.width);
            maximum_y = std::max(maximum_y, text.position.y + text.height);
        }
        else if (entity.type == SEntityType::Leader)
        {
            for (const SPoint2d& point : std::get<SLeaderEntity>(entity.geometry).vertices)
            {
                minimum_x = std::min(minimum_x, point.x);
                minimum_y = std::min(minimum_y, point.y);
                maximum_x = std::max(maximum_x, point.x);
                maximum_y = std::max(maximum_y, point.y);
            }
        }
    }

    if (minimum_x > maximum_x || minimum_y > maximum_y)
    {
        m_zoom = 1.0;
        m_pan_offset = {};
        update();
        return;
    }

    const double extent_width = std::max(1.0, maximum_x - minimum_x);
    const double extent_height = std::max(1.0, maximum_y - minimum_y);
    m_zoom = std::clamp(std::min((width() * 0.8) / extent_width, (height() * 0.8) / extent_height),
                        kMinimumZoom, kMaximumZoom);
    const SPoint2d center{(minimum_x + maximum_x) * 0.5, (minimum_y + maximum_y) * 0.5};
    m_pan_offset = QPointF(-center.x * m_zoom, center.y * m_zoom);
    update();
}

} // namespace smartGraphics
