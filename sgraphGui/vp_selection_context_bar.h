#pragma once

#include <QFrame>

class QAction;
class QHBoxLayout;

namespace Vp
{

// 选中对象后浮出的快捷操作条。
class VpSelectionContextBar final : public QFrame
{
  public:
    explicit VpSelectionContextBar(QWidget* parent = nullptr);

    void addAction(QAction* action);
    void addSeparator();
    void popupAt(const QPoint& global_position);

  private:
    QHBoxLayout* m_layout = nullptr;
};

} // namespace Vp
