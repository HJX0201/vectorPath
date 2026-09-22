#include "vp_cad_main_window.h"
#include "vp_shortcut_dialog.h"
#include "vp_shortcut_manager.h"

#include <QAction>

namespace Vp
{

void VpCadMainWindow::registerShortcutAction(QAction* action, const QString& command_id,
                                             const QKeySequence& default_sequence)
{
    if (m_shortcut_manager)
    {
        m_shortcut_manager->registerAction(action, command_id, default_sequence);
    }
}

void VpCadMainWindow::showShortcutSettings()
{
    if (!m_shortcut_manager)
    {
        return;
    }
    VpShortcutDialog dialog(*m_shortcut_manager, this);
    dialog.exec();
}

bool VpCadMainWindow::executeShortcutCommand(const QString& simplified_command)
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

} // namespace Vp
