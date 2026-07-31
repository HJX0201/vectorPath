#include "s_cad_document.h"
#include "s_cad_main_window.h"
#include "s_document_transaction.h"
#include "s_dialog_service.h"
#include "s_entity.h"
#include "s_icon_provider.h"
#include "s_layer_record.h"
#include "s_theme_manager.h"

#include <DockManager.h>
#include <DockWidget.h>
#include <QCheckBox>
#include <QColorDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>

namespace smartCam
{

namespace
{

QWidget* centeredCellWidget(QWidget* content, QWidget* parent)
{
    auto* container = new QWidget(parent);
    container->setAttribute(Qt::WA_TransparentForMouseEvents,
                            content->inherits("QFrame"));
    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(content, 0, Qt::AlignCenter);
    return container;
}

} // namespace

bool SCadMainWindow::clearLayerEntities(const QString& layer_name)
{
    if (!m_document->layer(layer_name))
    {
        return false;
    }

    auto transaction = m_document->beginTransaction(tr("删除图层实体"));
    bool has_entities = false;
    for (const SEntityRecord& entity : m_document->entities())
    {
        if (entity.layer_name.compare(layer_name, Qt::CaseInsensitive) == 0)
        {
            has_entities = transaction->removeEntity(entity.id) || has_entities;
        }
    }
    if (has_entities)
    {
        transaction->commit();
    }
    return true;
}

ads::CDockWidget* SCadMainWindow::createLayerDock()
{
    const SDesignToken& tokens = m_theme_manager.tokens();
    auto* layer_dock = new ads::CDockWidget(m_dock_manager, tr("图层管理器"), this);
    layer_dock->setObjectName(QStringLiteral("smartLayerDock"));
    layer_dock->setMinimumSizeHintMode(
        ads::CDockWidget::MinimumSizeHintFromContentMinimumSize);
    layer_dock->setIcon(
        SIconProvider::createIcon(SIconType::Layers, tokens.text_primary, tokens.accent));
    m_icon_docks.emplace_back(layer_dock, SIconType::Layers);
    m_layer_dock = layer_dock;
    m_dock_manager->addDockWidget(ads::RightDockWidgetArea, layer_dock);

    auto* layer_panel = new QWidget(layer_dock);
    layer_panel->setObjectName(QStringLiteral("smartLayerPanel"));
    layer_panel->setMinimumWidth(290);
    auto* layer_layout = new QVBoxLayout(layer_panel);
    layer_layout->setContentsMargins(5, 5, 5, 5);
    layer_layout->setSpacing(5);

    auto* layer_actions = new QHBoxLayout();
    auto* delete_layer_button = new QPushButton(tr("删除图层"), layer_panel);
    auto* clear_entities_button = new QPushButton(tr("删除图层实体"), layer_panel);
    auto* merge_color_button = new QPushButton(tr("合并同色"), layer_panel);
    delete_layer_button->setIcon(
        SIconProvider::createIcon(SIconType::Layers, tokens.text_primary, tokens.accent));
    clear_entities_button->setIcon(
        SIconProvider::createIcon(SIconType::Layers, tokens.text_primary, tokens.accent));
    merge_color_button->setIcon(
        SIconProvider::createIcon(SIconType::BooleanUnion, tokens.text_primary, tokens.accent));
    delete_layer_button->setIconSize(QSize(22, 22));
    clear_entities_button->setIconSize(QSize(22, 22));
    merge_color_button->setIconSize(QSize(22, 22));
    delete_layer_button->setToolTip(tr("删除选中的空图层"));
    clear_entities_button->setToolTip(tr("删除选中图层的全部实体，但保留图层"));
    merge_color_button->setToolTip(tr("把颜色相同的普通图层合并为一个图层"));
    layer_actions->addWidget(delete_layer_button);
    layer_actions->addWidget(clear_entities_button);
    layer_actions->addWidget(merge_color_button);
    layer_actions->addStretch();

    auto* layer_table = new QTableWidget(layer_panel);
    layer_table->setObjectName(QStringLiteral("smartLayerTable"));
    layer_table->setColumnCount(5);
    layer_table->setHorizontalHeaderLabels(
        {tr("序号"), tr("色号"), tr("颜色"), tr("数量"), tr("显示")});
    layer_table->verticalHeader()->setVisible(false);
    layer_table->setAlternatingRowColors(true);
    layer_table->setSelectionMode(QAbstractItemView::SingleSelection);
    layer_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    layer_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layer_table->setShowGrid(false);
    layer_table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    layer_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    layer_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    layer_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive);
    layer_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    layer_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    layer_table->setColumnWidth(2, 110);
    layer_layout->addWidget(layer_table);
    layer_layout->addLayout(layer_actions);
    layer_dock->setWidget(layer_panel);

    const auto refresh_layer_table = [this, layer_table]()
    {
        const int selected_row = layer_table->currentRow();
        const QTableWidgetItem* selected_item =
            selected_row >= 0 ? layer_table->item(selected_row, 0) : nullptr;
        const QString selected_name =
            selected_item ? selected_item->data(Qt::UserRole).toString() : QString();
        layer_table->clearContents();
        layer_table->setRowCount(static_cast<int>(m_document->layers().size()));
        int layer_index = 0;
        for (const SLayerRecord& layer_record : m_document->layers())
        {
            const int row = layer_index++;
            const int entity_count = static_cast<int>(
                std::count_if(m_document->entities().begin(), m_document->entities().end(),
                              [&layer_record](const SEntityRecord& entity)
                              {
                                  return entity.layer_name.compare(layer_record.name,
                                                                   Qt::CaseInsensitive) == 0;
                              }));
            const QString cell_texts[]{QString::number(row + 1), layer_record.color.name(),
                                       QString(), QString::number(entity_count), QString()};
            for (int column = 0; column < layer_table->columnCount(); ++column)
            {
                auto* item = new QTableWidgetItem(cell_texts[column]);
                item->setTextAlignment(Qt::AlignCenter);
                layer_table->setItem(row, column, item);
            }
            layer_table->item(row, 0)->setData(Qt::UserRole, layer_record.name);
            layer_table->item(row, 0)->setToolTip(tr("图层：%1").arg(layer_record.name));
            layer_table->item(row, 2)->setToolTip(tr("单击修改图层颜色"));
            auto* color_block = new QFrame(layer_table);
            color_block->setObjectName(QStringLiteral("smartLayerColorBlock"));
            color_block->setFixedSize(84, 18);
            color_block->setFrameShape(QFrame::Box);
            color_block->setStyleSheet(
                QStringLiteral("QFrame#smartLayerColorBlock { background-color:%1; "
                               "border:1px solid %2; }")
                    .arg(layer_record.color.name(), m_theme_manager.tokens().border.name()));
            layer_table->setCellWidget(
                row, 2, centeredCellWidget(color_block, layer_table));
            auto* visible_check = new QCheckBox(layer_table);
            visible_check->setObjectName(QStringLiteral("smartLayerVisibleCheck"));
            visible_check->setChecked(layer_record.is_visible);
            visible_check->setToolTip(tr("显示或隐藏图层“%1”").arg(layer_record.name));
            layer_table->setCellWidget(
                row, 4, centeredCellWidget(visible_check, layer_table));
            connect(visible_check, &QCheckBox::toggled, layer_table,
                    [this, layer_name = layer_record.name](bool is_visible)
                    {
                        QTimer::singleShot(0, this,
                                           [this, layer_name, is_visible]()
                                           {
                                               m_document->setLayerVisible(layer_name,
                                                                           is_visible);
                                           });
                    });
            if (layer_record.name == selected_name)
            {
                layer_table->selectRow(row);
            }
        }
    };
    refresh_layer_table();
    connect(m_document.get(), &SCadDocument::layersChanged, layer_table, refresh_layer_table);
    connect(m_document.get(), &SCadDocument::documentChanged, layer_table, refresh_layer_table);

    connect(delete_layer_button, &QPushButton::clicked, this,
            [this, layer_table]()
            {
                const int row = layer_table->currentRow();
                QTableWidgetItem* layer_item = row >= 0 ? layer_table->item(row, 0) : nullptr;
                if (!layer_item)
                {
                    return;
                }
                const QString layer_name = layer_item->data(Qt::UserRole).toString();
                if (!m_document->removeLayer(layer_name))
                {
                    SDialogService::warning(
                        this, tr("删除图层"),
                        tr("只能删除不含实体且未作为当前图层的普通图层；0 和 DEFPOINTS 不能删除。"));
                }
            });
    connect(clear_entities_button, &QPushButton::clicked, this,
            [this, layer_table]()
            {
                const int row = layer_table->currentRow();
                QTableWidgetItem* layer_item = row >= 0 ? layer_table->item(row, 0) : nullptr;
                if (!layer_item)
                {
                    return;
                }
                const QString layer_name = layer_item->data(Qt::UserRole).toString();
                const int entity_count = layer_table->item(row, 3)->text().toInt();
                if (entity_count == 0)
                {
                    return;
                }
                if (SDialogService::question(
                        this, tr("删除图层实体"),
                        tr("确定删除图层“%1”上的 %2 个实体吗？此操作可撤销。")
                            .arg(layer_name)
                            .arg(entity_count)) != QMessageBox::Yes)
                {
                    return;
                }
                clearLayerEntities(layer_name);
            });
    connect(merge_color_button, &QPushButton::clicked, this,
            [this]()
            {
                const int merged_count = m_document->mergeLayersByColor();
                if (merged_count == 0)
                {
                    SDialogService::information(this, tr("合并同色图层"),
                                                tr("没有可合并的同色普通图层。"));
                    return;
                }
                SDialogService::information(
                    this, tr("合并同色图层"),
                    tr("已合并 %1 个同色图层；此操作可以撤销。").arg(merged_count));
            });
    connect(layer_table, &QTableWidget::cellClicked, this,
            [this, layer_table](int row, int column)
            {
                const QTableWidgetItem* layer_item = layer_table->item(row, 0);
                if (!layer_item)
                {
                    return;
                }
                const QString layer_name = layer_item->data(Qt::UserRole).toString();
                const SLayerRecord* layer_record = m_document->layer(layer_name);
                if (!layer_record)
                {
                    return;
                }
                if (column == 2)
                {
                    QColorDialog color_dialog(layer_record->color, this);
                    color_dialog.setWindowTitle(tr("修改图层颜色"));
                    color_dialog.setOption(QColorDialog::DontUseNativeDialog);
                    color_dialog.setMinimumSize(720, 520);
                    const QColor color =
                        color_dialog.exec() == QColorDialog::Accepted
                            ? color_dialog.currentColor()
                            : QColor{};
                    if (color.isValid())
                    {
                        m_document->setLayerColor(layer_name, color);
                    }
                }
            });
    return layer_dock;
}

} // namespace smartCam
