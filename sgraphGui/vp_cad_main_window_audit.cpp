#include "vp_cad_document.h"
#include "vp_cad_main_window.h"
#include "vp_command_line_widget.h"
#include "vp_dialog_service.h"

#include <algorithm>

namespace Vp
{

void VpCadMainWindow::runDocumentAudit(bool repair)
{
    const VpDocumentAuditReport report = m_document->audit(repair);
    const QString summary =
        tr("已检查 %1 个实体，发现 %2 个问题，修复 %3 个实体，移除 %4 个无效实体。")
            .arg(report.scanned_entity_count)
            .arg(report.issue_count)
            .arg(report.repaired_entity_count)
            .arg(report.removed_entity_count);
    m_command_line->appendMessage(tr("AUDIT：%1").arg(summary));
    const int visible_issue_count = std::min<int>(static_cast<int>(report.issues.size()), 20);
    for (int index = 0; index < visible_issue_count; ++index)
    {
        const VpDocumentAuditIssue& issue = report.issues[static_cast<std::size_t>(index)];
        m_command_line->appendMessage(tr("  实体 %1：%2%3")
                                          .arg(issue.entity_id)
                                          .arg(issue.message)
                                          .arg(issue.was_repaired ? tr("（已修复）") : QString()));
    }
    if (report.issues.size() > static_cast<std::size_t>(visible_issue_count))
    {
        m_command_line->appendMessage(
            tr("  另有 %1 个问题，请保存后重新运行 AUDIT 检查。")
                .arg(report.issues.size() - static_cast<std::size_t>(visible_issue_count)));
    }
    if (report.isClean())
    {
        VpDialogService::information(this, tr("图形审计"),
                                     tr("未发现图形数据库问题。\n%1").arg(summary));
    }
    else
    {
        VpDialogService::warning(this, tr("图形审计"), summary);
    }
}

} // namespace Vp
