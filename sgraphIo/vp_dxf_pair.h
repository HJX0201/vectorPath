#pragma once

#include <QString>
#include <optional>
#include <vector>

class QTextStream;

namespace Vp
{

struct VpDxfPair
{
    int group_code = 0;
    QString value;
};

void writeDxfPair(QTextStream& stream, int group_code, const QString& value);
void writeDxfPair(QTextStream& stream, int group_code, double value);
std::optional<double> findDxfDouble(const std::vector<VpDxfPair>& pairs, int group_code);
std::optional<QString> findDxfString(const std::vector<VpDxfPair>& pairs, int group_code);
std::vector<VpDxfPair> readDxfPairs(QTextStream& stream, QString& error_message);

} // namespace Vp
