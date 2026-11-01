#pragma once

#include <QString>
#include <QStringList>

namespace Vp
{

// 返回全部命令的补全候选。
QStringList commandCompletionEntries();
// 按前缀过滤命令补全候选。
QStringList commandCompletionsForPrefix(const QString& prefix);

} // namespace Vp
