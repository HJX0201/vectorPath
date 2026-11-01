#pragma once

#include <QStringList>
#include <cstddef>

namespace Vp
{

// 读写文件时的兼容性提示与实体统计。
struct VpFileCompatibilityReport
{
    QStringList warnings;
    std::size_t imported_entity_count = 0;
    std::size_t exported_entity_count = 0;
    std::size_t skipped_entity_count = 0;

    bool hasWarnings() const noexcept
    {
        return !warnings.isEmpty();
    }
};

} // namespace Vp
