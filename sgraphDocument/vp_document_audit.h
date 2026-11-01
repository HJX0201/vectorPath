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

// 文档检查发现的一条问题记录。
struct VpDocumentAuditIssue
{
    VpAuditSeverity severity = VpAuditSeverity::Warning;
    VpEntityId entity_id = 0;
    QString message;
    bool was_repaired = false;
};

// 文档检查报告：统计问题数量与修复结果。
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
