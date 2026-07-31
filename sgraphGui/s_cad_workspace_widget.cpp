#include "s_cad_workspace_widget.h"

#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_dialog_service.h"

#include <QLineEdit>
#include <QTabBar>
#include <QTabWidget>
#include <QToolBar>
#include <QVBoxLayout>

namespace smartCam
{

SCadWorkspaceWidget::SCadWorkspaceWidget(QWidget* parent) : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_document_tabs = new QTabWidget(this);
    m_document_tabs->setObjectName(QStringLiteral("smartDocumentTabs"));
    m_document_tabs->setDocumentMode(true);
    m_document_tabs->setTabsClosable(false);
    m_document_tabs->setTabBarAutoHide(true);
    m_document_tabs->tabBar()->setObjectName(QStringLiteral("smartDocumentTabBar"));
    m_document_tabs->tabBar()->setExpanding(false);
    m_document_tabs->tabBar()->setElideMode(Qt::ElideRight);
    m_document_tabs->tabBar()->setMinimumHeight(27);
    m_document_tabs->tabBar()->setMaximumHeight(27);
    m_viewport = new SCadViewport(m_document_tabs);
    m_document_tabs->addTab(m_viewport, tr("未命名"));

    layout->addWidget(m_document_tabs, 1);
    m_quick_operation_bar = new QToolBar(tr("快速变换"), this);
    m_quick_operation_bar->setObjectName(QStringLiteral("smartQuickOperationBar"));
    m_quick_operation_bar->setMovable(false);
    m_quick_operation_bar->setFloatable(false);
    m_quick_operation_bar->setContextMenuPolicy(Qt::PreventContextMenu);
    m_quick_operation_bar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_quick_operation_bar->setIconSize(QSize(22, 22));
    m_quick_operation_bar->setFixedHeight(38);
    layout->addWidget(m_quick_operation_bar);
}

void SCadWorkspaceWidget::setDocument(SCadDocument* document)
{
    if (m_document)
    {
        disconnect(m_document, nullptr, this, nullptr);
    }
    m_document = document;
    m_viewport->setDocument(document);
    if (m_document)
    {
    }
    updateDocumentTitle();
}

void SCadWorkspaceWidget::applyUiScale(int scale_percent)
{
    const auto scaled = [scale_percent](int value)
    {
        return qRound(static_cast<double>(value) * scale_percent / 100.0);
    };
    m_document_tabs->tabBar()->setFixedHeight(scaled(27));
    m_quick_operation_bar->setIconSize(QSize(scaled(22), scaled(22)));
    m_quick_operation_bar->setFixedHeight(scaled(38));
}

SCadViewport* SCadWorkspaceWidget::viewport() const noexcept
{
    return m_viewport;
}

QToolBar* SCadWorkspaceWidget::quickOperationBar() const noexcept
{
    return m_quick_operation_bar;
}

void SCadWorkspaceWidget::updateDocumentTitle()
{
    if (!m_document)
    {
        return;
    }
    QString title = m_document->displayName();
    if (m_document->isModified())
    {
        title += QLatin1Char('*');
    }
    m_document_tabs->setTabText(0, title);
}

} // namespace smartCam
