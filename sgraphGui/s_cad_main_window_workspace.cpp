#include "s_cad_document.h"
#include "s_cad_main_window.h"
#include "s_cad_property_tree.h"
#include "s_cad_property_ui.h"
#include "s_cad_viewport.h"
#include "s_cad_workspace_widget.h"
#include "s_command_line_widget.h"
#include "s_document_transaction.h"
#include "s_dialog_service.h"
#include "s_dxf_codec.h"
#include "s_geometry_types.h"
#include "s_icon_provider.h"
#include "s_layer_record.h"
#include "s_theme_manager.h"

#include <DockAreaWidget.h>
#include <DockManager.h>
#include <DockWidget.h>
#include <QAbstractButton>
#include <QAction>
#include <QCloseEvent>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QTextBrowser>
#include <QTextStream>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <SARibbonApplicationButton.h>
#include <SARibbonBar.h>
#include <SARibbonCategory.h>
#include <SARibbonPanel.h>
#include <SARibbonQuickAccessBar.h>
#include <algorithm>
#include <cmath>
#include <vector>

namespace smartCam
{
namespace
{

ads::CDockWidget* createListDock(ads::CDockManager* dock_manager, const QString& title,
                                 const QStringList& items, const QIcon& icon, QWidget* parent)
{
    auto* dock_widget = new ads::CDockWidget(dock_manager, title, parent);
    dock_widget->setIcon(icon);
    auto* list_widget = new QListWidget(dock_widget);
    list_widget->addItems(items);
    list_widget->setAlternatingRowColors(true);
    dock_widget->setWidget(list_widget);
    return dock_widget;
}

} // namespace

QToolBar* SCadMainWindow::createDrawingToolBar()
{
    auto* drawing_toolbar = new QToolBar(tr("绘图工具"), this);
    drawing_toolbar->setObjectName(QStringLiteral("smartDrawingToolBar"));
    drawing_toolbar->setMovable(false);
    drawing_toolbar->setFloatable(false);
    drawing_toolbar->setOrientation(Qt::Vertical);
    drawing_toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    drawing_toolbar->setIconSize(QSize(30, 30));
    drawing_toolbar->setFixedWidth(64);
    drawing_toolbar->setContextMenuPolicy(Qt::PreventContextMenu);

    drawing_toolbar->addAction(m_line_action);
    QAction* polyline_action = createAction(tr("多段线"), SIconType::DrawPolyline);
    registerShortcutAction(polyline_action, QStringLiteral("draw.polyline"));
    registerToolAction(polyline_action, SToolMode::Polyline);
    connect(polyline_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::Polyline);
            });
    drawing_toolbar->addAction(polyline_action);
    drawing_toolbar->addAction(m_circle_action);
    drawing_toolbar->addAction(m_arc_action);
    if (auto* circle_button =
            qobject_cast<QToolButton*>(drawing_toolbar->widgetForAction(m_circle_action)))
    {
        circle_button->setPopupMode(QToolButton::MenuButtonPopup);
    }
    if (auto* arc_button =
            qobject_cast<QToolButton*>(drawing_toolbar->widgetForAction(m_arc_action)))
    {
        arc_button->setPopupMode(QToolButton::MenuButtonPopup);
    }
    QAction* rectangle_action = createAction(tr("矩形"), SIconType::DrawRectangle);
    registerShortcutAction(rectangle_action, QStringLiteral("draw.rectangle"));
    registerToolAction(rectangle_action, SToolMode::Rectangle);
    connect(rectangle_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::Rectangle);
            });
    drawing_toolbar->addAction(rectangle_action);
    drawing_toolbar->addAction(m_ellipse_action);
    drawing_toolbar->addAction(m_spline_action);
    drawing_toolbar->addSeparator();

    QAction* dimension_action = createAction(tr("线性标注"), SIconType::Dimension);
    connect(dimension_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::LinearDimension);
            });
    QAction* text_action = createAction(tr("文字"), SIconType::Text);
    connect(text_action, &QAction::triggered, this, &SCadMainWindow::beginTextCommand);
    QAction* hatch_action = createAction(tr("填充"), SIconType::Hatch);
    registerShortcutAction(dimension_action, QStringLiteral("annotation.dimension_linear"));
    registerShortcutAction(text_action, QStringLiteral("annotation.text"));
    registerShortcutAction(hatch_action, QStringLiteral("annotation.hatch"));
    registerToolAction(dimension_action, SToolMode::LinearDimension);
    registerToolAction(text_action, SToolMode::Text);
    registerToolAction(hatch_action, SToolMode::Hatch);
    connect(hatch_action, &QAction::triggered, this,
            [this]()
            {
                m_workspace->viewport()->setToolMode(SToolMode::Hatch);
            });
    drawing_toolbar->addAction(dimension_action);
    drawing_toolbar->addAction(text_action);
    drawing_toolbar->addAction(hatch_action);
    return drawing_toolbar;
}

void SCadMainWindow::createDockingWorkspace()
{
    const SDesignToken& tokens = m_theme_manager.tokens();
    const auto icon_for = [&tokens](SIconType icon_type)
    {
        return SIconProvider::createIcon(icon_type, tokens.text_primary, tokens.accent);
    };
    ads::CDockManager::setConfigFlag(ads::CDockManager::OpaqueSplitterResize, true);
    ads::CDockManager::setConfigFlag(ads::CDockManager::FocusHighlighting, true);
    ads::CDockManager::setConfigFlag(ads::CDockManager::ActiveTabHasCloseButton, false);
    ads::CDockManager::setConfigFlag(ads::CDockManager::AllTabsHaveCloseButton, false);
    ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaHasCloseButton, false);
    ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaHasUndockButton, false);
    ads::CDockManager::setConfigFlag(ads::CDockManager::DoubleClickUndocksWidget, false);
    ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaHideDisabledButtons, true);
    ads::CDockManager::setAutoHideConfigFlag(ads::CDockManager::AutoHideFeatureEnabled, false);
    ads::CDockManager::setAutoHideConfigFlag(ads::CDockManager::DockAreaHasAutoHideButton,
                                             false);
    auto* workspace_container = new QWidget(this);
    workspace_container->setObjectName(QStringLiteral("smartWorkspaceContainer"));
    auto* workspace_layout = new QHBoxLayout(workspace_container);
    workspace_layout->setContentsMargins(0, 0, 0, 0);
    workspace_layout->setSpacing(0);
    m_dock_manager = new ads::CDockManager(workspace_container);

    m_workspace = new SCadWorkspaceWidget();
    m_workspace->setDocument(m_document.get());
    configureQuickOperationBar();
    workspace_layout->addWidget(createDrawingToolBar());
    workspace_layout->addWidget(m_dock_manager, 1);
    setCentralWidget(workspace_container);
    auto* center_dock = new ads::CDockWidget(m_dock_manager, tr("图形"));
    center_dock->setIcon(icon_for(SIconType::Viewport));
    m_icon_docks.emplace_back(center_dock, SIconType::Viewport);
    center_dock->setWidget(m_workspace);
    center_dock->setFeature(ads::CDockWidget::DockWidgetClosable, false);
    center_dock->setFeature(ads::CDockWidget::DockWidgetMovable, false);
    m_dock_manager->setCentralWidget(center_dock);

    ads::CDockWidget* layer_dock = createLayerDock();

    auto* property_dock = new ads::CDockWidget(m_dock_manager, tr("特性"), this);
    property_dock->setObjectName(QStringLiteral("smartPropertiesDock"));
    m_property_dock = property_dock;
    property_dock->setMinimumSizeHintMode(
        ads::CDockWidget::MinimumSizeHintFromContentMinimumSize);
    property_dock->setIcon(icon_for(SIconType::Properties));
    m_icon_docks.emplace_back(property_dock, SIconType::Properties);
    auto* property_panel = new QWidget(property_dock);
    property_panel->setObjectName(QStringLiteral("smartPropertiesPanel"));
    property_panel->setMinimumWidth(290);
    auto* property_layout = new QVBoxLayout(property_panel);
    property_layout->setContentsMargins(6, 6, 6, 6);
    property_layout->setSpacing(6);

    auto* property_summary = new QFrame(property_panel);
    property_summary->setObjectName(QStringLiteral("smartPropertySummary"));
    auto* property_summary_layout = new QGridLayout(property_summary);
    property_summary_layout->setContentsMargins(8, 4, 8, 8);
    property_summary_layout->setHorizontalSpacing(8);
    property_summary_layout->setVerticalSpacing(4);
    auto* property_caption = new QLabel(tr("当前选择"), property_summary);
    property_caption->setObjectName(QStringLiteral("smartPropertyCaption"));
    auto* property_summary_label = new QLabel(tr("未选择对象"), property_summary);
    property_summary_label->setObjectName(QStringLiteral("smartPropertySummaryText"));
    property_summary_layout->addWidget(property_caption, 0, 0, 1, 2);
    property_summary_layout->addWidget(property_summary_label, 1, 0, 1, 2);

    auto* layer_caption = new QLabel(tr("图层"), property_summary);
    layer_caption->setObjectName(QStringLiteral("smartPropertyCaption"));
    auto* property_layer_combo = new QComboBox(property_summary);
    property_layer_combo->setObjectName(QStringLiteral("smartPropertyCombo"));
    property_layer_combo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    property_summary_layout->addWidget(layer_caption, 2, 0);
    property_summary_layout->addWidget(property_layer_combo, 2, 1);

    auto* color_caption = new QLabel(tr("颜色"), property_summary);
    color_caption->setObjectName(QStringLiteral("smartPropertyCaption"));
    auto* property_color_button = new QPushButton(tr("未选择"), property_summary);
    property_color_button->setObjectName(QStringLiteral("smartPropertyColorButton"));
    property_color_button->setToolTip(tr("颜色绑定到图层；修改后会影响所选对象所在图层的全部实体"));
    property_summary_layout->addWidget(color_caption, 3, 0);
    property_summary_layout->addWidget(property_color_button, 3, 1);

    auto* line_width_caption = new QLabel(tr("线宽"), property_summary);
    line_width_caption->setObjectName(QStringLiteral("smartPropertyCaption"));
    auto* property_line_width_combo = new QComboBox(property_summary);
    property_line_width_combo->setObjectName(QStringLiteral("smartPropertyCombo"));
    property_line_width_combo->setToolTip(tr("修改选择集线宽；底部状态栏可切换线宽显示"));
    property_summary_layout->addWidget(line_width_caption, 4, 0);
    property_summary_layout->addWidget(property_line_width_combo, 4, 1);
    property_summary_layout->setColumnStretch(1, 1);
    property_layout->addWidget(property_summary);

    auto* property_tree = new QTreeWidget(property_panel);
    property_tree->setObjectName(QStringLiteral("smartPropertyTree"));
    property_tree->setColumnCount(2);
    property_tree->setHeaderLabels({tr("属性"), tr("值")});
    property_tree->setAlternatingRowColors(false);
    property_tree->setAnimated(false);
    property_tree->setIndentation(14);
    property_tree->setUniformRowHeights(true);
    property_tree->header()->setStretchLastSection(false);
    property_tree->header()->setSectionResizeMode(0, QHeaderView::Interactive);
    property_tree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    property_tree->setColumnWidth(0, 190);
    property_layout->addWidget(property_tree, 1);

    const auto apply_property_style = [this, property_panel]()
    {
        property_panel->setStyleSheet(propertyPanelStyle(m_theme_manager.tokens()));
    };
    const auto refresh_properties = [this, property_tree, property_layer_combo,
                                     property_color_button, property_line_width_combo,
                                     property_summary_label]()
    {
        const QVector<quint64> entity_ids = m_workspace->viewport()->selectedEntityIds();
        property_tree->clear();
        property_layer_combo->clear();
        property_line_width_combo->clear();
        if (entity_ids.isEmpty())
        {
            property_summary_label->setText(tr("未选择对象"));
            auto* empty_item = new QTreeWidgetItem(property_tree, {tr("没有选中的对象"), tr("—")});
            empty_item->setForeground(0, m_theme_manager.tokens().text_secondary);
            property_layer_combo->addItem(tr("未选择"));
            property_line_width_combo->addItem(tr("未选择"));
            property_color_button->setText(tr("未选择"));
            property_color_button->setStyleSheet(
                propertyColorButtonStyle({}, m_theme_manager.tokens()));
            property_layer_combo->setEnabled(false);
            property_color_button->setEnabled(false);
            property_line_width_combo->setEnabled(false);
            return;
        }

        property_summary_label->setText(tr("已选择 %1 个对象").arg(entity_ids.size()));
        QString common_layer;
        bool has_multiple_layers = false;
        QColor common_color;
        bool has_color = false;
        bool has_multiple_colors = false;
        double common_line_width = -1.0;
        bool has_line_width = false;
        bool has_multiple_line_widths = false;
        auto* selection_item =
            new QTreeWidgetItem(property_tree, {entity_ids.size() == 1 ? tr("对象") : tr("选择集"),
                                                tr("%1 个实体").arg(entity_ids.size())});
        QFont selection_font = selection_item->font(0);
        selection_font.setBold(true);
        selection_item->setFont(0, selection_font);
        QColor selection_background = m_theme_manager.tokens().accent;
        selection_background.setAlpha(38);
        selection_item->setBackground(0, selection_background);
        selection_item->setBackground(1, selection_background);
        QVector<QTreeWidgetItem*> entity_items;
        for (quint64 entity_id : entity_ids)
        {
            const auto iterator =
                std::find_if(m_document->entities().begin(), m_document->entities().end(),
                             [entity_id](const SEntityRecord& entity)
                             {
                                 return entity.id == entity_id;
                             });
            if (iterator == m_document->entities().end())
            {
                continue;
            }
            if (common_layer.isEmpty())
            {
                common_layer = iterator->layer_name;
            }
            else if (common_layer != iterator->layer_name)
            {
                has_multiple_layers = true;
            }
            const QColor entity_color = m_document->layerColor(iterator->layer_name);
            if (!has_color)
            {
                common_color = entity_color;
                has_color = true;
            }
            else if (common_color != entity_color)
            {
                has_multiple_colors = true;
            }
            if (!has_line_width)
            {
                common_line_width = iterator->line_width_mm;
                has_line_width = true;
            }
            else if (std::abs(common_line_width - iterator->line_width_mm) > 1.0e-9)
            {
                has_multiple_line_widths = true;
            }
            entity_items.append(addEntityPropertyTree(selection_item, *iterator, *m_document,
                                                      m_theme_manager.tokens()));
        }
        selection_item->setExpanded(true);
        for (int index = 0; index < entity_items.size(); ++index)
        {
            const bool is_expanded = entity_items.size() == 1 || index == 0;
            entity_items[index]->setExpanded(is_expanded);
            if (is_expanded)
            {
                for (int child_index = 0; child_index < entity_items[index]->childCount();
                     ++child_index)
                {
                    entity_items[index]->child(child_index)->setExpanded(true);
                }
            }
        }

        if (has_multiple_layers)
        {
            property_layer_combo->addItem(tr("<多种图层>"), QString());
        }
        for (const SLayerRecord& layer_record : m_document->layers())
        {
            property_layer_combo->addItem(tr("[%1] %2").arg(layer_record.id).arg(layer_record.name),
                                          layer_record.name);
        }
        if (!has_multiple_layers)
        {
            property_layer_combo->setCurrentIndex(property_layer_combo->findData(common_layer));
        }
        else
        {
            property_layer_combo->setCurrentIndex(0);
        }
        property_layer_combo->setEnabled(true);
        property_color_button->setText(has_multiple_colors
                                           ? tr("<多种颜色>")
                                           : tr("%1（随图层）").arg(common_color.name().toUpper()));
        property_color_button->setStyleSheet(propertyColorButtonStyle(
            has_multiple_colors ? QColor() : common_color, m_theme_manager.tokens()));
        property_color_button->setEnabled(true);
        populateLineWidthCombo(property_line_width_combo, true);
        if (has_multiple_line_widths)
        {
            property_line_width_combo->insertItem(0, tr("<多种线宽>"), QVariant());
            property_line_width_combo->setCurrentIndex(0);
        }
        else
        {
            property_line_width_combo->setCurrentIndex(
                property_line_width_combo->findData(common_line_width));
        }
        property_line_width_combo->setEnabled(true);
    };
    connect(m_workspace->viewport(), &SCadViewport::selectionChanged, property_tree,
            [refresh_properties](const QVector<quint64>&)
            {
                refresh_properties();
            });
    connect(m_document.get(), &SCadDocument::documentChanged, property_tree, refresh_properties);
    connect(m_document.get(), &SCadDocument::layersChanged, property_tree, refresh_properties);
    connect(&m_theme_manager, &SThemeManager::themeChanged, property_panel,
            [apply_property_style, refresh_properties](SThemeMode)
            {
                apply_property_style();
                refresh_properties();
            });
    connect(property_layer_combo, QOverload<int>::of(&QComboBox::activated), this,
            [this, property_layer_combo](int index)
            {
                const QString layer_name = property_layer_combo->itemData(index).toString();
                if (layer_name.isEmpty())
                {
                    return;
                }
                const QVector<quint64> entity_ids = m_workspace->viewport()->selectedEntityIds();
                auto transaction = m_document->beginTransaction(tr("更改选择集图层"));
                bool has_changes = false;
                for (quint64 entity_id : entity_ids)
                {
                    has_changes = transaction->setEntityLayer(entity_id, layer_name) || has_changes;
                }
                if (has_changes)
                {
                    transaction->commit();
                }
            });
    connect(property_color_button, &QPushButton::clicked, this,
            [this]()
            {
                QColor initial_color;
                const QVector<quint64> selected_ids =
                    m_workspace->viewport()->selectedEntityIds();
                for (quint64 entity_id : selected_ids)
                {
                    const auto iterator =
                        std::find_if(m_document->entities().begin(), m_document->entities().end(),
                                     [entity_id](const SEntityRecord& entity)
                                     {
                                         return entity.id == entity_id;
                                     });
                    if (iterator != m_document->entities().end())
                    {
                        initial_color = m_document->layerColor(iterator->layer_name);
                        break;
                    }
                }
                if (!initial_color.isValid())
                {
                    return;
                }
                QColorDialog color_dialog(initial_color, this);
                color_dialog.setWindowTitle(tr("修改实体颜色"));
                color_dialog.setOption(QColorDialog::DontUseNativeDialog);
                color_dialog.setMinimumSize(720, 520);
                const bool is_accepted = color_dialog.exec() == QDialog::Accepted;
                const QColor selected_color = color_dialog.currentColor();
                if (!is_accepted || !selected_color.isValid())
                {
                    return;
                }
                std::vector<SEntityId> entity_ids;
                entity_ids.reserve(static_cast<std::size_t>(selected_ids.size()));
                for (quint64 entity_id : selected_ids)
                {
                    entity_ids.push_back(entity_id);
                }
                const QString new_layer_name =
                    m_document->assignEntitiesToColorLayer(entity_ids, selected_color);
                if (new_layer_name.isEmpty())
                {
                    SDialogService::warning(
                        this, tr("修改实体颜色"),
                        tr("未能修改颜色。请确认实体未处于锁定图层。"));
                }
            });
    connect(property_line_width_combo, QOverload<int>::of(&QComboBox::activated), this,
            [this, property_line_width_combo](int index)
            {
                const QVariant line_width_data = property_line_width_combo->itemData(index);
                if (!line_width_data.isValid())
                {
                    return;
                }
                const double line_width_mm = line_width_data.toDouble();
                auto transaction = m_document->beginTransaction(tr("更改选择集线宽"));
                bool has_changes = false;
                for (quint64 entity_id : m_workspace->viewport()->selectedEntityIds())
                {
                    has_changes =
                        transaction->setEntityLineWidth(entity_id, line_width_mm) || has_changes;
                }
                if (has_changes)
                {
                    transaction->commit();
                }
            });
    apply_property_style();
    refresh_properties();
    property_dock->setWidget(property_panel);
    m_dock_manager->addDockWidget(ads::BottomDockWidgetArea, property_dock,
                                  layer_dock->dockAreaWidget());

    auto* external_reference_dock = createListDock(m_dock_manager, tr("外部参照"),
                                                   {tr("DWG 参照"), tr("PDF 参照"), tr("图像参照")},
                                                   icon_for(SIconType::ExternalReference), this);
    m_icon_docks.emplace_back(external_reference_dock, SIconType::ExternalReference);
    m_dock_manager->addDockWidget(ads::CenterDockWidgetArea, external_reference_dock,
                                  layer_dock->dockAreaWidget());

    auto* tools_dock =
        createListDock(m_dock_manager, tr("工具选项板"), {tr("直线"), tr("圆"), tr("常用标注")},
                       icon_for(SIconType::ToolPalette), this);
    m_icon_docks.emplace_back(tools_dock, SIconType::ToolPalette);
    m_dock_manager->addDockWidget(ads::CenterDockWidgetArea, tools_dock,
                                  layer_dock->dockAreaWidget());

    auto* design_center_dock = createListDock(m_dock_manager, tr("设计中心"),
                                              {tr("收藏夹"), tr("打开的图形"), tr("资源库")},
                                              icon_for(SIconType::DesignCenter), this);
    m_icon_docks.emplace_back(design_center_dock, SIconType::DesignCenter);
    m_dock_manager->addDockWidget(ads::CenterDockWidgetArea, design_center_dock,
                                  layer_dock->dockAreaWidget());

    auto* sheet_set_dock =
        createListDock(m_dock_manager, tr("图纸集"), {tr("图纸"), tr("视图"), tr("模型视图")},
                       icon_for(SIconType::SheetSet), this);
    m_icon_docks.emplace_back(sheet_set_dock, SIconType::SheetSet);
    m_dock_manager->addDockWidget(ads::CenterDockWidgetArea, sheet_set_dock,
                                  layer_dock->dockAreaWidget());

    m_command_line = new SCommandLineWidget();
    m_command_line->setMinimumHeight(68);
    auto* command_dock = new ads::CDockWidget(m_dock_manager, tr("命令行"));
    command_dock->setObjectName(QStringLiteral("smartCommandDock"));
    m_command_dock = command_dock;
    command_dock->setMinimumSizeHintMode(
        ads::CDockWidget::MinimumSizeHintFromContentMinimumSize);
    command_dock->setIcon(icon_for(SIconType::CommandLine));
    m_icon_docks.emplace_back(command_dock, SIconType::CommandLine);
    command_dock->setWidget(m_command_line);
    command_dock->setFeature(ads::CDockWidget::DockWidgetClosable, false);
    m_dock_manager->addDockWidget(ads::BottomDockWidgetArea, command_dock);

    auto* report_dock = new ads::CDockWidget(m_dock_manager, tr("兼容性报告"), this);
    report_dock->setIcon(icon_for(SIconType::CompatibilityReport));
    m_icon_docks.emplace_back(report_dock, SIconType::CompatibilityReport);
    auto* report_view = new QTextBrowser(report_dock);
    report_view->setText(
        tr("当前原生 .smartcam/.smartcad 格式无兼容性警告。\n"
           "DWG/DXF 适配器将在后续里程碑启用。"));
    report_dock->setWidget(report_view);
    m_dock_manager->addDockWidget(ads::CenterDockWidgetArea, report_dock,
                                  command_dock->dockAreaWidget());
    external_reference_dock->toggleView(false);
    tools_dock->toggleView(false);
    design_center_dock->toggleView(false);
    sheet_set_dock->toggleView(false);
    report_dock->toggleView(false);
    property_dock->toggleView(false);

    m_dock_manager->lockDockWidgetFeaturesGlobally();
    configureDockSplitters();

    const auto add_dock_shortcut =
        [this](const QKeySequence& key_sequence, ads::CDockWidget* dock_widget,
               const QString& command_id, SIconType icon_type)
    {
        QAction* shortcut =
            createAction(tr("显示%1").arg(dock_widget->windowTitle()), icon_type);
        registerShortcutAction(shortcut, command_id, key_sequence);
        addAction(shortcut);
        connect(shortcut, &QAction::triggered, dock_widget,
                [dock_widget]()
                {
                    dock_widget->toggleView(true);
                    dock_widget->raise();
                    dock_widget->setFocus(Qt::ShortcutFocusReason);
                });
    };
    add_dock_shortcut(QKeySequence(QStringLiteral("Ctrl+3")), tools_dock,
                      QStringLiteral("ui.tool_palette"), SIconType::ToolPalette);
    add_dock_shortcut(QKeySequence(QStringLiteral("Ctrl+9")), command_dock,
                      QStringLiteral("ui.command_line"), SIconType::CommandLine);
    QAction* command_search_shortcut =
        createAction(tr("命令搜索"), SIconType::CommandLine);
    registerShortcutAction(command_search_shortcut, QStringLiteral("ui.command_search"),
                           QKeySequence(QStringLiteral("Ctrl+K")));
    addAction(command_search_shortcut);
    connect(command_search_shortcut, &QAction::triggered, m_command_line,
            &SCommandLineWidget::focusInput);

    connect(m_command_line, &SCommandLineWidget::commandSubmitted, this,
            &SCadMainWindow::executeCommand);
    connect(m_command_line, &SCommandLineWidget::commandInputChanged, this,
            &SCadMainWindow::previewCommandInput);
    connect(m_workspace->viewport(), &SCadViewport::commandMessage, m_command_line,
            &SCommandLineWidget::appendMessage);
    connect(m_workspace->viewport(), &SCadViewport::toolModeChanged, this,
            &SCadMainWindow::updateToolButtonHighlight);
    updateToolButtonHighlight(SToolMode::Select);
}

void SCadMainWindow::configureDockSplitters()
{
    for (QSplitter* splitter : m_dock_manager->findChildren<QSplitter*>())
    {
        splitter->setChildrenCollapsible(false);
        splitter->setHandleWidth(5);
        for (int handle_index = 1; handle_index < splitter->count(); ++handle_index)
        {
            auto* handle = splitter->handle(handle_index);
            handle->setEnabled(true);
            handle->setToolTip(tr("拖动调整区域大小"));
            handle->setAccessibleName(tr("区域大小调整分隔线"));
        }
    }
}

} // namespace smartCam
