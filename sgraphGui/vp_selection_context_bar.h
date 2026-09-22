#pragma once

#include <QFrame>

class QAction;
class QHBoxLayout;

namespace Vp
{

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
