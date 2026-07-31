#pragma once

#include "s_entity.h"

#include <QString>
#include <vector>

namespace vectorPath
{

enum class SAuditSeverity
{
    Information,
    Warning,
    Error
};

struct SDocumentAuditIssue
{
    SAuditSeverity severity = SAuditSeverity::Warning;
    SEntityId entity_id = 0;
    QString message;
    bool was_repaired = false;
};

struct SDocumentAuditReport
{
    int scanned_entity_count = 0;
    int issue_count = 0;
    int repaired_entity_count = 0;
    int removed_entity_count = 0;
    std::vector<SDocumentAuditIssue> issues;

    bool isClean() const noexcept
    {
        return issue_count == 0;
    }
};

} // namespace vectorPath
