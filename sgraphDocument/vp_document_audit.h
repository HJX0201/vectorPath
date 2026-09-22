#pragma once

#include "vp_entity.h"

#include <QString>
#include <vector>

namespace Vp
{

enum class VpAuditSeverity
{
    Information,
    Warning,
    Error
};

struct VpDocumentAuditIssue
{
    VpAuditSeverity severity = VpAuditSeverity::Warning;
    VpEntityId entity_id = 0;
    QString message;
    bool was_repaired = false;
};

struct VpDocumentAuditReport
{
    int scanned_entity_count = 0;
    int issue_count = 0;
    int repaired_entity_count = 0;
    int removed_entity_count = 0;
    std::vector<VpDocumentAuditIssue> issues;

    bool isClean() const noexcept
    {
        return issue_count == 0;
    }
};

} // namespace Vp
