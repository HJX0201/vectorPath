#pragma once

#include <QString>
#include <optional>
#include <vector>

class QTextStream;

namespace Vp
{

// DXF 文件中的一组组码与值。
struct VpDxfPair
{
    int group_code = 0;
    QString value;
};

// 写出字符串型组码对。
void writeDxfPair(QTextStream& stream, int group_code, const QString& value);
// 写出数值型组码对。
void writeDxfPair(QTextStream& stream, int group_code, double value);
// 按组码查找第一个数值。
std::optional<double> findDxfDouble(const std::vector<VpDxfPair>& pairs, int group_code);
// 按组码查找第一个字符串。
std::optional<QString> findDxfString(const std::vector<VpDxfPair>& pairs, int group_code);
// 从流中读出全部组码对。
std::vector<VpDxfPair> readDxfPairs(QTextStream& stream, QString& error_message);

} // namespace Vp
