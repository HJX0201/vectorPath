#include "s_dwg_dxf_adapter.h"

#include "s_cad_document.h"
#include "s_dxf_hatch_io.h"
#include "s_layer_record.h"

#include <QFile>
#include <QSaveFile>
#include <QTextStream>
#include <cmath>
#include <vector>

namespace smartGraphics
{
namespace
{

void writePair(QTextStream& stream, int group_code, const QString& value)
{
    stream << group_code << '\n' << value << '\n';
}

void writePair(QTextStream& stream, int group_code, int value)
{
    writePair(stream, group_code, QString::number(value));
}

std::vector<SDxfPair> readPairs(const QString& file_path, QString& error_message)
{
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        error_message = QObject::tr("无法读取 LibreDWG 转换源 DXF：%1").arg(file.errorString());
        return {};
    }
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    std::vector<SDxfPair> pairs;
    while (!stream.atEnd())
    {
        const QString group_line = stream.readLine().trimmed();
        if (stream.atEnd() && group_line.isEmpty())
        {
            break;
        }
        if (stream.atEnd())
        {
            error_message = QObject::tr("LibreDWG 转换源 DXF 的组码缺少对应值。");
            return {};
        }
        bool is_valid = false;
        const int group_code = group_line.toInt(&is_valid);
        const QString value = stream.readLine().trimmed();
        if (!is_valid)
        {
            error_message = QObject::tr("LibreDWG 转换源 DXF 包含无效组码：%1").arg(group_line);
            return {};
        }
        pairs.push_back({group_code, value});
    }
    return pairs;
}

void writeTableStart(QTextStream& stream, const QString& name, const QString& handle, int count)
{
    writePair(stream, 0, QStringLiteral("TABLE"));
    writePair(stream, 2, name);
    writePair(stream, 5, handle);
    writePair(stream, 100, QStringLiteral("AcDbSymbolTable"));
    writePair(stream, 70, count);
}

void writeEmptyTable(QTextStream& stream, const QString& name, const QString& handle)
{
    writeTableStart(stream, name, handle, 0);
    writePair(stream, 0, QStringLiteral("ENDTAB"));
}

void writeHeader(QTextStream& stream)
{
    writePair(stream, 999, QStringLiteral("smartCad LibreDWG R2000 interchange"));
    writePair(stream, 0, QStringLiteral("SECTION"));
    writePair(stream, 2, QStringLiteral("HEADER"));
    writePair(stream, 9, QStringLiteral("$ACADVER"));
    writePair(stream, 1, QStringLiteral("AC1015"));
    writePair(stream, 9, QStringLiteral("$HANDSEED"));
    writePair(stream, 5, QStringLiteral("FFFFFF"));
    writePair(stream, 9, QStringLiteral("$INSUNITS"));
    writePair(stream, 70, 4);
    writePair(stream, 0, QStringLiteral("ENDSEC"));
}

void writeLineTypeTable(QTextStream& stream)
{
    writeTableStart(stream, QStringLiteral("LTYPE"), QStringLiteral("5"), 1);
    writePair(stream, 0, QStringLiteral("LTYPE"));
    writePair(stream, 5, QStringLiteral("14"));
    writePair(stream, 100, QStringLiteral("AcDbSymbolTableRecord"));
    writePair(stream, 100, QStringLiteral("AcDbLinetypeTableRecord"));
    writePair(stream, 2, QStringLiteral("CONTINUOUS"));
    writePair(stream, 70, 0);
    writePair(stream, 3, QStringLiteral("Solid line"));
    writePair(stream, 72, 65);
    writePair(stream, 73, 0);
    writePair(stream, 40, QStringLiteral("0.0"));
    writePair(stream, 0, QStringLiteral("ENDTAB"));
}

void writeLayerTable(QTextStream& stream, const SCadDocument& document)
{
    const std::vector<SLayerRecord>& layers = document.layers();
    writeTableStart(stream, QStringLiteral("LAYER"), QStringLiteral("2"),
                    static_cast<int>(layers.size()));
    int handle = 0x200;
    for (const SLayerRecord& layer : layers)
    {
        int flags = layer.is_frozen ? 1 : 0;
        flags |= layer.is_locked ? 4 : 0;
        const int rgb = (layer.color.red() << 16) | (layer.color.green() << 8) | layer.color.blue();
        writePair(stream, 0, QStringLiteral("LAYER"));
        writePair(stream, 5, QString::number(handle++, 16).toUpper());
        writePair(stream, 100, QStringLiteral("AcDbSymbolTableRecord"));
        writePair(stream, 100, QStringLiteral("AcDbLayerTableRecord"));
        writePair(stream, 2, layer.name);
        writePair(stream, 70, flags);
        writePair(stream, 62, layer.is_visible ? 7 : -7);
        writePair(stream, 420, rgb);
        writePair(stream, 6, QStringLiteral("CONTINUOUS"));
        writePair(stream, 370, static_cast<int>(std::round(layer.line_width_mm * 100.0)));
    }
    writePair(stream, 0, QStringLiteral("ENDTAB"));
}

void writeBlockRecordTable(QTextStream& stream)
{
    writeTableStart(stream, QStringLiteral("BLOCK_RECORD"), QStringLiteral("1"), 2);
    const auto write_record = [&stream](const QString& handle, const QString& name)
    {
        writePair(stream, 0, QStringLiteral("BLOCK_RECORD"));
        writePair(stream, 5, handle);
        writePair(stream, 100, QStringLiteral("AcDbSymbolTableRecord"));
        writePair(stream, 100, QStringLiteral("AcDbBlockTableRecord"));
        writePair(stream, 2, name);
        writePair(stream, 70, 0);
    };
    write_record(QStringLiteral("F0"), QStringLiteral("*MODEL_SPACE"));
    write_record(QStringLiteral("F1"), QStringLiteral("*PAPER_SPACE"));
    writePair(stream, 0, QStringLiteral("ENDTAB"));
}

void writeTables(QTextStream& stream, const SCadDocument& document)
{
    writePair(stream, 0, QStringLiteral("SECTION"));
    writePair(stream, 2, QStringLiteral("TABLES"));
    writeLineTypeTable(stream);
    writeLayerTable(stream, document);
    writeEmptyTable(stream, QStringLiteral("STYLE"), QStringLiteral("3"));
    writeEmptyTable(stream, QStringLiteral("VIEW"), QStringLiteral("4"));
    writeEmptyTable(stream, QStringLiteral("UCS"), QStringLiteral("6"));
    writeEmptyTable(stream, QStringLiteral("APPID"), QStringLiteral("7"));
    writeEmptyTable(stream, QStringLiteral("DIMSTYLE"), QStringLiteral("8"));
    writeBlockRecordTable(stream);
    writePair(stream, 0, QStringLiteral("ENDSEC"));
}

void writeBlock(QTextStream& stream, const QString& name, const QString& owner,
                const QString& begin_handle, const QString& end_handle, bool is_paper_space)
{
    writePair(stream, 0, QStringLiteral("BLOCK"));
    writePair(stream, 5, begin_handle);
    writePair(stream, 330, owner);
    writePair(stream, 100, QStringLiteral("AcDbEntity"));
    if (is_paper_space)
    {
        writePair(stream, 67, 1);
    }
    writePair(stream, 8, QStringLiteral("0"));
    writePair(stream, 100, QStringLiteral("AcDbBlockBegin"));
    writePair(stream, 2, name);
    writePair(stream, 70, 0);
    writePair(stream, 10, QStringLiteral("0.0"));
    writePair(stream, 20, QStringLiteral("0.0"));
    writePair(stream, 30, QStringLiteral("0.0"));
    writePair(stream, 3, name);
    writePair(stream, 1, QString());
    writePair(stream, 0, QStringLiteral("ENDBLK"));
    writePair(stream, 5, end_handle);
    writePair(stream, 330, owner);
    writePair(stream, 100, QStringLiteral("AcDbEntity"));
    if (is_paper_space)
    {
        writePair(stream, 67, 1);
    }
    writePair(stream, 8, QStringLiteral("0"));
    writePair(stream, 100, QStringLiteral("AcDbBlockEnd"));
}

QStringList entitySubclasses(const QString& entity_name)
{
    if (entity_name == QLatin1String("LINE"))
    {
        return {QStringLiteral("AcDbLine")};
    }
    if (entity_name == QLatin1String("CIRCLE"))
    {
        return {QStringLiteral("AcDbCircle")};
    }
    if (entity_name == QLatin1String("ARC"))
    {
        return {QStringLiteral("AcDbCircle"), QStringLiteral("AcDbArc")};
    }
    if (entity_name == QLatin1String("ELLIPSE"))
    {
        return {QStringLiteral("AcDbEllipse")};
    }
    if (entity_name == QLatin1String("LWPOLYLINE"))
    {
        return {QStringLiteral("AcDbPolyline")};
    }
    if (entity_name == QLatin1String("TEXT"))
    {
        return {QStringLiteral("AcDbText")};
    }
    if (entity_name == QLatin1String("MTEXT"))
    {
        return {QStringLiteral("AcDbMText")};
    }
    if (entity_name == QLatin1String("DIMENSION"))
    {
        return {QStringLiteral("AcDbDimension")};
    }
    if (entity_name == QLatin1String("HATCH"))
    {
        return {QStringLiteral("AcDbHatch")};
    }
    if (entity_name == QLatin1String("SPLINE"))
    {
        return {QStringLiteral("AcDbSpline")};
    }
    return {};
}

void writeEntity(QTextStream& stream, const QString& entity_name,
                 const std::vector<SDxfPair>& entity_pairs, quint64 handle,
                 const QString& owner_handle)
{
    writePair(stream, 0, entity_name);
    writePair(stream, 5, QString::number(handle, 16).toUpper());
    writePair(stream, 330, owner_handle);
    writePair(stream, 100, QStringLiteral("AcDbEntity"));
    for (const SDxfPair& pair : entity_pairs)
    {
        if ((pair.group_code == 8 || pair.group_code == 62 || pair.group_code == 370 ||
             pair.group_code == 420 || pair.group_code == 440) &&
            pair.group_code != 100)
        {
            writePair(stream, pair.group_code, pair.value);
        }
    }
    for (const QString& subclass : entitySubclasses(entity_name))
    {
        writePair(stream, 100, subclass);
    }
    for (const SDxfPair& pair : entity_pairs)
    {
        if (pair.group_code != 5 && pair.group_code != 8 && pair.group_code != 62 &&
            pair.group_code != 100 && pair.group_code != 330 && pair.group_code != 370 &&
            pair.group_code != 420 && pair.group_code != 440)
        {
            writePair(stream, pair.group_code, pair.value);
        }
    }
}

void writeRequiredBlocks(QTextStream& stream)
{
    writePair(stream, 0, QStringLiteral("SECTION"));
    writePair(stream, 2, QStringLiteral("BLOCKS"));
    writeBlock(stream, QStringLiteral("*MODEL_SPACE"), QStringLiteral("F0"), QStringLiteral("F2"),
               QStringLiteral("F3"), false);
    writeBlock(stream, QStringLiteral("*PAPER_SPACE"), QStringLiteral("F1"), QStringLiteral("F4"),
               QStringLiteral("F5"), true);
    writePair(stream, 0, QStringLiteral("ENDSEC"));
}

void writeEntities(QTextStream& stream, const std::vector<SDxfPair>& pairs, quint64& handle)
{
    writePair(stream, 0, QStringLiteral("SECTION"));
    writePair(stream, 2, QStringLiteral("ENTITIES"));
    bool is_entities_section = false;
    for (std::size_t index = 0; index < pairs.size();)
    {
        if (pairs[index].group_code == 0 && pairs[index].value == QLatin1String("SECTION") &&
            index + 1 < pairs.size() && pairs[index + 1].group_code == 2)
        {
            is_entities_section = pairs[index + 1].value == QLatin1String("ENTITIES");
            index += 2;
            continue;
        }
        if (pairs[index].group_code == 0 && pairs[index].value == QLatin1String("ENDSEC"))
        {
            is_entities_section = false;
            ++index;
            continue;
        }
        if (!is_entities_section || pairs[index].group_code != 0)
        {
            ++index;
            continue;
        }
        const QString entity_name = pairs[index++].value;
        std::vector<SDxfPair> entity_pairs;
        while (index < pairs.size() && pairs[index].group_code != 0)
        {
            entity_pairs.push_back(pairs[index++]);
        }
        writeEntity(stream, entity_name, entity_pairs, handle++, QStringLiteral("F0"));
    }
    writePair(stream, 0, QStringLiteral("ENDSEC"));
    writePair(stream, 0, QStringLiteral("EOF"));
}

} // namespace

SResult<void> prepareLibreDwgR2000Dxf(const QString& source_path, const QString& target_path,
                                      const SCadDocument& document)
{
    QString error_message;
    const std::vector<SDxfPair> pairs = readPairs(source_path, error_message);
    if (!error_message.isEmpty())
    {
        return SResult<void>::failure(error_message);
    }
    QSaveFile file(target_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return SResult<void>::failure(
            QObject::tr("无法写入 LibreDWG R2000 DXF：%1").arg(file.errorString()));
    }
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    stream.setGenerateByteOrderMark(false);
    writeHeader(stream);
    writeTables(stream, document);
    quint64 handle = 0x1000;
    writeRequiredBlocks(stream);
    writeEntities(stream, pairs, handle);
    stream.flush();
    if (stream.status() != QTextStream::Ok || !file.commit())
    {
        return SResult<void>::failure(
            QObject::tr("LibreDWG R2000 DXF 保存失败：%1").arg(file.errorString()));
    }
    return SResult<void>::success();
}

} // namespace smartGraphics
