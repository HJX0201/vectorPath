#pragma once

#include <QIcon>
#include <QWidget>

class QTabBar;
class QTabWidget;
class QToolBar;

namespace Vp
{

class VpCadDocument;
class VpCadViewport;

// 文档工作区：承载视口与快捷操作条。
class VpCadWorkspaceWidget final : public QWidget
{
    Q_OBJECT

  public:
    explicit VpCadWorkspaceWidget(QWidget* parent = nullptr);

    void setDocument(VpCadDocument* document);
    VpCadViewport* viewport() const noexcept;
    QToolBar* quickOperationBar() const noexcept;
    void updateDocumentTitle();
    void applyUiScale(int scale_percent);

  private:
    QTabWidget* m_document_tabs = nullptr;
    VpCadViewport* m_viewport = nullptr;
    VpCadDocument* m_document = nullptr;
    QToolBar* m_quick_operation_bar = nullptr;
};

} // namespace Vp
