#pragma once

#include <QDialog>

class QKeySequenceEdit;
class QLineEdit;
class QTreeWidget;
class QTreeWidgetItem;

namespace smartGraphics
{

class SShortcutManager;

class SShortcutDialog final : public QDialog
{
  public:
    explicit SShortcutDialog(SShortcutManager& shortcut_manager, QWidget* parent = nullptr);

  private:
    void populateTree();
    void refreshTreeValues();
    void updateEditor(QTreeWidgetItem* item);
    void applySelectedShortcut();
    void clearSelectedShortcut();
    void restoreDefaultShortcuts();

    SShortcutManager& m_shortcut_manager;
    QLineEdit* m_search_edit = nullptr;
    QTreeWidget* m_tree = nullptr;
    QKeySequenceEdit* m_sequence_edit = nullptr;
};

} // namespace smartGraphics
