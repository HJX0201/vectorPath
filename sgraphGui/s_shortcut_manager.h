#pragma once

#include <QKeySequence>
#include <QPointer>
#include <QString>
#include <QVector>

class QAction;

namespace vectorPath
{

struct SShortcutBinding
{
    QString command_id;
    QString display_name;
    QKeySequence default_sequence;
    QVector<QPointer<QAction>> actions;

    QKeySequence currentSequence() const;
};

class SShortcutManager final
{
  public:
    void registerAction(QAction* action, const QString& command_id,
                        const QKeySequence& default_sequence = {});
    QVector<SShortcutBinding> bindings() const;
    const SShortcutBinding* binding(const QString& command_id) const;
    bool setShortcut(const QString& command_id, const QKeySequence& shortcut,
                     QString* conflicting_command_id = nullptr);
    bool clearShortcut(const QString& command_id);
    void restoreDefaults();

  private:
    int bindingIndex(const QString& command_id) const;
    QKeySequence storedSequence(const QString& command_id,
                                const QKeySequence& default_sequence) const;
    void applySequence(SShortcutBinding& binding, const QKeySequence& shortcut);

    QVector<SShortcutBinding> m_bindings;
};

} // namespace vectorPath
