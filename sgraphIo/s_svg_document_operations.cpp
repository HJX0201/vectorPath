#include "s_svg_document_operations.h"

#include "s_cad_document.h"
#include "s_document_transaction.h"
#include "s_hatch_geometry.h"

#include <clipper2/clipper.h>
#include <QHash>
#include <algorithm>
#include <exception>
#include <unordered_set>

namespace vectorPath
{
namespace
{

constexpr int kClipperPrecision = 6;

bool isSvgFillLayer(const QString& layer_name)
{
    return layer_name.startsWith(QStringLiteral("SVG_FILL_"), Qt::CaseInsensitive);
}

bool isSvgColorLayer(const QString& layer_name)
{
    return layer_name.startsWith(QStringLiteral("SVG_"), Qt::CaseInsensitive) &&
           !isSvgFillLayer(layer_name);
}

QColor entityColor(const SCadDocument& document, const QString& layer_name)
{
    QColor color = document.layerColor(layer_name);
    const int transparency = document.layerTransparency(layer_name);
    color.setAlpha(std::clamp(255 - transparency * 255 / 100, 25, 255));
    return color;
}

SVectorRegion hatchRegion(const SHatchEntity& hatch, const QColor& color)
{
    SVectorRegion region;
    region.color = color;
    region.fill_rule = SVectorFillRule::EvenOdd;
    region.contours.push_back(hatch.boundary);
    region.contours.insert(region.contours.end(), hatch.island_boundaries.begin(),
                           hatch.island_boundaries.end());
    return region;
}

Clipper2Lib::PathsD pathsFromHatches(const std::vector<const SEntityRecord*>& entities)
{
    Clipper2Lib::PathsD result;
    for (const SEntityRecord* entity : entities)
    {
        const SHatchEntity& hatch = std::get<SHatchEntity>(entity->geometry);
        const auto append_loop = [&](const std::vector<SPoint2d>& loop,
                                     bool is_hole)
        {
            Clipper2Lib::PathD path;
            path.reserve(loop.size());
            for (const SPoint2d& point : loop)
            {
                path.emplace_back(point.x, point.y);
            }
            if (path.size() >= 3)
            {
                if (Clipper2Lib::IsPositive(path) == is_hole)
                {
                    std::reverse(path.begin(), path.end());
                }
                result.push_back(std::move(path));
            }
        };
        append_loop(hatch.boundary, false);
        for (const std::vector<SPoint2d>& island : hatch.island_boundaries)
        {
            append_loop(island, true);
        }
    }
    return result;
}

SVectorRegion regionFromPaths(const Clipper2Lib::PathsD& paths)
{
    SVectorRegion result;
    result.fill_rule = SVectorFillRule::EvenOdd;
    for (const Clipper2Lib::PathD& path : paths)
    {
        std::vector<SPoint2d> contour;
        contour.reserve(path.size());
        for (const Clipper2Lib::PointD& point : path)
        {
            contour.push_back({point.x, point.y});
        }
        if (contour.size() >= 3)
        {
            result.contours.push_back(std::move(contour));
        }
    }
    return result;
}

QString fillLayerName(const QString& source_layer)
{
    return QStringLiteral("SVG_FILL_%1").arg(source_layer.mid(4));
}

bool isTargetEntity(
    const SEntityRecord& entity,
    const std::unordered_set<SEntityId>& target_entity_ids)
{
    return target_entity_ids.empty() ||
           target_entity_ids.find(entity.id) != target_entity_ids.end();
}

bool pointInsideHatch(const SPoint2d& point, const SHatchEntity& hatch)
{
    if (!pointInsidePolygon(point, hatch.boundary))
    {
        return false;
    }
    return std::none_of(
        hatch.island_boundaries.begin(), hatch.island_boundaries.end(),
        [&point](const std::vector<SPoint2d>& island)
        {
            return pointInsidePolygon(point, island);
        });
}

SPoint2d representativePoint(const SPolylineEntity& polyline)
{
    if (polyline.vertices.empty() || polyline.is_closed)
    {
        return polyline.vertices.empty() ? SPoint2d{} : polyline.vertices.front();
    }
    const SPoint2d& first = polyline.vertices.front();
    const SPoint2d& last = polyline.vertices.back();
    return {(first.x + last.x) * 0.5, (first.y + last.y) * 0.5};
}

bool isFillForSource(const SEntityRecord& fill_entity,
                     const SEntityRecord& source_entity)
{
    if (fill_entity.type != SEntityType::Polyline ||
        fill_entity.layer_name.compare(
            fillLayerName(source_entity.layer_name), Qt::CaseInsensitive) != 0)
    {
        return false;
    }
    const SPolylineEntity& polyline =
        std::get<SPolylineEntity>(fill_entity.geometry);
    return !polyline.vertices.empty() &&
           pointInsideHatch(
               representativePoint(polyline),
               std::get<SHatchEntity>(source_entity.geometry));
}

} // namespace

SSvgVectorData svgColorBlockVectorData(
    const SCadDocument& document,
    const std::vector<SEntityId>& target_entity_ids)
{
    SSvgVectorData result;
    const std::unordered_set<SEntityId> targets(
        target_entity_ids.begin(), target_entity_ids.end());
    bool has_point = false;
    double minimum_x = 0.0;
    double maximum_x = 0.0;
    double minimum_y = 0.0;
    double maximum_y = 0.0;
    for (const SEntityRecord& entity : document.entities())
    {
        if (entity.type != SEntityType::Hatch ||
            !isSvgColorLayer(entity.layer_name) ||
            !isTargetEntity(entity, targets))
        {
            continue;
        }
        const SHatchEntity& hatch = std::get<SHatchEntity>(entity.geometry);
        result.regions.push_back(
            hatchRegion(hatch, entityColor(document, entity.layer_name)));
        for (const SPoint2d& point : hatch.boundary)
        {
            minimum_x = has_point ? std::min(minimum_x, point.x) : point.x;
            maximum_x = has_point ? std::max(maximum_x, point.x) : point.x;
            minimum_y = has_point ? std::min(minimum_y, point.y) : point.y;
            maximum_y = has_point ? std::max(maximum_y, point.y) : point.y;
            has_point = true;
        }
    }
    result.source_width = has_point ? maximum_x - minimum_x : 0.0;
    result.source_height = has_point ? maximum_y - minimum_y : 0.0;
    return result;
}

SResult<SSvgFillReport> fillSvgColorBlocks(
    SCadDocument& document, const SVectorImportSettings& settings,
    const std::vector<SEntityId>& target_entity_ids)
{
    if (settings.fill_mode == SImportFillMode::None)
    {
        return SResult<SSvgFillReport>::failure(
            QStringLiteral("请选择一种 SVG 填充方式。"));
    }
    const std::unordered_set<SEntityId> targets(
        target_entity_ids.begin(), target_entity_ids.end());
    std::vector<const SEntityRecord*> sources;
    for (const SEntityRecord& entity : document.entities())
    {
        if (entity.type == SEntityType::Hatch &&
            isSvgColorLayer(entity.layer_name) &&
            isTargetEntity(entity, targets))
        {
            sources.push_back(&entity);
        }
    }
    if (sources.empty())
    {
        return SResult<SSvgFillReport>::failure(
            target_entity_ids.empty()
                ? QStringLiteral("当前图纸没有可填充的 SVG 色块。")
                : QStringLiteral("选中的实体不是可填充的 SVG 色块。"));
    }

    auto transaction = document.beginTransaction(QObject::tr("SVG 线填充"));
    SSvgFillReport report;
    for (const SEntityRecord& entity : document.entities())
    {
        const bool replace_fill =
            target_entity_ids.empty()
                ? isSvgFillLayer(entity.layer_name)
                : std::any_of(
                      sources.begin(), sources.end(),
                      [&entity](const SEntityRecord* source)
                      {
                          return isFillForSource(entity, *source);
                      });
        if (replace_fill)
        {
            if (transaction->removeEntity(entity.id))
            {
                ++report.replaced_entity_count;
            }
        }
    }
    for (const SEntityRecord* source : sources)
    {
        ++report.source_region_count;
        const SLayerRecord* source_layer = document.layer(source->layer_name);
        const QColor color = source_layer ? source_layer->color : QColor(Qt::black);
        const QString target_layer =
            transaction->ensureLayer(fillLayerName(source->layer_name), color, 0);
        const SVectorRegion region =
            hatchRegion(std::get<SHatchEntity>(source->geometry),
                        entityColor(document, source->layer_name));
        SResult<std::vector<SPolylineEntity>> polylines =
            settings.fill_mode == SImportFillMode::SingleLine
            ? createSingleLineFill(region, settings.fill_spacing,
                                   settings.fill_angle_degrees)
            : createPolygonOffsetFill(region, settings.fill_spacing);
        if (!polylines)
        {
            return SResult<SSvgFillReport>::failure(polylines.errorMessage());
        }
        for (const SPolylineEntity& polyline : polylines.value())
        {
            transaction->addEntity(SEntityType::Polyline, polyline, target_layer);
            ++report.created_polyline_count;
        }
    }
    transaction->commit();
    return SResult<SSvgFillReport>::success(report);
}

SResult<SSvgDeduplicateReport> deduplicateSvgColorBlocks(
    SCadDocument& document, SSvgLayerPriority priority)
{
    std::vector<QString> ordered_layers;
    QHash<QString, std::vector<const SEntityRecord*>> layer_entities;
    for (const SLayerRecord& layer : document.layers())
    {
        if (isSvgColorLayer(layer.name))
        {
            ordered_layers.push_back(layer.name);
        }
    }
    if (priority == SSvgLayerPriority::LowerFirst)
    {
        std::reverse(ordered_layers.begin(), ordered_layers.end());
    }
    for (const SEntityRecord& entity : document.entities())
    {
        if (entity.type == SEntityType::Hatch && isSvgColorLayer(entity.layer_name))
        {
            layer_entities[entity.layer_name].push_back(&entity);
        }
    }
    if (layer_entities.isEmpty())
    {
        return SResult<SSvgDeduplicateReport>::failure(
            QStringLiteral("当前图纸没有可去重的 SVG 色块。"));
    }

    try
    {
        auto transaction = document.beginTransaction(QObject::tr("SVG 图层布尔去重"));
        SSvgDeduplicateReport report;
        Clipper2Lib::PathsD occupied;
        for (const SEntityRecord& entity : document.entities())
        {
            if ((entity.type == SEntityType::Hatch &&
                 isSvgColorLayer(entity.layer_name)) ||
                isSvgFillLayer(entity.layer_name))
            {
                transaction->removeEntity(entity.id);
            }
        }
        for (const QString& layer_name : ordered_layers)
        {
            const std::vector<const SEntityRecord*> sources =
                layer_entities.value(layer_name);
            if (sources.empty())
            {
                continue;
            }
            ++report.processed_layer_count;
            report.source_region_count += sources.size();
            const Clipper2Lib::PathsD source =
                Clipper2Lib::Union(pathsFromHatches(sources),
                                   Clipper2Lib::FillRule::NonZero, kClipperPrecision);
            const Clipper2Lib::PathsD visible =
                occupied.empty()
                ? source
                : Clipper2Lib::Difference(source, occupied,
                                          Clipper2Lib::FillRule::EvenOdd,
                                          kClipperPrecision);
            const SResult<std::vector<SHatchEntity>> hatches =
                createRegionHatches(regionFromPaths(visible));
            if (!hatches)
            {
                return SResult<SSvgDeduplicateReport>::failure(
                    hatches.errorMessage());
            }
            for (const SHatchEntity& hatch : hatches.value())
            {
                transaction->addEntity(SEntityType::Hatch, hatch, layer_name);
                ++report.result_region_count;
            }
            Clipper2Lib::PathsD combined = occupied;
            combined.insert(combined.end(), source.begin(), source.end());
            occupied = Clipper2Lib::Union(combined, Clipper2Lib::FillRule::NonZero,
                                          kClipperPrecision);
        }
        transaction->commit();
        return SResult<SSvgDeduplicateReport>::success(report);
    }
    catch (const std::exception& exception)
    {
        return SResult<SSvgDeduplicateReport>::failure(
            QStringLiteral("SVG 图层布尔去重失败：%1")
                .arg(QString::fromLocal8Bit(exception.what())));
    }
}

} // namespace vectorPath
