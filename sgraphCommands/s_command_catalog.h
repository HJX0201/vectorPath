#pragma once

#include <QString>
#include <QStringList>

namespace smartCam
{

QStringList commandCompletionEntries();
QStringList commandCompletionsForPrefix(const QString& prefix);

} // namespace smartCam
