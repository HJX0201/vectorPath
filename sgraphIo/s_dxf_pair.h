#pragma once

#include <QString>
#include <optional>
#include <vector>

class QTextStream;

namespace vectorPath
{

struct SDxfPair
{
    int group_code = 0;
    QString value;
};

void writeDxfPair(QTextStream& stream, int group_code, const QString& value);
void writeDxfPair(QTextStream& stream, int group_code, double value);
std::optional<double> findDxfDouble(const std::vector<SDxfPair>& pairs, int group_code);
std::optional<QString> findDxfString(const std::vector<SDxfPair>& pairs, int group_code);
std::vector<SDxfPair> readDxfPairs(QTextStream& stream, QString& error_message);

} // namespace vectorPath
