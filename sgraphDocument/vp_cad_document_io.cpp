#include "vp_associative_array_io.h"
#include "vp_cad_document.h"
#include "vp_dimension_binary_io.h"
#include "vp_document_style_io.h"
#include "vp_entity_binary_io.h"
#include "vp_hatch_binary_io.h"
#include "vp_qt_text.h"

#include <QDataStream>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <algorithm>
#include <utility>

namespace Vp
{
namespace
{

constexpr char kFileMagic[] = "SMCAD001";
constexpr quint32 kFileVersion = 24;

void readPoint(QDataStream& stream, VpPoint2d& point)
{
    stream >> point.x >> point.y;
}

} // namespace

VpResult<void> VpCadDocument::save(const QString& file_path)
{
    return saveInternal(file_path, true);
}

VpResult<void> VpCadDocument::saveRecoveryCopy(const QString& file_path)
{
    return saveInternal(file_path, false);
}

VpResult<void> VpCadDocument::saveInternal(const QString& file_path, bool update_document_state)
{
    QSaveFile file(file_path);
    if (!file.open(QIODevice::WriteOnly))
    {
        return VpResult<void>::failure(toCoreText(tr("无法写入文件：%1").arg(file.errorString())));
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.setVersion(QDataStream::Qt_5_12);
    stream.writeRawData(kFileMagic, 8);

    QJsonObject manifest;
    manifest.insert(QStringLiteral("application"), QStringLiteral("smartCad"));
    manifest.insert(QStringLiteral("formatVersion"), static_cast<int>(kFileVersion));
    manifest.insert(QStringLiteral("unit"), insertionUnitKey(m_drawing_settings.insertion_unit));
    manifest.insert(QStringLiteral("angleFormat"), angleFormatKey(m_drawing_settings.angle_format));
    manifest.insert(QStringLiteral("linearPrecision"), m_drawing_settings.linear_precision);
    manifest.insert(QStringLiteral("angularPrecision"), m_drawing_settings.angular_precision);
    const QByteArray manifest_data = QJsonDocument(manifest).toJson(QJsonDocument::Compact);
    stream << static_cast<quint32>(manifest_data.size());
    stream.writeRawData(manifest_data.constData(), manifest_data.size());
    stream << static_cast<quint32>(m_layers.size());
    for (const VpLayerRecord& layer_record : m_layers)
    {
        stream << static_cast<quint64>(layer_record.id) << layer_record.name
               << static_cast<quint32>(layer_record.color.rgba()) << layer_record.line_width_mm
               << layer_record.is_visible << layer_record.is_locked << layer_record.is_plottable
               << layer_record.is_frozen << layer_record.line_type
               << static_cast<qint32>(layer_record.transparency);
    }
    stream << m_current_layer_name;
    stream << static_cast<quint32>(m_layer_states.size());
    for (const VpLayerStateRecord& state : m_layer_states)
    {
        stream << state.name << state.current_layer_name
               << static_cast<quint32>(state.layers.size());
        for (const VpLayerRecord& layer_record : state.layers)
        {
            stream << static_cast<quint64>(layer_record.id) << layer_record.name
                   << static_cast<quint32>(layer_record.color.rgba()) << layer_record.line_width_mm
                   << layer_record.is_visible << layer_record.is_locked << layer_record.is_plottable
                   << layer_record.is_frozen << layer_record.line_type
                   << static_cast<qint32>(layer_record.transparency);
        }
    }
    writeDocumentStyles(stream, m_text_styles, m_current_text_style_name, m_dimension_styles,
                        m_current_dimension_style_name);
    stream << static_cast<quint64>(m_entities.size());
    for (const VpEntityRecord& entity : m_entities)
    {
        writeEntityRecord(stream, entity);
    }
    stream << static_cast<quint32>(0);

    if (stream.status() != QDataStream::Ok || !file.commit())
    {
        return VpResult<void>::failure(toCoreText(tr("保存失败：%1").arg(file.errorString())));
    }

    if (update_document_state)
    {
        m_file_path = file_path;
        setModified(false);
        emit filePathChanged(m_file_path);
    }
    return VpResult<void>::success();
}

VpResult<void> VpCadDocument::recover(const QString& recovery_file_path,
                                      const QString& original_file_path)
{
    const VpResult<void> result = load(recovery_file_path);
    if (!result)
    {
        return result;
    }
    m_file_path = original_file_path;
    setModified(true);
    emit filePathChanged(m_file_path);
    return VpResult<void>::success();
}

VpResult<void> VpCadDocument::load(const QString& file_path)
{
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly))
    {
        return VpResult<void>::failure(toCoreText(tr("无法读取文件：%1").arg(file.errorString())));
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.setVersion(QDataStream::Qt_5_12);
    char magic[8]{};
    if (stream.readRawData(magic, 8) != 8 || QByteArray(magic, 8) != QByteArray(kFileMagic, 8))
    {
        return VpResult<void>::failure(toCoreText(tr("不是有效的 vectorPath 文件。")));
    }

    quint32 manifest_size = 0;
    stream >> manifest_size;
    if (manifest_size > 1024 * 1024)
    {
        return VpResult<void>::failure(toCoreText(tr("文件清单异常。")));
    }
    QByteArray manifest_data(static_cast<int>(manifest_size), Qt::Uninitialized);
    if (stream.readRawData(manifest_data.data(), manifest_data.size()) != manifest_data.size())
    {
        return VpResult<void>::failure(toCoreText(tr("文件清单不完整。")));
    }
    const QJsonDocument manifest = QJsonDocument::fromJson(manifest_data);
    const QJsonObject manifest_object = manifest.object();
    const int format_version = manifest_object.value(QStringLiteral("formatVersion")).toInt();
    if (format_version < 1 || format_version > static_cast<int>(kFileVersion))
    {
        return VpResult<void>::failure(toCoreText(tr("不支持此 vectorPath 文件版本。")));
    }

    VpDrawingSettings loaded_drawing_settings;
    if (format_version >= 12)
    {
        const std::optional<VpInsertionUnit> insertion_unit =
            insertionUnitFromKey(manifest_object.value(QStringLiteral("unit")).toString());
        const std::optional<VpAngleFormat> angle_format =
            angleFormatFromKey(manifest_object.value(QStringLiteral("angleFormat")).toString());
        loaded_drawing_settings.insertion_unit =
            insertion_unit.value_or(VpInsertionUnit::Millimeters);
        loaded_drawing_settings.angle_format = angle_format.value_or(VpAngleFormat::DecimalDegrees);
        loaded_drawing_settings.linear_precision =
            manifest_object.value(QStringLiteral("linearPrecision")).toInt(3);
        loaded_drawing_settings.angular_precision =
            manifest_object.value(QStringLiteral("angularPrecision")).toInt(2);
        if (!insertion_unit || !angle_format || !isValidDrawingSettings(loaded_drawing_settings))
        {
            return VpResult<void>::failure(toCoreText(tr("文件单位或角度设置无效。")));
        }
    }

    std::vector<VpLayerRecord> loaded_layers{
        {0, QStringLiteral("0"), QColor(220, 228, 238), 0.25, true, false, true},
        {1, QStringLiteral("DEFPOINTS"), QColor(128, 138, 148), 0.25, true, false, false},
    };
    QString loaded_current_layer = QStringLiteral("0");
    if (format_version >= 5)
    {
        quint32 layer_count = 0;
        stream >> layer_count;
        if (layer_count == 0 || layer_count > 100'000U)
        {
            return VpResult<void>::failure(toCoreText(tr("文件图层表无效。")));
        }
        loaded_layers.clear();
        loaded_layers.reserve(layer_count);
        for (quint32 index = 0; index < layer_count; ++index)
        {
            VpLayerRecord layer_record;
            quint32 rgba = 0;
            if (format_version >= 7)
            {
                quint64 layer_id = 0;
                stream >> layer_id;
                layer_record.id = layer_id;
            }
            else
            {
                layer_record.id = index;
            }
            stream >> layer_record.name >> rgba;
            if (format_version >= 6)
            {
                stream >> layer_record.line_width_mm;
                if (!std::isfinite(layer_record.line_width_mm) ||
                    layer_record.line_width_mm < 0.0 || layer_record.line_width_mm > 2.11)
                {
                    return VpResult<void>::failure(toCoreText(tr("文件图层线宽无效。")));
                }
            }
            stream >> layer_record.is_visible >> layer_record.is_locked >>
                layer_record.is_plottable;
            if (format_version >= 9)
            {
                stream >> layer_record.is_frozen;
            }
            if (format_version >= 11)
            {
                qint32 transparency = 0;
                stream >> layer_record.line_type >> transparency;
                layer_record.transparency = transparency;
                if (layer_record.line_type.trimmed().isEmpty() || transparency < 0 ||
                    transparency > 90)
                {
                    return VpResult<void>::failure(toCoreText(tr("文件图层线型或透明度无效。")));
                }
            }
            layer_record.color = QColor::fromRgba(rgba);
            if (layer_record.name.trimmed().isEmpty())
            {
                return VpResult<void>::failure(toCoreText(tr("文件图层表无效。")));
            }
            const bool is_duplicate = std::any_of(
                loaded_layers.begin(), loaded_layers.end(),
                [&layer_record](const VpLayerRecord& existing_layer)
                {
                    return existing_layer.id == layer_record.id ||
                           existing_layer.name.compare(layer_record.name, Qt::CaseInsensitive) == 0;
                });
            if (is_duplicate)
            {
                return VpResult<void>::failure(toCoreText(tr("文件图层表包含重复序号或名称。")));
            }
            loaded_layers.push_back(std::move(layer_record));
        }
        stream >> loaded_current_layer;
        const bool has_current_layer =
            std::any_of(loaded_layers.begin(), loaded_layers.end(),
                        [&loaded_current_layer](const VpLayerRecord& layer_record)
                        {
                            return layer_record.name == loaded_current_layer;
                        });
        if (!has_current_layer)
        {
            return VpResult<void>::failure(toCoreText(tr("文件当前图层无效。")));
        }
    }
    else if (format_version >= 4)
    {
        QStringList loaded_layer_names;
        stream >> loaded_layer_names >> loaded_current_layer;
        if (loaded_layer_names.isEmpty() || !loaded_layer_names.contains(loaded_current_layer))
        {
            return VpResult<void>::failure(toCoreText(tr("文件图层表无效。")));
        }
        loaded_layers.clear();
        for (const QString& layer_name : loaded_layer_names)
        {
            loaded_layers.push_back({static_cast<VpLayerId>(loaded_layers.size()), layer_name,
                                     layer_name == QStringLiteral("DEFPOINTS")
                                         ? QColor(128, 138, 148)
                                         : QColor(220, 228, 238),
                                     0.25, true, false, layer_name != QStringLiteral("DEFPOINTS")});
        }
    }

    std::vector<VpLayerStateRecord> loaded_layer_states;
    if (format_version >= 10)
    {
        quint32 state_count = 0;
        stream >> state_count;
        if (state_count > 10'000U)
        {
            return VpResult<void>::failure(toCoreText(tr("文件图层状态数量超过安全限制。")));
        }
        loaded_layer_states.reserve(state_count);
        for (quint32 state_index = 0; state_index < state_count; ++state_index)
        {
            VpLayerStateRecord state;
            quint32 snapshot_count = 0;
            stream >> state.name >> state.current_layer_name >> snapshot_count;
            if (state.name.trimmed().isEmpty() || snapshot_count > 100'000U)
            {
                return VpResult<void>::failure(toCoreText(tr("文件图层状态无效。")));
            }
            state.layers.reserve(snapshot_count);
            for (quint32 layer_index = 0; layer_index < snapshot_count; ++layer_index)
            {
                VpLayerRecord layer_record;
                quint64 layer_id = 0;
                quint32 rgba = 0;
                stream >> layer_id >> layer_record.name >> rgba >> layer_record.line_width_mm >>
                    layer_record.is_visible >> layer_record.is_locked >>
                    layer_record.is_plottable >> layer_record.is_frozen;
                if (format_version >= 11)
                {
                    qint32 transparency = 0;
                    stream >> layer_record.line_type >> transparency;
                    layer_record.transparency = transparency;
                }
                layer_record.id = layer_id;
                layer_record.color = QColor::fromRgba(rgba);
                if (layer_record.name.trimmed().isEmpty() ||
                    !std::isfinite(layer_record.line_width_mm) ||
                    layer_record.line_type.trimmed().isEmpty() || layer_record.transparency < 0 ||
                    layer_record.transparency > 90)
                {
                    return VpResult<void>::failure(toCoreText(tr("文件图层状态数据无效。")));
                }
                state.layers.push_back(std::move(layer_record));
            }
            loaded_layer_states.push_back(std::move(state));
        }
    }

    VpResult<VpLoadedDocumentStyles> loaded_styles = readDocumentStyles(stream, format_version);
    if (!loaded_styles)
    {
        return VpResult<void>::failure(toCoreText(toQtError(loaded_styles)));
    }
    std::vector<VpTextStyleRecord> loaded_text_styles =
        std::move(loaded_styles.value().text_styles);
    QString loaded_current_text_style = std::move(loaded_styles.value().current_text_style);
    std::vector<VpDimensionStyleRecord> loaded_dimension_styles =
        std::move(loaded_styles.value().dimension_styles);
    QString loaded_current_dimension_style =
        std::move(loaded_styles.value().current_dimension_style);

    quint64 entity_count = 0;
    stream >> entity_count;
    if (entity_count > 10'000'000ULL)
    {
        return VpResult<void>::failure(toCoreText(tr("实体数量超过安全限制。")));
    }

    std::vector<VpEntityRecord> loaded_entities;
    loaded_entities.reserve(static_cast<std::size_t>(entity_count));
    VpEntityId maximum_id = 0;
    for (quint64 index = 0; index < entity_count; ++index)
    {
        quint64 id = 0;
        quint8 type_value = 0;
        stream >> id >> type_value;
        if (format_version >= 21 && type_value == 12)
        {
            return VpResult<void>::failure(
                toCoreText(tr("此文件包含已移除的块引用，当前版本无法打开。")));
        }
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
                return VpResult<void>::failure(toCoreText(tr("文件实体线宽无效。")));
            }
        }
        if (format_version >= 22)
        {
            QString unused_space_name;
            stream >> unused_space_name;
        }
        if (entity.type == VpEntityType::Line)
        {
            VpLineEntity line;
            readPoint(stream, line.start_point);
            readPoint(stream, line.end_point);
            entity.geometry = line;
        }
        else if (entity.type == VpEntityType::Circle)
        {
            VpCircleEntity circle;
            readPoint(stream, circle.center);
            stream >> circle.radius;
            entity.geometry = circle;
        }
        else if (entity.type == VpEntityType::Arc && format_version >= 2)
        {
            VpArcEntity arc;
            readPoint(stream, arc.center);
            stream >> arc.radius >> arc.start_angle >> arc.end_angle;
            if (format_version >= 24)
            {
                stream >> arc.is_clockwise;
            }
            entity.geometry = arc;
        }
        else if (entity.type == VpEntityType::Polyline && format_version >= 2)
        {
            quint32 vertex_count = 0;
            bool is_closed = false;
            stream >> vertex_count >> is_closed;
            if (vertex_count > 1'000'000U)
            {
                return VpResult<void>::failure(toCoreText(tr("多段线顶点数量超过安全限制。")));
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
                    return VpResult<void>::failure(
                        toCoreText(tr("多段线 bulge 数量与顶点数量不一致。")));
                }
                polyline.bulges.resize(bulge_count);
                for (double& bulge : polyline.bulges)
                {
                    stream >> bulge;
                    if (!std::isfinite(bulge))
                    {
                        return VpResult<void>::failure(toCoreText(tr("多段线包含无效 bulge。")));
                    }
                }
            }
            if (format_version >= 14)
            {
                quint32 start_width_count = 0;
                quint32 end_width_count = 0;
                stream >> start_width_count;
                if ((start_width_count != 0U && start_width_count != vertex_count) ||
                    start_width_count > 1'000'000U)
                {
                    return VpResult<void>::failure(
                        toCoreText(tr("多段线起始宽度数量与顶点数量不一致。")));
                }
                polyline.start_widths.resize(start_width_count);
                for (double& width : polyline.start_widths)
                {
                    stream >> width;
                    if (!std::isfinite(width) || width < 0.0 || width > 1.0e9)
                    {
                        return VpResult<void>::failure(toCoreText(tr("多段线包含无效起始宽度。")));
                    }
                }
                stream >> end_width_count;
                if ((end_width_count != 0U && end_width_count != vertex_count) ||
                    end_width_count > 1'000'000U)
                {
                    return VpResult<void>::failure(
                        toCoreText(tr("多段线终止宽度数量与顶点数量不一致。")));
                }
                polyline.end_widths.resize(end_width_count);
                for (double& width : polyline.end_widths)
                {
                    stream >> width;
                    if (!std::isfinite(width) || width < 0.0 || width > 1.0e9)
                    {
                        return VpResult<void>::failure(toCoreText(tr("多段线包含无效终止宽度。")));
                    }
                }
            }
            entity.geometry = std::move(polyline);
        }
        else if (entity.type == VpEntityType::Text && format_version >= 3)
        {
            VpTextEntity text;
            readPoint(stream, text.position);
            stream >> text.text >> text.height >> text.rotation;
            if (format_version >= 17)
            {
                quint8 horizontal_alignment = 0;
                quint8 vertical_alignment = 0;
                stream >> text.style_name >> horizontal_alignment >> vertical_alignment;
                if (horizontal_alignment >
                        static_cast<quint8>(VpTextHorizontalAlignment::Justified) ||
                    vertical_alignment > static_cast<quint8>(VpTextVerticalAlignment::Bottom))
                {
                    return VpResult<void>::failure(toCoreText(tr("文件文字对齐方式无效。")));
                }
                text.horizontal_alignment =
                    static_cast<VpTextHorizontalAlignment>(horizontal_alignment);
                text.vertical_alignment = static_cast<VpTextVerticalAlignment>(vertical_alignment);
            }
            entity.geometry = std::move(text);
        }
        else if (entity.type == VpEntityType::LinearDimension && format_version >= 3)
        {
            VpResult<VpLinearDimensionEntity> dimension = readDimensionData(stream, format_version);
            if (!dimension)
            {
                return VpResult<void>::failure(toCoreText(toQtError(dimension)));
            }
            entity.geometry = std::move(dimension.value());
        }
        else if (entity.type == VpEntityType::Hatch && format_version >= 3)
        {
            VpResult<VpHatchEntity> hatch = readHatchData(stream, format_version);
            if (!hatch)
            {
                return VpResult<void>::failure(toCoreText(toQtError(hatch)));
            }
            entity.geometry = std::move(hatch.value());
        }
        else if (entity.type == VpEntityType::Spline && format_version >= 13)
        {
            VpSplineEntity spline;
            for (VpPoint2d& control_point : spline.control_points)
            {
                readPoint(stream, control_point);
            }
            entity.geometry = spline;
        }
        else if (entity.type == VpEntityType::Ellipse && format_version >= 15)
        {
            VpEllipseEntity ellipse;
            readPoint(stream, ellipse.center);
            readPoint(stream, ellipse.major_axis);
            readPoint(stream, ellipse.minor_axis);
            if (std::hypot(ellipse.major_axis.x, ellipse.major_axis.y) <= 1.0e-9 ||
                std::hypot(ellipse.minor_axis.x, ellipse.minor_axis.y) <= 1.0e-9)
            {
                return VpResult<void>::failure(toCoreText(tr("文件包含无效椭圆。")));
            }
            entity.geometry = ellipse;
        }
        else if (entity.type == VpEntityType::MText && format_version >= 17)
        {
            VpMTextEntity text;
            quint8 horizontal_alignment = 0;
            readPoint(stream, text.position);
            stream >> text.rich_text >> text.width >> text.height >> text.rotation >>
                text.style_name >> horizontal_alignment;
            if (text.rich_text.isEmpty() || !std::isfinite(text.width) || text.width <= 0.0 ||
                !std::isfinite(text.height) || text.height <= 0.0 ||
                !std::isfinite(text.rotation) ||
                horizontal_alignment > static_cast<quint8>(VpTextHorizontalAlignment::Justified))
            {
                return VpResult<void>::failure(toCoreText(tr("文件多行文字数据无效。")));
            }
            text.horizontal_alignment =
                static_cast<VpTextHorizontalAlignment>(horizontal_alignment);
            entity.geometry = std::move(text);
        }
        else if (entity.type == VpEntityType::Leader && format_version >= 17)
        {
            quint32 vertex_count = 0;
            stream >> vertex_count;
            if (vertex_count < 2 || vertex_count > 1'000'000U)
            {
                return VpResult<void>::failure(toCoreText(tr("文件引线顶点数量无效。")));
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
                return VpResult<void>::failure(toCoreText(tr("文件引线数据无效。")));
            }
            entity.geometry = std::move(leader);
        }
        else
        {
            return VpResult<void>::failure(toCoreText(tr("文件包含未知实体类型。")));
        }
        if (format_version >= 16 &&
            !readAssociativeArrayData(stream, entity.associative_array, format_version))
        {
            return VpResult<void>::failure(toCoreText(tr("文件包含无效关联阵列数据。")));
        }
        loaded_entities.push_back(entity);
        maximum_id = std::max(maximum_id, entity.id);
    }

    quint32 legacy_block_count = 0;
    if (format_version >= 21 && format_version < 23)
    {
        stream >> legacy_block_count;
        if (legacy_block_count != 0)
        {
            return VpResult<void>::failure(
                toCoreText(tr("此文件包含已移除的块数据，当前版本无法打开。")));
        }
    }
    quint32 unused_layout_count = 0;
    if (format_version >= 22)
    {
        stream >> unused_layout_count;
    }
    for (quint32 i = 0; i < unused_layout_count; ++i)
    {
        quint32 name_len = 0;
        stream >> name_len;
        for (quint32 j = 0; j < name_len; ++j)
        {
            quint16 unused_char = 0;
            stream >> unused_char;
        }
    }

    if (stream.status() != QDataStream::Ok)
    {
        return VpResult<void>::failure(toCoreText(tr("文件数据不完整。")));
    }

    const auto has_text_style = [&loaded_text_styles](const QString& style_name)
    {
        return std::any_of(loaded_text_styles.begin(), loaded_text_styles.end(),
                           [&style_name](const VpTextStyleRecord& style)
                           {
                               return style.name.compare(style_name, Qt::CaseInsensitive) == 0;
                           });
    };
    const auto has_dimension_style = [&loaded_dimension_styles](const QString& style_name)
    {
        return std::any_of(loaded_dimension_styles.begin(), loaded_dimension_styles.end(),
                           [&style_name](const VpDimensionStyleRecord& style)
                           {
                               return style.name.compare(style_name, Qt::CaseInsensitive) == 0;
                           });
    };
    const auto validate_entity = [&has_text_style,
                                  &has_dimension_style](const VpEntityRecord& entity) -> QString
    {
        QString text_style_name;
        if (entity.type == VpEntityType::Text)
        {
            text_style_name = std::get<VpTextEntity>(entity.geometry).style_name;
        }
        else if (entity.type == VpEntityType::MText)
        {
            text_style_name = std::get<VpMTextEntity>(entity.geometry).style_name;
        }
        else if (entity.type == VpEntityType::Leader)
        {
            text_style_name = std::get<VpLeaderEntity>(entity.geometry).style_name;
        }
        if (!text_style_name.isEmpty() && !has_text_style(text_style_name))
        {
            return QStringLiteral("实体引用了不存在的文字样式。");
        }
        if (entity.type == VpEntityType::LinearDimension &&
            !has_dimension_style(std::get<VpLinearDimensionEntity>(entity.geometry).style_name))
        {
            return QStringLiteral("标注引用了不存在的标注样式。");
        }
        return {};
    };
    for (const VpEntityRecord& entity : loaded_entities)
    {
        const QString validation_error = validate_entity(entity);
        if (!validation_error.isEmpty())
        {
            return VpResult<void>::failure(toCoreText(validation_error));
        }
    }
    m_entities = std::move(loaded_entities);
    m_next_entity_id = maximum_id + 1;
    m_layers = std::move(loaded_layers);
    m_layer_states = std::move(loaded_layer_states);
    const auto maximum_layer =
        std::max_element(m_layers.begin(), m_layers.end(),
                         [](const VpLayerRecord& left, const VpLayerRecord& right)
                         {
                             return left.id < right.id;
                         });
    m_next_layer_id = maximum_layer == m_layers.end() ? 0 : maximum_layer->id + 1;
    m_current_layer_name = std::move(loaded_current_layer);
    m_drawing_settings = loaded_drawing_settings;
    m_text_styles = std::move(loaded_text_styles);
    m_current_text_style_name = std::move(loaded_current_text_style);
    m_dimension_styles = std::move(loaded_dimension_styles);
    m_current_dimension_style_name = std::move(loaded_current_dimension_style);
    m_undo_stack.clear();
    m_redo_stack.clear();
    m_file_path = file_path;
    m_is_modified = false;
    emit filePathChanged(m_file_path);
    emit layersChanged();
    emit currentLayerChanged(m_current_layer_name);
    emit layerStatesChanged();
    emit drawingSettingsChanged();
    emit textStylesChanged();
    emit currentTextStyleChanged(m_current_text_style_name);
    emit dimensionStylesChanged();
    emit currentDimensionStyleChanged(m_current_dimension_style_name);
    emitDocumentState();
    return VpResult<void>::success();
}

} // namespace Vp
