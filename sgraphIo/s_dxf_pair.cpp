#include "s_dxf_pair.h"

#include <QTextStream>

namespace smartCam
{

void writeDxfPair(QTextStream& stream, int group_code, const QString& value)
{
    stream << group_code << "\n" << value << "\n";
}

void writeDxfPair(QTextStream& stream, int group_code, double value)
{
    stream << group_code << "\n" << QString::number(value, 'g', 16) << "\n";
}

std::optional<double> findDxfDouble(const std::vector<SDxfPair>& pairs, int group_code)
{
    for (const SDxfPair& pair : pairs)
    {
        if (pair.group_code != group_code)
        {
            continue;
        }
        bool is_valid = false;
        const double value = pair.value.toDouble(&is_valid);
        if (is_valid)
        {
            return value;
        }
    }
    return std::nullopt;
}

std::optional<QString> findDxfString(const std::vector<SDxfPair>& pairs, int group_code)
{
    for (const SDxfPair& pair : pairs)
    {
        if (pair.group_code == group_code)
        {
            return pair.value;
        }
    }
    return std::nullopt;
}

std::vector<SDxfPair> readDxfPairs(QTextStream& stream, QString& error_message)
{
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
            error_message = QObject::tr("DXF 组码缺少对应值。");
            return {};
        }
        const QString value_line = stream.readLine().trimmed();
        bool is_valid = false;
        const int group_code = group_line.toInt(&is_valid);
        if (!is_valid)
        {
            error_message = QObject::tr("DXF 包含无效组码：%1").arg(group_line);
            return {};
        }
        pairs.push_back({group_code, value_line});
    }
    return pairs;
}

} // namespace smartCam
