#include "s_shortcut_manager.h"

#include <QAction>
#include <QSettings>
#include <algorithm>

namespace vectorPath
{
namespace
{

QString settingKey(const QString& command_id)
{
    return QStringLiteral("shortcuts/%1").arg(command_id);
}

} // namespace

QKeySequence SShortcutBinding::currentSequence() const
{
    for (const QPointer<QAction>& action : actions)
    {
        if (action)
        {
            return action->shortcut();
        }
    }
    return {};
}

void SShortcutManager::registerAction(QAction* action, const QString& command_id,
                                      const QKeySequence& default_sequence)
{
    if (!action || command_id.trimmed().isEmpty())
    {
        return;
    }
    const QString normalized_id = command_id.trimmed().toLower();
    int index = bindingIndex(normalized_id);
    if (index < 0)
    {
        SShortcutBinding binding;
        binding.command_id = normalized_id;
        binding.display_name = action->text().remove(QLatin1Char('&'));
        binding.default_sequence = default_sequence;
        m_bindings.push_back(binding);
        index = m_bindings.size() - 1;
    }

    SShortcutBinding& binding = m_bindings[index];
    if (binding.default_sequence.isEmpty() && !default_sequence.isEmpty())
    {
        binding.default_sequence = default_sequence;
    }
    if (!binding.actions.contains(action))
    {
        binding.actions.push_back(action);
    }
    action->setProperty("smartShortcutId", normalized_id);
    const QKeySequence active_sequence =
        storedSequence(normalized_id, binding.default_sequence);
    for (const QPointer<QAction>& registered_action : binding.actions)
    {
        if (!registered_action)
        {
            continue;
        }
        registered_action->setProperty(
            "smartDefaultShortcut",
            binding.default_sequence.toString(QKeySequence::PortableText));
        registered_action->setShortcutContext(Qt::WindowShortcut);
    }
    applySequence(binding, active_sequence);
}

QVector<SShortcutBinding> SShortcutManager::bindings() const
{
    QVector<SShortcutBinding> result = m_bindings;
    std::sort(result.begin(), result.end(),
              [](const SShortcutBinding& first, const SShortcutBinding& second)
              {
                  return first.display_name.localeAwareCompare(second.display_name) < 0;
              });
    return result;
}

const SShortcutBinding* SShortcutManager::binding(const QString& command_id) const
{
    const int index = bindingIndex(command_id.trimmed().toLower());
    return index < 0 ? nullptr : &m_bindings[index];
}

bool SShortcutManager::setShortcut(const QString& command_id, const QKeySequence& shortcut,
                                   QString* conflicting_command_id)
{
    const QString normalized_id = command_id.trimmed().toLower();
    const int target_index = bindingIndex(normalized_id);
    if (target_index < 0)
    {
        return false;
    }
    if (!shortcut.isEmpty())
    {
        for (int index = 0; index < m_bindings.size(); ++index)
        {
            if (index == target_index)
            {
                continue;
            }
            if (m_bindings[index].currentSequence() == shortcut)
            {
                if (conflicting_command_id)
                {
                    *conflicting_command_id = m_bindings[index].command_id;
                }
                return false;
            }
        }
    }

    QSettings settings;
    settings.setValue(settingKey(normalized_id),
                      shortcut.toString(QKeySequence::PortableText));
    settings.sync();
    applySequence(m_bindings[target_index], shortcut);
    return true;
}

bool SShortcutManager::clearShortcut(const QString& command_id)
{
    return setShortcut(command_id, {});
}

void SShortcutManager::restoreDefaults()
{
    QSettings settings;
    for (SShortcutBinding& binding : m_bindings)
    {
        settings.remove(settingKey(binding.command_id));
        applySequence(binding, binding.default_sequence);
    }
    settings.sync();
}

int SShortcutManager::bindingIndex(const QString& command_id) const
{
    for (int index = 0; index < m_bindings.size(); ++index)
    {
        if (m_bindings[index].command_id == command_id)
        {
            return index;
        }
    }
    return -1;
}

QKeySequence SShortcutManager::storedSequence(
    const QString& command_id, const QKeySequence& default_sequence) const
{
    QSettings settings;
    const QString key = settingKey(command_id);
    if (!settings.contains(key))
    {
        return default_sequence;
    }
    return QKeySequence::fromString(settings.value(key).toString(),
                                    QKeySequence::PortableText);
}

void SShortcutManager::applySequence(SShortcutBinding& binding,
                                     const QKeySequence& shortcut)
{
    bool has_shortcut_owner = false;
    for (const QPointer<QAction>& action : binding.actions)
    {
        if (action)
        {
            action->setShortcut(has_shortcut_owner ? QKeySequence{} : shortcut);
            has_shortcut_owner = true;
        }
    }
}

} // namespace vectorPath
