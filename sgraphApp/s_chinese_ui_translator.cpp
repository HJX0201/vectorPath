#include "s_chinese_ui_translator.h"

#include <cstring>

namespace vectorPath
{
namespace
{

QString translateSystemCommand(const char* source_text)
{
    if (std::strcmp(source_text, "Restore") == 0 ||
        std::strcmp(source_text, "Restored") == 0)
    {
        return QStringLiteral("还原");
    }
    if (std::strcmp(source_text, "Restore(R)") == 0)
    {
        return QStringLiteral("还原(&R)");
    }
    if (std::strcmp(source_text, "Move(M)") == 0)
    {
        return QStringLiteral("移动(&M)");
    }
    if (std::strcmp(source_text, "Size(S)") == 0)
    {
        return QStringLiteral("大小(&S)");
    }
    if (std::strcmp(source_text, "Minimize(N)") == 0)
    {
        return QStringLiteral("最小化(&N)");
    }
    if (std::strcmp(source_text, "Maximize") == 0)
    {
        return QStringLiteral("最大化");
    }
    if (std::strcmp(source_text, "Maximize(X)") == 0)
    {
        return QStringLiteral("最大化(&X)");
    }
    if (std::strcmp(source_text, "Close(C)") == 0)
    {
        return QStringLiteral("关闭(&C)");
    }
    return {};
}

} // namespace

SChineseUiTranslator::SChineseUiTranslator(QObject* parent) : QTranslator(parent)
{
}

QString SChineseUiTranslator::translate(const char* context, const char* source_text,
                                        const char* disambiguation, int count) const
{
    Q_UNUSED(disambiguation)
    Q_UNUSED(count)
    if (!context || !source_text)
    {
        return {};
    }
    if (std::strcmp(context, "SARibbonSystemButtonBar") == 0 ||
        std::strcmp(context, "SARibbonTitleIconWidget") == 0)
    {
        return translateSystemCommand(source_text);
    }
    return {};
}

} // namespace vectorPath
