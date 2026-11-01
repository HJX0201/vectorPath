#pragma once

#include <QDialog>

class QKeySequenceEdit;
class QLineEdit;
class QTreeWidget;
class QTreeWidgetItem;

namespace Vp
{

class VpShortcutManager;

// 快捷键设置对话框：列出命令并编辑按键。
class VpShortcutDialog final : public QDialog
{
  public:
    // 构造快捷键对话框。
    explicit VpShortcutDialog(VpShortcutManager& shortcut_manager, QWidget* parent = nullptr);

  private:
    // 按命令分组填充快捷键树。
    void populateTree();
    // 刷新树中的按键显示。
    void refreshTreeValues();
    // 把选中项的按键填入编辑区。
    void updateEditor(QTreeWidgetItem* item);
    // 应用编辑区输入的按键。
    void applySelectedShortcut();
    // 清除选中命令的按键。
    void clearSelectedShortcut();
    // 恢复全部默认快捷键。
    void restoreDefaultShortcuts();

    VpShortcutManager& m_shortcut_manager;
    QLineEdit* m_search_edit = nullptr;
    QTreeWidget* m_tree = nullptr;
    QKeySequenceEdit* m_sequence_edit = nullptr;
};

} // namespace Vp
