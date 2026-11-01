#pragma once

#include <QKeySequence>
#include <QPointer>
#include <QString>
#include <QVector>

class QAction;

namespace Vp
{

// 一条命令与按键序列的绑定关系。
struct VpShortcutBinding
{
    QString command_id;
    QString display_name;
    QKeySequence default_sequence;
    QVector<QPointer<QAction>> actions;

    // 返回该绑定当前生效的按键。
    QKeySequence currentSequence() const;
};

// 集中管理快捷键，检测冲突并持久化。
class VpShortcutManager final
{
  public:
    // 登记命令动作及其默认按键。
    void registerAction(QAction* action, const QString& command_id,
                        const QKeySequence& default_sequence = {});
    // 返回全部快捷键绑定。
    QVector<VpShortcutBinding> bindings() const;
    // 按命令标识查找绑定。
    const VpShortcutBinding* binding(const QString& command_id) const;
    // 设置快捷键，冲突时通过参数返回冲突命令。
    bool setShortcut(const QString& command_id, const QKeySequence& shortcut,
                     QString* conflicting_command_id = nullptr);
    // 清除命令的快捷键。
    bool clearShortcut(const QString& command_id);
    // 恢复全部默认快捷键。
    void restoreDefaults();

  private:
    // 返回命令绑定的下标。
    int bindingIndex(const QString& command_id) const;
    // 返回已保存的按键，缺省时用默认值。
    QKeySequence storedSequence(const QString& command_id,
                                const QKeySequence& default_sequence) const;
    // 把按键应用到动作并刷新界面。
    void applySequence(VpShortcutBinding& binding, const QKeySequence& shortcut);

    QVector<VpShortcutBinding> m_bindings;
};

} // namespace Vp
