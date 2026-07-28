#include "s_cad_document.h"
#include "s_cad_main_window.h"
#include "s_cad_viewport.h"
#include "s_cad_workspace_widget.h"
#include "s_command_line_widget.h"
#include "s_dialog_service.h"
#include "s_document_recovery_manager.h"

#include <QDateTime>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace smartGraphics
{

void SCadMainWindow::createRecoveryCopy()
{
    const SResult<QString> result = m_recovery_manager->autosaveNow();
    if (!result)
    {
        SDialogService::warning(this, tr("创建恢复副本"), result.errorMessage());
        return;
    }
    if (result.value().isEmpty())
    {
        m_command_line->appendMessage(tr("当前图形没有未保存更改，无需创建恢复副本。"));
    }
    else
    {
        m_command_line->appendMessage(tr("已创建恢复副本：%1").arg(result.value()));
    }
}

void SCadMainWindow::showRecoveryManager()
{
    SDialog dialog(this);
    dialog.setWindowTitle(tr("图形恢复管理器"));
    dialog.resize(760, 390);
    auto* layout = new QVBoxLayout(&dialog);
    auto* description = new QLabel(
        tr("以下副本由自动保存生成。恢复后图形保持未保存状态，确认内容后请正常保存。"), &dialog);
    description->setWordWrap(true);
    layout->addWidget(description);

    auto* tree = new QTreeWidget(&dialog);
    tree->setHeaderLabels({tr("图形"), tr("自动保存时间"), tr("原文件"), tr("恢复文件")});
    tree->setRootIsDecorated(false);
    tree->setAlternatingRowColors(true);
    tree->setSelectionMode(QAbstractItemView::SingleSelection);
    const QVector<SRecoveryEntry> entries = m_recovery_manager->availableRecoveries();
    for (int index = 0; index < entries.size(); ++index)
    {
        const SRecoveryEntry& entry = entries[index];
        auto* item = new QTreeWidgetItem(tree);
        item->setText(0, entry.display_name);
        item->setText(1, QDateTime::fromMSecsSinceEpoch(entry.created_at_milliseconds)
                             .toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
        item->setText(2, entry.original_file_path.isEmpty() ? tr("未命名图形")
                                                            : entry.original_file_path);
        item->setText(3, entry.recovery_file_path);
        item->setData(0, Qt::UserRole, index);
    }
    for (int column = 0; column < tree->columnCount(); ++column)
    {
        tree->resizeColumnToContents(column);
    }
    if (tree->topLevelItemCount() > 0)
    {
        tree->setCurrentItem(tree->topLevelItem(0));
    }
    layout->addWidget(tree, 1);

    auto* button_layout = new QHBoxLayout();
    auto* restore_button = new QPushButton(tr("恢复选中图形"), &dialog);
    auto* delete_button = new QPushButton(tr("删除副本"), &dialog);
    auto* close_buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    restore_button->setEnabled(!entries.isEmpty());
    delete_button->setEnabled(!entries.isEmpty());
    button_layout->addWidget(restore_button);
    button_layout->addWidget(delete_button);
    button_layout->addStretch();
    button_layout->addWidget(close_buttons);
    layout->addLayout(button_layout);

    connect(close_buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(delete_button, &QPushButton::clicked, &dialog,
            [this, tree, entries, delete_button, restore_button]()
            {
                QTreeWidgetItem* item = tree->currentItem();
                const int index = item ? item->data(0, Qt::UserRole).toInt() : -1;
                if (index < 0 || index >= entries.size())
                {
                    return;
                }
                if (!m_recovery_manager->discardRecovery(entries[index]))
                {
                    SDialogService::warning(this, tr("删除恢复副本"),
                                            tr("部分恢复文件无法删除。"));
                    return;
                }
                delete tree->takeTopLevelItem(tree->indexOfTopLevelItem(item));
                const bool has_items = tree->topLevelItemCount() > 0;
                delete_button->setEnabled(has_items);
                restore_button->setEnabled(has_items);
            });
    connect(restore_button, &QPushButton::clicked, &dialog,
            [this, &dialog, tree, entries]()
            {
                QTreeWidgetItem* item = tree->currentItem();
                const int index = item ? item->data(0, Qt::UserRole).toInt() : -1;
                if (index < 0 || index >= entries.size() || !maybeSave())
                {
                    return;
                }
                const SResult<void> result = m_recovery_manager->restoreRecovery(entries[index]);
                if (!result)
                {
                    SDialogService::critical(this, tr("恢复失败"), result.errorMessage());
                    return;
                }
                m_workspace->viewport()->cancelCommand();
                m_workspace->viewport()->zoomExtents();
                m_command_line->appendMessage(
                    tr("已恢复：%1。请检查后保存图形。").arg(entries[index].display_name));
                dialog.accept();
            });
    dialog.exec();
}

} // namespace smartGraphics
