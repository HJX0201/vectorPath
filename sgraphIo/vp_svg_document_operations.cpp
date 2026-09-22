#include "vp_svg_document_operations.h"

#include "vp_cad_document.h"
#include "vp_document_transaction.h"
#include "vp_hatch_geometry.h"
#include "vp_qt_text.h"

#include <QHash>
#include <algorithm>
#include <clipper2/clipper.h>
#include <exception>
#include <unordered_set>

namespace Vp
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

QColor entityColor(const VpCadDocument& document, const QString& layer_name)
{
    QColor color = document.layerColor(layer_name);
    const int transparency = document.layerTransparency(layer_name);
    color.setAlpha(std::clamp(255 - transparency * 255 / 100, 25, 255));
    return color;
}

VpVectorRegion hatchRegion(const VpHatchEntity& hatch, const QColor& color)
{
    VpVectorRegion region;
    region.color = color;
    region.fill_rule = VpVectorFillRule::EvenOdd;
    region.contours.push_back(hatch.boundary);
    region.contours.insert(region.contours.end(), hatch.island_boundaries.begin(),
                           hatch.island_boundaries.end());
    return region;
}

Clipper2Lib::PathsD pathsFromHatches(const std::vector<const VpEntityRecord*>& entities)
{
    Clipper2Lib::PathsD result;
    for (const VpEntityRecord* entity : entities)
    {
        const VpHatchEntity& hatch = std::get<VpHatchEntity>(entity->geometry);
        const auto append_loop = [&](const std::vector<VpPoint2d>& loop, bool is_hole)
        {
            Clipper2Lib::PathD path;
            path.reserve(loop.size());
            for (const VpPoint2d& point : loop)
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
        for (const std::vector<VpPoint2d>& island : hatch.island_boundaries)
        {
            append_loop(island, true);
        }
    }
    return result;
}

VpVectorRegion regionFromPaths(const Clipper2Lib::PathsD& paths)
{
    VpVectorRegion result;
    result.fill_rule = VpVectorFillRule::EvenOdd;
    for (const Clipper2Lib::PathD& path : paths)
    {
        std::vector<VpPoint2d> contour;
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

bool isTargetEntity(const VpEntityRecord& entity,
                    const std::unordered_set<VpEntityId>& target_entity_ids)
{
    return target_entity_ids.empty() ||
           target_entity_ids.find(entity.id) != target_entity_ids.end();
}

bool pointInsideHatch(const VpPoint2d& point, const VpHatchEntity& hatch)
{
    if (!pointInsidePolygon(point, hatch.boundary))
    {
        return false;
    }
    return std::none_of(hatch.island_boundaries.begin(), hatch.island_boundaries.end(),
                        [&point](const std::vector<VpPoint2d>& island)
                        {
                            return pointInsidePolygon(point, island);
                        });
}

VpPoint2d representativePoint(const VpPolylineEntity& polyline)
{
    if (polyline.vertices.empty() || polyline.is_closed)
    {
        return polyline.vertices.empty() ? VpPoint2d{} : polyline.vertices.front();
    }
    const VpPoint2d& first = polyline.vertices.front();
    const VpPoint2d& last = polyline.vertices.back();
    return {(first.x + last.x) * 0.5, (first.y + last.y) * 0.5};
}

bool isFillForSource(const VpEntityRecord& fill_entity, const VpEntityRecord& source_entity)
{
    if (fill_entity.type != VpEntityType::Polyline ||
        fill_entity.layer_name.compare(fillLayerName(source_entity.layer_name),
                                       Qt::CaseInsensitive) != 0)
    {
        return false;
    }
    const VpPolylineEntity& polyline = std::get<VpPolylineEntity>(fill_entity.geometry);
    return !polyline.vertices.empty() &&
           pointInsideHatch(representativePoint(polyline),
                            std::get<VpHatchEntity>(source_entity.geometry));
}

} // namespace

VpSvgVectorData svgColorBlockVectorData(const VpCadDocument& document,
                                        const std::vector<VpEntityId>& target_entity_ids)
{
    VpSvgVectorData result;
    const std::unordered_set<VpEntityId> targets(target_entity_ids.begin(),
                                                 target_entity_ids.end());
    bool has_point = false;
    double minimum_x = 0.0;
    double maximum_x = 0.0;
    double minimum_y = 0.0;
    double maximum_y = 0.0;
    for (const VpEntityRecord& entity : document.entities())
    {
        if (entity.type != VpEntityType::Hatch || !isSvgColorLayer(entity.layer_name) ||
            !isTargetEntity(entity, targets))
        {
            continue;
        }
        const VpHatchEntity& hatch = std::get<VpHatchEntity>(entity.geometry);
        result.regions.push_back(hatchRegion(hatch, entityColor(document, entity.layer_name)));
        for (const VpPoint2d& point : hatch.boundary)
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

VpResult<VpSvgFillReport> fillSvgColorBlocks(VpCadDocument& document,
                                             const VpVectorImportSettings& settings,
                                             const std::vector<VpEntityId>& target_entity_ids)
{
    if (settings.fill_mode == VpImportFillMode::None)
    {
        return VpResult<VpSvgFillReport>::failure(
            toCoreText(QStringLiteral("请选择一种 SVG 填充方式。")));
    }
    const std::unordered_set<VpEntityId> targets(target_entity_ids.begin(),
                                                 target_entity_ids.end());
    std::vector<const VpEntityRecord*> sources;
    for (const VpEntityRecord& entity : document.entities())
    {
        if (entity.type == VpEntityType::Hatch && isSvgColorLayer(entity.layer_name) &&
            isTargetEntity(entity, targets))
        {
            sources.push_back(&entity);
        }
    }
    if (sources.empty())
    {
        return VpResult<VpSvgFillReport>::failure(toCoreText(
            target_entity_ids.empty() ? QStringLiteral("当前图纸没有可填充的 SVG 色块。")
                                      : QStringLiteral("选中的实体不是可填充的 SVG 色块。")));
    }

    auto transaction = document.beginTransaction(QObject::tr("SVG 线填充"));
    VpSvgFillReport report;
    for (const VpEntityRecord& entity : document.entities())
    {
        const bool replace_fill = target_entity_ids.empty()
                                      ? isSvgFillLayer(entity.layer_name)
                                      : std::any_of(sources.begin(), sources.end(),
                                                    [&entity](const VpEntityRecord* source)
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
    for (const VpEntityRecord* source : sources)
    {
        ++report.source_region_count;
        const VpLayerRecord* source_layer = document.layer(source->layer_name);
        const QColor color = source_layer ? source_layer->color : QColor(Qt::black);
        const QString target_layer =
            transaction->ensureLayer(fillLayerName(source->layer_name), color, 0);
        const VpVectorRegion region = hatchRegion(std::get<VpHatchEntity>(source->geometry),
                                                  entityColor(document, source->layer_name));
        VpResult<std::vector<VpPolylineEntity>> polylines =
            settings.fill_mode == VpImportFillMode::SingleLine
                ? createSingleLineFill(region, settings.fill_spacing, settings.fill_angle_degrees)
                : createPolygonOffsetFill(region, settings.fill_spacing);
        if (!polylines)
        {
            return VpResult<VpSvgFillReport>::failure(toCoreText(toQtError(polylines)));
        }
        for (const VpPolylineEntity& polyline : polylines.value())
        {
            transaction->addEntity(VpEntityType::Polyline, polyline, target_layer);
            ++report.created_polyline_count;
        }
    }
    transaction->commit();
    return VpResult<VpSvgFillReport>::success(report);
}

VpResult<VpSvgDeduplicateReport> deduplicateSvgColorBlocks(VpCadDocument& document,
                                                           VpSvgLayerPriority priority)
{
    std::vector<QString> ordered_layers;
    QHash<QString, std::vector<const VpEntityRecord*>> layer_entities;
    for (const VpLayerRecord& layer : document.layers())
    {
        if (isSvgColorLayer(layer.name))
        {
            ordered_layers.push_back(layer.name);
        }
    }
    if (priority == VpSvgLayerPriority::LowerFirst)
    {
        std::reverse(ordered_layers.begin(), ordered_layers.end());
    }
    for (const VpEntityRecord& entity : document.entities())
    {
        if (entity.type == VpEntityType::Hatch && isSvgColorLayer(entity.layer_name))
        {
            layer_entities[entity.layer_name].push_back(&entity);
        }
    }
    if (layer_entities.isEmpty())
    {
        return VpResult<VpSvgDeduplicateReport>::failure(
            toCoreText(QStringLiteral("当前图纸没有可去重的 SVG 色块。")));
    }

    try
    {
        auto transaction = document.beginTransaction(QObject::tr("SVG 图层布尔去重"));
        VpSvgDeduplicateReport report;
        Clipper2Lib::PathsD occupied;
        for (const VpEntityRecord& entity : document.entities())
        {
            if ((entity.type == VpEntityType::Hatch && isSvgColorLayer(entity.layer_name)) ||
                isSvgFillLayer(entity.layer_name))
            {
                transaction->removeEntity(entity.id);
            }
        }
        for (const QString& layer_name : ordered_layers)
        {
            const std::vector<const VpEntityRecord*> sources = layer_entities.value(layer_name);
            if (sources.empty())
            {
                continue;
            }
            ++report.processed_layer_count;
            report.source_region_count += sources.size();
            const Clipper2Lib::PathsD source = Clipper2Lib::Union(
                pathsFromHatches(sources), Clipper2Lib::FillRule::NonZero, kClipperPrecision);
            const Clipper2Lib::PathsD visible =
                occupied.empty()
                    ? source
                    : Clipper2Lib::Difference(source, occupied, Clipper2Lib::FillRule::EvenOdd,
                                              kClipperPrecision);
            const VpResult<std::vector<VpHatchEntity>> hatches =
                createRegionHatches(regionFromPaths(visible));
            if (!hatches)
            {
                return VpResult<VpSvgDeduplicateReport>::failure(toCoreText(toQtError(hatches)));
            }
            for (const VpHatchEntity& hatch : hatches.value())
            {
                transaction->addEntity(VpEntityType::Hatch, hatch, layer_name);
                ++report.result_region_count;
            }
            Clipper2Lib::PathsD combined = occupied;
            combined.insert(combined.end(), source.begin(), source.end());
            occupied =
                Clipper2Lib::Union(combined, Clipper2Lib::FillRule::NonZero, kClipperPrecision);
        }
        transaction->commit();
        return VpResult<VpSvgDeduplicateReport>::success(report);
    }
    catch (const std::exception& exception)
    {
        return VpResult<VpSvgDeduplicateReport>::failure(
            toCoreText(QStringLiteral("SVG 图层布尔去重失败：%1")
                           .arg(QString::fromLocal8Bit(exception.what()))));
    }
}

} // namespace Vp
