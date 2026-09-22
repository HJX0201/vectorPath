#pragma once

#include <QKeySequence>
#include <QPointer>
#include <QString>
#include <QVector>

class QAction;

namespace Vp
{

struct VpShortcutBinding
{
    QString command_id;
    QString display_name;
    QKeySequence default_sequence;
    QVector<QPointer<QAction>> actions;

    QKeySequence currentSequence() const;
};

class VpShortcutManager final
{
  public:
    void registerAction(QAction* action, const QString& command_id,
                        const QKeySequence& default_sequence = {});
    QVector<VpShortcutBinding> bindings() const;
    const VpShortcutBinding* binding(const QString& command_id) const;
    bool setShortcut(const QString& command_id, const QKeySequence& shortcut,
                     QString* conflicting_command_id = nullptr);
    bool clearShortcut(const QString& command_id);
    void restoreDefaults();

  private:
    int bindingIndex(const QString& command_id) const;
    QKeySequence storedSequence(const QString& command_id,
                                const QKeySequence& default_sequence) const;
    void applySequence(VpShortcutBinding& binding, const QKeySequence& shortcut);

    QVector<VpShortcutBinding> m_bindings;
};

} // namespace Vp
