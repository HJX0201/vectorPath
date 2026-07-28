#include "s_cad_main_window.h"

#include "s_shortcut_dialog.h"
#include "s_shortcut_manager.h"

#include <QAction>

namespace smartGraphics
{

void SCadMainWindow::registerShortcutAction(QAction* action, const QString& command_id,
                                            const QKeySequence& default_sequence)
{
    if (m_shortcut_manager)
    {
        m_shortcut_manager->registerAction(action, command_id, default_sequence);
    }
}

void SCadMainWindow::showShortcutSettings()
{
    if (!m_shortcut_manager)
    {
        return;
    }
    SShortcutDialog dialog(*m_shortcut_manager, this);
    dialog.exec();
}

bool SCadMainWindow::executeShortcutCommand(const QString& simplified_command)
{
    const QString normalized = simplified_command.trimmed().toUpper();
    if (normalized == QLatin1String("CUI") || normalized == QLatin1String("SHORTCUTS") ||
        normalized == QLatin1String("KEYBOARD"))
    {
        showShortcutSettings();
        return true;
    }
    return false;
}

} // namespace smartGraphics
