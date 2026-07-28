#include "s_cad_main_window.h"

#include "s_cad_workspace_widget.h"

#include <DockManager.h>
#include <DockWidget.h>
#include <QSplitter>
#include <algorithm>
#include <numeric>

namespace smartGraphics
{
namespace
{

int splitterChildIndex(QSplitter* splitter, const QWidget* target)
{
    for (int child_index = 0; child_index < splitter->count(); ++child_index)
    {
        QWidget* child = splitter->widget(child_index);
        if (child == target || child->isAncestorOf(target))
        {
            return child_index;
        }
    }
    return -1;
}

QSplitter* sharedSplitter(QWidget* root, const QWidget* first, const QWidget* second)
{
    const QList<QSplitter*> splitters = root->findChildren<QSplitter*>();
    for (QSplitter* splitter : splitters)
    {
        const int first_index = splitterChildIndex(splitter, first);
        const int second_index = splitterChildIndex(splitter, second);
        if (first_index >= 0 && second_index >= 0 && first_index != second_index)
        {
            return splitter;
        }
    }
    return nullptr;
}

void setChildProportion(QSplitter* splitter, const QWidget* target, double proportion)
{
    if (!splitter || splitter->count() != 2)
    {
        return;
    }
    const int target_index = splitterChildIndex(splitter, target);
    if (target_index < 0)
    {
        return;
    }

    QList<int> sizes = splitter->sizes();
    int total_size = std::accumulate(sizes.cbegin(), sizes.cend(), 0);
    if (total_size <= 0)
    {
        total_size = splitter->orientation() == Qt::Horizontal ? splitter->width()
                                                               : splitter->height();
    }
    const int target_size = std::max(1, static_cast<int>(total_size * proportion));
    sizes[target_index] = target_size;
    sizes[1 - target_index] = std::max(1, total_size - target_size);
    splitter->setSizes(sizes);
}

} // namespace

void SCadMainWindow::applyDefaultWorkspaceProportions()
{
    if (!m_dock_manager || !m_layer_dock || !m_command_dock || !m_workspace)
    {
        return;
    }

    QSplitter* inspector_splitter =
        sharedSplitter(m_dock_manager, m_workspace, m_layer_dock);
    setChildProportion(inspector_splitter, m_layer_dock, 0.20);

    QSplitter* command_splitter =
        sharedSplitter(m_dock_manager, m_workspace, m_command_dock);
    setChildProportion(command_splitter, m_command_dock, 0.22);
    configureDockSplitters();
}

} // namespace smartGraphics
