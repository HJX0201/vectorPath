#include "s_shortcut_dialog.h"

#include "s_dialog_service.h"
#include "s_shortcut_manager.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace vectorPath
{
namespace
{

constexpr int kCommandIdRole = Qt::UserRole + 1;

QString sequenceText(const QKeySequence& sequence)
{
    return sequence.isEmpty()
               ? SShortcutDialog::tr("未设置")
               : sequence.toString(QKeySequence::NativeText);
}

} // namespace

SShortcutDialog::SShortcutDialog(SShortcutManager& shortcut_manager, QWidget* parent)
    : QDialog(parent), m_shortcut_manager(shortcut_manager)
{
    setObjectName(QStringLiteral("smartShortcutDialog"));
    setWindowTitle(tr("快捷键设置"));
    resize(720, 560);
    setMinimumSize(620, 460);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    auto* description = new QLabel(
        tr("选择功能后录入新的组合键。重复快捷键会被阻止，设置将在下次启动时保留。"),
        this);
    description->setWordWrap(true);
    layout->addWidget(description);

    m_search_edit = new QLineEdit(this);
    m_search_edit->setObjectName(QStringLiteral("smartShortcutSearch"));
    m_search_edit->setPlaceholderText(tr("搜索功能或命令标识"));
    m_search_edit->setClearButtonEnabled(true);
    layout->addWidget(m_search_edit);

    m_tree = new QTreeWidget(this);
    m_tree->setObjectName(QStringLiteral("smartShortcutTree"));
    m_tree->setColumnCount(4);
    m_tree->setHeaderLabels({tr("功能"), tr("命令标识"), tr("当前快捷键"), tr("默认快捷键")});
    m_tree->setRootIsDecorated(false);
    m_tree->setAlternatingRowColors(true);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    layout->addWidget(m_tree, 1);

    auto* editor_layout = new QFormLayout();
    editor_layout->setContentsMargins(0, 0, 0, 0);
    m_sequence_edit = new QKeySequenceEdit(this);
    m_sequence_edit->setObjectName(QStringLiteral("smartShortcutSequenceEdit"));
    m_sequence_edit->setEnabled(false);
    editor_layout->addRow(tr("新快捷键："), m_sequence_edit);
    layout->addLayout(editor_layout);

    auto* button_box = new QDialogButtonBox(QDialogButtonBox::Close, this);
    auto* apply_button = button_box->addButton(tr("应用"), QDialogButtonBox::ApplyRole);
    auto* clear_button = button_box->addButton(tr("清除当前"), QDialogButtonBox::ResetRole);
    auto* defaults_button = button_box->addButton(tr("恢复全部默认"), QDialogButtonBox::ResetRole);
    apply_button->setObjectName(QStringLiteral("smartShortcutApply"));
    clear_button->setObjectName(QStringLiteral("smartShortcutClear"));
    defaults_button->setObjectName(QStringLiteral("smartShortcutDefaults"));
    layout->addWidget(button_box);

    populateTree();
    connect(m_tree, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem* current)
            {
                updateEditor(current);
            });
    connect(m_search_edit, &QLineEdit::textChanged, this,
            [this](const QString& search_text)
            {
                const QString normalized = search_text.trimmed();
                for (int index = 0; index < m_tree->topLevelItemCount(); ++index)
                {
                    QTreeWidgetItem* item = m_tree->topLevelItem(index);
                    const bool is_match =
                        normalized.isEmpty() ||
                        item->text(0).contains(normalized, Qt::CaseInsensitive) ||
                        item->text(1).contains(normalized, Qt::CaseInsensitive);
                    item->setHidden(!is_match);
                }
            });
    connect(apply_button, &QPushButton::clicked, this,
            &SShortcutDialog::applySelectedShortcut);
    connect(clear_button, &QPushButton::clicked, this,
            &SShortcutDialog::clearSelectedShortcut);
    connect(defaults_button, &QPushButton::clicked, this,
            &SShortcutDialog::restoreDefaultShortcuts);
    connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    updateEditor(m_tree->currentItem());
}

void SShortcutDialog::populateTree()
{
    m_tree->clear();
    for (const SShortcutBinding& binding : m_shortcut_manager.bindings())
    {
        auto* item = new QTreeWidgetItem(
            {binding.display_name, binding.command_id, sequenceText(binding.currentSequence()),
             sequenceText(binding.default_sequence)});
        item->setData(0, kCommandIdRole, binding.command_id);
        m_tree->addTopLevelItem(item);
    }
    if (m_tree->topLevelItemCount() > 0)
    {
        m_tree->setCurrentItem(m_tree->topLevelItem(0));
    }
}

void SShortcutDialog::refreshTreeValues()
{
    for (int index = 0; index < m_tree->topLevelItemCount(); ++index)
    {
        QTreeWidgetItem* item = m_tree->topLevelItem(index);
        const SShortcutBinding* binding =
            m_shortcut_manager.binding(item->data(0, kCommandIdRole).toString());
        if (binding)
        {
            item->setText(2, sequenceText(binding->currentSequence()));
            item->setText(3, sequenceText(binding->default_sequence));
        }
    }
}

void SShortcutDialog::updateEditor(QTreeWidgetItem* item)
{
    const SShortcutBinding* binding =
        item ? m_shortcut_manager.binding(item->data(0, kCommandIdRole).toString()) : nullptr;
    m_sequence_edit->setEnabled(binding != nullptr);
    m_sequence_edit->setKeySequence(binding ? binding->currentSequence() : QKeySequence{});
}

void SShortcutDialog::applySelectedShortcut()
{
    QTreeWidgetItem* item = m_tree->currentItem();
    if (!item)
    {
        return;
    }
    const QString command_id = item->data(0, kCommandIdRole).toString();
    QString conflicting_id;
    if (!m_shortcut_manager.setShortcut(command_id, m_sequence_edit->keySequence(),
                                        &conflicting_id))
    {
        const SShortcutBinding* conflict = m_shortcut_manager.binding(conflicting_id);
        SDialogService::warning(
            this, tr("快捷键冲突"),
            tr("该快捷键已分配给“%1”，请先修改或清除原快捷键。")
                .arg(conflict ? conflict->display_name : conflicting_id));
        return;
    }
    refreshTreeValues();
}

void SShortcutDialog::clearSelectedShortcut()
{
    QTreeWidgetItem* item = m_tree->currentItem();
    if (!item)
    {
        return;
    }
    m_shortcut_manager.clearShortcut(item->data(0, kCommandIdRole).toString());
    refreshTreeValues();
    updateEditor(item);
}

void SShortcutDialog::restoreDefaultShortcuts()
{
    if (SDialogService::question(this, tr("恢复默认快捷键"),
                                 tr("确定恢复全部默认快捷键吗？")) != QMessageBox::Yes)
    {
        return;
    }
    m_shortcut_manager.restoreDefaults();
    refreshTreeValues();
    updateEditor(m_tree->currentItem());
}

} // namespace vectorPath
