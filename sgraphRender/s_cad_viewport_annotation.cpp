#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_document_transaction.h"

#include <QPainter>
#include <algorithm>
#include <utility>

namespace smartGraphics
{

void SCadViewport::setPendingMText(QString rich_text, double width, double height,
                                   STextHorizontalAlignment alignment)
{
    m_pending_mtext.rich_text = std::move(rich_text);
    m_pending_mtext.width = std::clamp(width, 0.01, 1.0e9);
    m_pending_mtext.height = std::clamp(height, 0.01, 1.0e9);
    m_pending_mtext.horizontal_alignment = alignment;
}

void SCadViewport::setPendingLeader(QString text, double text_height, double arrow_size)
{
    m_pending_leader_text = std::move(text);
    m_pending_leader_text_height = std::clamp(text_height, 0.01, 1.0e9);
    m_pending_leader_arrow_size = std::clamp(arrow_size, 0.01, 1.0e9);
}

void SCadViewport::acceptMTextPoint(const SPoint2d& world_point)
{
    if (!m_document || m_pending_mtext.rich_text.trimmed().isEmpty())
    {
        return;
    }
    auto transaction = m_document->beginTransaction(tr("创建多行文字"));
    transaction->addMText(world_point, m_pending_mtext.rich_text, m_pending_mtext.width,
                          m_pending_mtext.height, m_pending_mtext.rotation,
                          m_pending_mtext.horizontal_alignment);
    transaction->commit();
    emit commandMessage(tr("MTEXT 已创建。指定下一个插入点，Esc 结束："));
    update();
}

void SCadViewport::acceptLeaderPoint(const SPoint2d& world_point)
{
    if (!m_document || m_pending_leader_text.trimmed().isEmpty())
    {
        return;
    }
    m_input_points.push_back(world_point);
    if (m_input_points.size() < 3)
    {
        emit commandMessage(m_input_points.size() == 1 ? tr("MLEADER 指定引线折点：")
                                                       : tr("MLEADER 指定文字落点："));
        update();
        return;
    }
    auto transaction = m_document->beginTransaction(tr("创建多重引线"));
    transaction->addLeader(m_input_points, m_pending_leader_text, m_pending_leader_text_height,
                           m_pending_leader_arrow_size);
    transaction->commit();
    m_input_points.clear();
    emit commandMessage(tr("MLEADER 已创建。指定箭头位置："));
    update();
}

void SCadViewport::drawAnnotationPreview(QPainter& painter)
{
    if (m_tool_mode == SToolMode::MText && !m_pending_mtext.rich_text.trimmed().isEmpty())
    {
        SEntityRecord preview;
        preview.type = SEntityType::MText;
        m_pending_mtext.position = m_cursor_world;
        m_pending_mtext.style_name =
            m_document ? m_document->currentTextStyleName() : QStringLiteral("Standard");
        preview.geometry = m_pending_mtext;
        painter.setPen(QPen(QColor(92, 214, 255), 1.3, Qt::DashLine));
        drawEntityGeometry(painter, preview, QColor(92, 214, 255, 72));
        return;
    }
    if (m_tool_mode != SToolMode::Leader || m_pending_leader_text.trimmed().isEmpty())
    {
        return;
    }
    std::vector<SPoint2d> preview_points = m_input_points;
    preview_points.push_back(m_cursor_world);
    if (preview_points.size() < 2)
    {
        return;
    }
    SEntityRecord preview;
    preview.type = SEntityType::Leader;
    preview.geometry =
        SLeaderEntity{std::move(preview_points), m_pending_leader_text,
                      m_pending_leader_text_height, m_pending_leader_arrow_size,
                      m_document ? m_document->currentTextStyleName() : QStringLiteral("Standard")};
    painter.setPen(QPen(QColor(92, 214, 255), 1.5, Qt::DashLine));
    drawEntityGeometry(painter, preview, QColor(92, 214, 255, 72));
}

} // namespace smartGraphics
