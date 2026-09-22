#pragma once

#include <QString>
#include <QStringList>

namespace Vp
{

QStringList commandCompletionEntries();
QStringList commandCompletionsForPrefix(const QString& prefix);

} // namespace Vp
