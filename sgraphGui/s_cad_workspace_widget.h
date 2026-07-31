#pragma once

#include <QIcon>
#include <QWidget>

class QTabBar;
class QTabWidget;
class QToolBar;

namespace vectorPath
{

class SCadDocument;
class SCadViewport;

class SCadWorkspaceWidget final : public QWidget
{
    Q_OBJECT

  public:
    explicit SCadWorkspaceWidget(QWidget* parent = nullptr);

    void setDocument(SCadDocument* document);
    SCadViewport* viewport() const noexcept;
    QToolBar* quickOperationBar() const noexcept;
    void updateDocumentTitle();
    void applyUiScale(int scale_percent);

  private:
    QTabWidget* m_document_tabs = nullptr;
    SCadViewport* m_viewport = nullptr;
    SCadDocument* m_document = nullptr;
    QToolBar* m_quick_operation_bar = nullptr;
};

} // namespace vectorPath
