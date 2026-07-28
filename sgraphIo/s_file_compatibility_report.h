#pragma once

#include <QStringList>
#include <cstddef>

namespace smartGraphics
{

struct SFileCompatibilityReport
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

} // namespace smartGraphics
