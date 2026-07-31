#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_document_transaction.h"
#include "s_hatch_geometry.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <algorithm>
#include <cmath>
#include <functional>

namespace smartCam
{
namespace
{

QPainterPath hatchPath(const SHatchEntity& hatch,
                       const std::function<QPointF(const SPoint2d&)>& transform)
{
    QPainterPath path;
    path.setFillRule(Qt::OddEvenFill);
    const auto add_loop = [&path, &transform](const std::vector<SPoint2d>& loop)
    {
        if (loop.size() < 3)
        {
            return;
        }
        path.moveTo(transform(loop.front()));
        for (std::size_t index = 1; index < loop.size(); ++index)
        {
            path.lineTo(transform(loop[index]));
        }
        path.closeSubpath();
    };
    add_loop(hatch.boundary);
    for (const std::vector<SPoint2d>& island : hatch.island_boundaries)
    {
        add_loop(island);
    }
    return path;
}

double patternSpacing(const SHatchEntity& hatch, double zoom)
{
    return std::clamp(hatch.pattern_scale * zoom * 8.0, 4.0, 160.0);
}

} // namespace

void SCadViewport::setHatchSettings(const SHatchEntity& hatch_settings)
{
    m_hatch_settings.fill_type = hatch_settings.fill_type;
    m_hatch_settings.pattern_name = hatch_settings.pattern_name.trimmed().toUpper();
    m_hatch_settings.pattern_scale = hatch_settings.pattern_scale;
    m_hatch_settings.pattern_angle = hatch_settings.pattern_angle;
    m_hatch_settings.gradient_start = hatch_settings.gradient_start;
    m_hatch_settings.gradient_end = hatch_settings.gradient_end;
    update();
}

SHatchEntity SCadViewport::hatchSettings() const
{
    return m_hatch_settings;
}

bool SCadViewport::editSelectedHatches(const SHatchEntity& hatch_settings)
{
    if (!m_document || m_selected_entity_ids.empty())
    {
        return false;
    }
    auto transaction = m_document->beginTransaction(tr("编辑填充"));
    int changed_count = 0;
    for (SEntityId entity_id : m_selected_entity_ids)
    {
        const SEntityRecord* source = entityById(entity_id);
        if (!source || source->type != SEntityType::Hatch)
        {
            continue;
        }
        SEntityRecord replacement = *source;
        auto& hatch = std::get<SHatchEntity>(replacement.geometry);
        hatch.fill_type = hatch_settings.fill_type;
        hatch.pattern_name = hatch_settings.pattern_name.trimmed().toUpper();
        hatch.pattern_scale = hatch_settings.pattern_scale;
        hatch.pattern_angle = hatch_settings.pattern_angle;
        hatch.gradient_start = hatch_settings.gradient_start;
        hatch.gradient_end = hatch_settings.gradient_end;
        if (transaction->replaceEntity(entity_id, std::move(replacement)))
        {
            ++changed_count;
        }
    }
    if (changed_count == 0)
    {
        return false;
    }
    transaction->commit();
    update();
    return true;
}

std::optional<SHatchEntity>
SCadViewport::hatchFromBoundaryEntity(const SEntityRecord& boundary) const
{
    if (!m_document || boundary.type != SEntityType::Polyline)
    {
        return std::nullopt;
    }
    const auto& polyline = std::get<SPolylineEntity>(boundary.geometry);
    if (!polyline.is_closed || polyline.vertices.size() < 3)
    {
        return std::nullopt;
    }
    SHatchEntity hatch = m_hatch_settings;
    hatch.boundary = polyline.vertices;
    hatch.island_boundaries.clear();
    hatch.associative_boundary_id = boundary.id;
    const double outer_area = hatchLoopArea(hatch.boundary);
    for (const SEntityRecord& candidate : m_document->entities())
    {
        if (candidate.id == boundary.id || candidate.type != SEntityType::Polyline)
        {
            continue;
        }
        const auto& candidate_polyline = std::get<SPolylineEntity>(candidate.geometry);
        if (candidate_polyline.is_closed && candidate_polyline.vertices.size() >= 3 &&
            hatchLoopArea(candidate_polyline.vertices) < outer_area &&
            pointInsidePolygon(candidate_polyline.vertices.front(), hatch.boundary))
        {
            hatch.island_boundaries.push_back(candidate_polyline.vertices);
        }
    }
    return isHatchValid(hatch) ? std::optional<SHatchEntity>(std::move(hatch)) : std::nullopt;
}

void SCadViewport::drawHatchEntity(QPainter& painter, const SHatchEntity& source_hatch,
                                   const QColor& fill_color)
{
    SHatchEntity hatch = source_hatch;
    if (m_document && hatch.associative_boundary_id != 0)
    {
        const SEntityRecord* boundary = entityById(hatch.associative_boundary_id);
        if (boundary && boundary->type == SEntityType::Polyline)
        {
            const auto& polyline = std::get<SPolylineEntity>(boundary->geometry);
            if (polyline.is_closed && polyline.vertices.size() >= 3)
            {
                hatch.boundary = polyline.vertices;
            }
        }
    }
    const QPainterPath path = hatchPath(hatch,
                                        [this](const SPoint2d& point)
                                        {
                                            return worldToScreen(point);
                                        });
    painter.save();
    if (hatch.fill_type == SHatchFillType::Solid)
    {
        painter.fillPath(path, fill_color);
    }
    else if (hatch.fill_type == SHatchFillType::Gradient)
    {
        const QRectF bounds = path.boundingRect();
        const double radians = hatch.pattern_angle * 3.14159265358979323846 / 180.0;
        const QPointF direction{std::cos(radians), -std::sin(radians)};
        const double extent = std::hypot(bounds.width(), bounds.height()) * 0.5;
        QLinearGradient gradient(bounds.center() - (direction * extent),
                                 bounds.center() + (direction * extent));
        gradient.setColorAt(0.0, hatch.gradient_start);
        gradient.setColorAt(1.0, hatch.gradient_end);
        painter.fillPath(path, gradient);
    }
    else
    {
        painter.save();
        painter.setClipPath(path);
        const QRectF bounds = path.boundingRect();
        const QPointF center = bounds.center();
        const double extent = std::hypot(bounds.width(), bounds.height()) + 80.0;
        const double spacing = patternSpacing(hatch, m_zoom);
        painter.setPen(QPen(fill_color, 1.0));
        painter.translate(center);
        painter.rotate(-hatch.pattern_angle);
        if (hatch.pattern_name == QLatin1String("DOTS"))
        {
            for (double x = -extent; x <= extent; x += spacing)
            {
                for (double y = -extent; y <= extent; y += spacing)
                {
                    painter.drawEllipse(QPointF(x, y), 1.2, 1.2);
                }
            }
        }
        else
        {
            for (double x = -extent; x <= extent; x += spacing)
            {
                painter.drawLine(QPointF(x, -extent), QPointF(x, extent));
            }
            if (hatch.pattern_name == QLatin1String("CROSS") ||
                hatch.pattern_name == QLatin1String("ANSI37"))
            {
                for (double y = -extent; y <= extent; y += spacing)
                {
                    painter.drawLine(QPointF(-extent, y), QPointF(extent, y));
                }
            }
            else if (hatch.pattern_name == QLatin1String("ANSI32"))
            {
                painter.rotate(90.0);
                for (double x = -extent; x <= extent; x += spacing * 2.0)
                {
                    painter.drawLine(QPointF(x, -extent), QPointF(x, extent));
                }
            }
        }
        painter.restore();
    }
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);
    painter.restore();
}

} // namespace smartCam
