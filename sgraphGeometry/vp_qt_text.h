#pragma once

#include "vp_result.h"

#include <QString>
#include <string>

namespace Vp
{

inline std::u16string toCoreText(const QString& text)
{
    return text.toStdU16String();
}

inline QString toQtText(const std::u16string& text)
{
    return QString::fromStdU16String(text);
}

// Only the desktop boundary interprets native codec/library diagnostic bytes.
template <typename TValue> QString toQtError(const VpResult<TValue>& result)
{
    return toQtText(result.errorMessage()) +
           QString::fromLocal8Bit(result.nativeErrorDetail().data(),
                                  static_cast<int>(result.nativeErrorDetail().size()));
}

} // namespace Vp
