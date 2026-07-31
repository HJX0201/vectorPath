#pragma once

#include <QString>
#include <QStringList>

namespace vectorPath
{

QStringList commandCompletionEntries();
QStringList commandCompletionsForPrefix(const QString& prefix);

} // namespace vectorPath
