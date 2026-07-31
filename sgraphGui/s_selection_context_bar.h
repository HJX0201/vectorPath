#pragma once

#include <QFrame>

class QAction;
class QHBoxLayout;

namespace vectorPath
{

class SSelectionContextBar final : public QFrame
{
  public:
    explicit SSelectionContextBar(QWidget* parent = nullptr);

    void addAction(QAction* action);
    void addSeparator();
    void popupAt(const QPoint& global_position);

  private:
    QHBoxLayout* m_layout = nullptr;
};

} // namespace vectorPath
