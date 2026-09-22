#include "vp_dxf_codec.h"

#include "vp_cad_document.h"
#include "vp_document_transaction.h"
#include "vp_dxf_entity_io.h"
#include "vp_dxf_pair.h"
#include "vp_dxf_table_io.h"
#include "vp_qt_text.h"

#include <QFile>
#include <QSaveFile>
#include <QSet>
#include <QTextStream>
#include <utility>
#include <vector>

namespace Vp
{
namespace
{

std::vector<VpDxfPair> entityPairs(const std::vector<VpDxfPair>& pairs, std::size_t& index)
{
    std::vector<VpDxfPair> entity_pairs;
    while (index < pairs.size() && pairs[index].group_code != 0)
    {
        entity_pairs.push_back(pairs[index]);
        ++index;
    }
    return entity_pairs;
}

void ensureLayer(VpCadDocument& document, const QString& layer_name)
{
    if (!document.layer(layer_name))
    {
        document.addLayer(layer_name);
    }
}

void ensureEntityStyles(VpCadDocument& document, const VpEntityRecord& entity)
{
    if (entity.type == VpEntityType::LinearDimension)
    {
        const QString& style_name = std::get<VpLinearDimensionEntity>(entity.geometry).style_name;
        if (!document.dimensionStyle(style_name))
        {
            VpDimensionStyleRecord style;
            style.name = style_name;
            document.addOrUpdateDimensionStyle(style);
        }
    }
    else if (entity.type == VpEntityType::Text || entity.type == VpEntityType::MText)
    {
        const QString style_name = entity.type == VpEntityType::Text
                                       ? std::get<VpTextEntity>(entity.geometry).style_name
                                       : std::get<VpMTextEntity>(entity.geometry).style_name;
        if (!document.textStyle(style_name))
        {
            VpTextStyleRecord style;
            style.name = style_name;
            document.addOrUpdateTextStyle(style);
        }
    }
}

void appendWarning(VpFileCompatibilityReport& report, const QString& warning)
{
    if (report.warnings.size() < 100)
    {
        report.warnings.append(warning);
    }
}

int layerFlags(const VpLayerRecord& layer)
{
    int flags = layer.is_frozen ? 1 : 0;
    if (layer.is_locked)
    {
        flags |= 4;
    }
    return flags;
}

void writeLayerTable(QTextStream& stream, const VpCadDocument& document)
{
    writeDxfPair(stream, 0, QStringLiteral("SECTION"));
    writeDxfPair(stream, 2, QStringLiteral("TABLES"));
    writeDxfPair(stream, 0, QStringLiteral("TABLE"));
    writeDxfPair(stream, 2, QStringLiteral("LAYER"));
    writeDxfPair(stream, 70, QString::number(document.layers().size()));
    for (const VpLayerRecord& layer : document.layers())
    {
        writeDxfPair(stream, 0, QStringLiteral("LAYER"));
        writeDxfPair(stream, 2, layer.name);
        writeDxfPair(stream, 70, QString::number(layerFlags(layer)));
        writeDxfPair(stream, 62, layer.is_visible ? QStringLiteral("7") : QStringLiteral("-7"));
        writeDxfPair(stream, 420,
                     QString::number((layer.color.red() << 16) | (layer.color.green() << 8) |
                                     layer.color.blue()));
        writeDxfPair(stream, 6, layer.line_type);
        writeDxfPair(stream, 290, layer.is_plottable ? QStringLiteral("1") : QStringLiteral("0"));
        writeDxfPair(stream, 370,
                     QString::number(static_cast<int>(layer.line_width_mm * 100.0 + 0.5)));
        writeDxfPair(stream, 440,
                     QString::number(0x02000000 |
                                     static_cast<int>(layer.transparency * 255.0 / 100.0 + 0.5)));
    }
    writeDxfPair(stream, 0, QStringLiteral("ENDTAB"));
    writeDxfPair(stream, 0, QStringLiteral("ENDSEC"));
}

} // namespace

QString VpDxfCodec::id() const
{
    return QStringLiteral("vectorPath.dxf.ascii");
}

QString VpDxfCodec::displayName() const
{
    return QStringLiteral("AutoCAD DXF ASCII R2018");
}

QStringList VpDxfCodec::extensions() const
{
    return {QStringLiteral("dxf")};
}

VpResult<VpFileCompatibilityReport> VpDxfCodec::read(const QString& file_path,
                                                     VpCadDocument& document) const
{
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return VpResult<VpFileCompatibilityReport>::failure(
            toCoreText(QObject::tr("无法读取 DXF：%1").arg(file.errorString())));
    }
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    QString parse_error;
    const std::vector<VpDxfPair> pairs = readDxfPairs(stream, parse_error);
    if (!parse_error.isEmpty())
    {
        return VpResult<VpFileCompatibilityReport>::failure(toCoreText(parse_error));
    }

    document.clear();
    VpFileCompatibilityReport report;
    applyDxfLayerTable(document, readDxfLayerTable(pairs), readDxfCurrentLayer(pairs));
    auto transaction = document.beginTransaction(QObject::tr("导入 DXF"));
    bool is_entities_section = false;
    for (std::size_t index = 0; index < pairs.size();)
    {
        const VpDxfPair& pair = pairs[index];
        if (pair.group_code == 0 && pair.value == QLatin1String("SECTION") &&
            index + 1 < pairs.size() && pairs[index + 1].group_code == 2)
        {
            is_entities_section = pairs[index + 1].value == QLatin1String("ENTITIES");
            index += 2;
            continue;
        }
        if (pair.group_code == 0 && pair.value == QLatin1String("ENDSEC"))
        {
            is_entities_section = false;
            ++index;
            continue;
        }
        if (!is_entities_section || pair.group_code != 0)
        {
            ++index;
            continue;
        }

        const QString entity_name = pair.value;
        ++index;
        const std::vector<VpDxfPair> entity_pairs = entityPairs(pairs, index);
        VpResult<VpEntityRecord> entity_result = readDxfEntity(entity_name, entity_pairs);
        if (!entity_result)
        {
            ++report.skipped_entity_count;
            appendWarning(report, toQtError(entity_result));
            continue;
        }
        VpEntityRecord entity = std::move(entity_result.value());
        ensureLayer(document, entity.layer_name);
        ensureEntityStyles(document, entity);
        const VpEntityId entity_id = transaction->addEntityCopy(std::move(entity));
        if (entity_id == 0)
        {
            ++report.skipped_entity_count;
            appendWarning(
                report, QObject::tr("%1 引用了缺失定义或包含无效数据，已跳过。").arg(entity_name));
            continue;
        }
        ++report.imported_entity_count;
    }
    transaction->commit();
    return VpResult<VpFileCompatibilityReport>::success(std::move(report));
}

VpResult<VpFileCompatibilityReport> VpDxfCodec::write(const QString& file_path,
                                                      const VpCadDocument& document) const
{
    QSaveFile file(file_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return VpResult<VpFileCompatibilityReport>::failure(
            toCoreText(QObject::tr("无法写入 DXF：%1").arg(file.errorString())));
    }
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    stream.setGenerateByteOrderMark(false);
    writeDxfPair(stream, 0, QStringLiteral("SECTION"));
    writeDxfPair(stream, 2, QStringLiteral("HEADER"));
    writeDxfPair(stream, 9, QStringLiteral("$ACADVER"));
    writeDxfPair(stream, 1, QStringLiteral("AC1032"));
    writeDxfPair(stream, 9, QStringLiteral("$INSUNITS"));
    writeDxfPair(stream, 70, QStringLiteral("4"));
    writeDxfPair(stream, 9, QStringLiteral("$CLAYER"));
    writeDxfPair(stream, 8, document.currentLayerName());
    writeDxfPair(stream, 0, QStringLiteral("ENDSEC"));
    writeLayerTable(stream, document);

    VpFileCompatibilityReport report;
    writeDxfPair(stream, 0, QStringLiteral("SECTION"));
    writeDxfPair(stream, 2, QStringLiteral("BLOCKS"));
    writeDxfPair(stream, 0, QStringLiteral("ENDSEC"));
    writeDxfPair(stream, 0, QStringLiteral("SECTION"));
    writeDxfPair(stream, 2, QStringLiteral("ENTITIES"));
    QSet<qulonglong> reported_array_ids;
    for (const VpEntityRecord& entity : document.entities())
    {
        if (entity.associative_array &&
            !reported_array_ids.contains(entity.associative_array->array_id))
        {
            reported_array_ids.insert(entity.associative_array->array_id);
            report.warnings.append(
                QObject::tr("关联阵列 %1 已输出为普通 DXF 实体；重新导入后关联参数不可编辑。")
                    .arg(entity.associative_array->array_id));
        }
        if (!writeDxfEntity(stream, entity, report))
        {
            ++report.skipped_entity_count;
            appendWarning(report, QObject::tr("跳过不支持的实体 ID %1。").arg(entity.id));
        }
    }
    writeDxfPair(stream, 0, QStringLiteral("ENDSEC"));
    writeDxfPair(stream, 0, QStringLiteral("EOF"));
    stream.flush();
    if (stream.status() != QTextStream::Ok || !file.commit())
    {
        return VpResult<VpFileCompatibilityReport>::failure(
            toCoreText(QObject::tr("DXF 保存失败：%1").arg(file.errorString())));
    }
    return VpResult<VpFileCompatibilityReport>::success(std::move(report));
}

} // namespace Vp
