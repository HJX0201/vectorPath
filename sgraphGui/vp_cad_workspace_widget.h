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
    // 构造工作区。
    explicit VpCadWorkspaceWidget(QWidget* parent = nullptr);

    // 绑定要显示的文档。
    void setDocument(VpCadDocument* document);
    // 返回内部视口。
    VpCadViewport* viewport() const noexcept;
    // 返回快捷操作工具条。
    QToolBar* quickOperationBar() const noexcept;
    // 按文档名刷新窗口标题。
    void updateDocumentTitle();
    // 应用界面缩放。
    void applyUiScale(int scale_percent);

  private:
    QTabWidget* m_document_tabs = nullptr;
    VpCadViewport* m_viewport = nullptr;
    VpCadDocument* m_document = nullptr;
    QToolBar* m_quick_operation_bar = nullptr;
};

} // namespace Vp
