#include "s_associative_array_geometry.h"
#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_cad_viewport_geometry.h"
#include "s_document_transaction.h"

#include <QPainter>
#include <algorithm>
#include <cmath>
#include <utility>

namespace vectorPath
{
namespace
{

struct SPathSegment
{
    SPoint2d start_point;
    SPoint2d end_point;
    double start_distance = 0.0;
    double length = 0.0;
};

std::vector<SPathSegment> pathSegments(const SEntityRecord& path, bool& is_closed,
                                       double& total_length)
{
    std::vector<SPoint2d> vertices;
    is_closed = false;
    if (path.type == SEntityType::Line)
    {
        const auto& line = std::get<SLineEntity>(path.geometry);
        vertices = {line.start_point, line.end_point};
    }
    else if (path.type == SEntityType::Polyline)
    {
        const auto& polyline = std::get<SPolylineEntity>(path.geometry);
        if (std::any_of(polyline.bulges.begin(), polyline.bulges.end(),
                        [](double bulge)
                        {
                            return std::abs(bulge) > 1.0e-12;
                        }))
        {
            return {};
        }
        vertices = polyline.vertices;
        is_closed = polyline.is_closed;
    }
    std::vector<SPathSegment> segments;
    total_length = 0.0;
    const auto add_segment = [&](const SPoint2d& start_point, const SPoint2d& end_point)
    {
        const double segment_length = distance(start_point, end_point);
        if (segment_length > 1.0e-9)
        {
            segments.push_back({start_point, end_point, total_length, segment_length});
            total_length += segment_length;
        }
    };
    for (std::size_t index = 1; index < vertices.size(); ++index)
    {
        add_segment(vertices[index - 1], vertices[index]);
    }
    if (is_closed && vertices.size() > 2)
    {
        add_segment(vertices.back(), vertices.front());
    }
    return segments;
}

SPoint2d pointOnPath(const std::vector<SPathSegment>& segments, double target_distance,
                     double& tangent_angle)
{
    const auto iterator =
        std::find_if(segments.begin(), segments.end(),
                     [target_distance](const SPathSegment& segment)
                     {
                         return target_distance <= segment.start_distance + segment.length + 1.0e-9;
                     });
    const SPathSegment& segment = iterator == segments.end() ? segments.back() : *iterator;
    const double parameter =
        std::clamp((target_distance - segment.start_distance) / segment.length, 0.0, 1.0);
    tangent_angle = entityAngleDegrees(segment.start_point, segment.end_point);
    return {segment.start_point.x + ((segment.end_point.x - segment.start_point.x) * parameter),
            segment.start_point.y + ((segment.end_point.y - segment.start_point.y) * parameter)};
}

} // namespace

std::vector<SEntityRecord> pathArrayEntities(const std::vector<SEntityRecord>& sources,
                                             const SEntityRecord& path,
                                             const SPoint2d& source_base_point, int item_count,
                                             bool align_to_path)
{
    std::vector<SEntityRecord> results;
    if (sources.empty() || item_count < 2 ||
        sources.size() * static_cast<std::size_t>(item_count) > 10000)
    {
        return results;
    }
    bool is_closed = false;
    double total_length = 0.0;
    const std::vector<SPathSegment> segments = pathSegments(path, is_closed, total_length);
    if (segments.empty() || total_length <= 1.0e-9)
    {
        return results;
    }
    const double spacing =
        total_length / static_cast<double>(is_closed ? item_count : item_count - 1);
    double reference_angle = 0.0;
    pointOnPath(segments, 0.0, reference_angle);
    results.reserve(sources.size() * static_cast<std::size_t>(item_count));
    for (int item = 0; item < item_count; ++item)
    {
        double tangent_angle = 0.0;
        const SPoint2d item_point = pointOnPath(segments, item * spacing, tangent_angle);
        for (const SEntityRecord& source : sources)
        {
            SEntityRecord result = align_to_path ? rotatedEntity(source, source_base_point,
                                                                 tangent_angle - reference_angle)
                                                 : source;
            result = translatedEntity(result, item_point.x - source_base_point.x,
                                      item_point.y - source_base_point.y);
            results.push_back(std::move(result));
        }
    }
    return results;
}

void SCadViewport::setPathArrayParameters(int item_count, bool align_to_path)
{
    if (item_count < 2 || item_count > 10000)
    {
        emit commandMessage(tr("ARRAYPATH 项目数必须为 2–10000。"));
        return;
    }
    m_array_path_item_count = item_count;
    m_array_path_align = align_to_path;
    emit commandMessage(tr("ARRAYPATH 已设为 %1 项，%2。选择源对象或指定基点：")
                            .arg(item_count)
                            .arg(align_to_path ? tr("沿路径对齐") : tr("保持方向")));
}

void SCadViewport::acceptPathArrayPoint(const SPoint2d& world_point)
{
    if (m_selected_entity_ids.empty())
    {
        selectAt(world_point);
        emit commandMessage(m_selected_entity_ids.empty() ? tr("ARRAYPATH 未选择源对象，请重试：")
                                                          : tr("ARRAYPATH 指定源基点："));
        return;
    }
    if (!m_first_point)
    {
        m_first_point = world_point;
        emit commandMessage(tr("ARRAYPATH 选择直线或多段线路径："));
        update();
        return;
    }
    const std::optional<SEntityId> path_id = entityAt(world_point);
    const SEntityRecord* path = path_id ? entityById(*path_id) : nullptr;
    if (!path || (path->type != SEntityType::Line && path->type != SEntityType::Polyline) ||
        std::find(m_selected_entity_ids.begin(), m_selected_entity_ids.end(), path->id) !=
            m_selected_entity_ids.end())
    {
        emit commandMessage(tr("ARRAYPATH 路径必须是未包含在源对象中的直线或多段线。"));
        return;
    }
    std::vector<SEntityRecord> sources;
    for (SEntityId entity_id : m_selected_entity_ids)
    {
        if (const SEntityRecord* source = entityById(entity_id))
        {
            sources.push_back(*source);
        }
    }
    if (sources.empty() || std::any_of(sources.begin(), sources.end(),
                                       [](const auto& source)
                                       {
                                           return source.associative_array.has_value();
                                       }))
    {
        emit commandMessage(tr("ARRAYPATH 不能嵌套关联阵列，请先使用 ARRAYEDIT 或分解。"));
        return;
    }
    std::vector<SEntityRecord> array =
        associativePathArrayEntities(sources, sources.front().id, *path, *m_first_point,
                                     m_array_path_item_count, m_array_path_align);
    if (array.empty())
    {
        emit commandMessage(tr("ARRAYPATH 路径无有效长度或阵列项目总数超过 10000。"));
        return;
    }
    auto transaction = m_document->beginTransaction(tr("路径阵列"));
    std::vector<SEntityId> result_ids;
    for (std::size_t index = 0; index < sources.size(); ++index)
    {
        transaction->replaceEntity(sources[index].id, std::move(array[index]));
        result_ids.push_back(sources[index].id);
    }
    for (std::size_t index = sources.size(); index < array.size(); ++index)
    {
        result_ids.push_back(transaction->addEntityCopy(std::move(array[index]), true));
    }
    transaction->commit();
    m_selected_entity_ids = std::move(result_ids);
    m_selected_entity_id = m_selected_entity_ids.front();
    m_first_point.reset();
    emitSelectionState();
    emit commandMessage(
        tr("ARRAYPATH 已创建包含 %1 个实体的关联阵列。").arg(m_selected_entity_ids.size()));
    update();
}

void SCadViewport::drawPathArrayPreview(QPainter& painter)
{
    if (!m_first_point || m_selected_entity_ids.empty())
    {
        return;
    }
    const std::optional<SEntityId> path_id = entityAt(m_cursor_world);
    const SEntityRecord* path = path_id ? entityById(*path_id) : nullptr;
    if (!path || (path->type != SEntityType::Line && path->type != SEntityType::Polyline) ||
        std::find(m_selected_entity_ids.begin(), m_selected_entity_ids.end(), path->id) !=
            m_selected_entity_ids.end())
    {
        return;
    }
    std::vector<SEntityRecord> sources;
    for (SEntityId entity_id : m_selected_entity_ids)
    {
        if (const SEntityRecord* source = entityById(entity_id))
        {
            sources.push_back(*source);
        }
    }
    const std::vector<SEntityRecord> previews = pathArrayEntities(
        sources, *path, *m_first_point, m_array_path_item_count, m_array_path_align);
    painter.setPen(QPen(QColor(105, 219, 142), 2.0, Qt::DashLine));
    drawEntityGeometry(painter, *path, QColor(105, 219, 142, 60));
    painter.setPen(QPen(QColor(92, 214, 255), 1.4, Qt::DashLine));
    for (const SEntityRecord& preview : previews)
    {
        drawEntityGeometry(painter, preview, QColor(92, 214, 255, 60));
    }
}

} // namespace vectorPath
