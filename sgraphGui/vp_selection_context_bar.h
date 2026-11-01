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
    // 构造浮动操作条。
    explicit VpSelectionContextBar(QWidget* parent = nullptr);

    // 添加一个操作按钮。
    void addAction(QAction* action);
    // 添加分隔线。
    void addSeparator();
    // 在指定屏幕位置弹出。
    void popupAt(const QPoint& global_position);

  private:
    QHBoxLayout* m_layout = nullptr;
};

} // namespace Vp
