#pragma once

#include <QString>
#include <QStringList>

namespace smartGraphics
{

QStringList commandCompletionEntries();
QStringList commandCompletionsForPrefix(const QString& prefix);

} // namespace smartGraphics
