#include "s_cad_main_window.h"

#include "s_cad_workspace_widget.h"
#include "s_dialog_service.h"
#include "s_theme_manager.h"

#include <DockWidget.h>
#include <DockWidgetTab.h>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <SARibbonBar.h>
#include <SARibbonQuickAccessBar.h>

namespace vectorPath
{

void SCadMainWindow::showInterfaceSettings()
{
    SDialog dialog(this);
    dialog.setWindowTitle(tr("界面比例"));
    auto* layout = new QVBoxLayout(&dialog);
    auto* description =
        new QLabel(tr("按比例同时调整界面字体、Ribbon、工具栏、状态栏和面板图标大小。"
                      "设置会自动保存，并在下次启动时恢复。"),
                   &dialog);
    description->setWordWrap(true);
    layout->addWidget(description);

    auto* scale_combo = new QComboBox(&dialog);
    scale_combo->setObjectName(QStringLiteral("smartUiScaleCombo"));
    const int scale_options[]{100, 125, 150, 175, 200};
    for (int scale_percent : scale_options)
    {
        scale_combo->addItem(tr("%1%").arg(scale_percent), scale_percent);
    }
    const int current_index = scale_combo->findData(m_theme_manager.uiScalePercent());
    scale_combo->setCurrentIndex(current_index >= 0 ? current_index : 0);
    layout->addWidget(new QLabel(tr("界面比例："), &dialog));
    layout->addWidget(scale_combo);
    layout->addStretch();

    auto* buttons =
        new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() == QDialog::Accepted)
    {
        m_theme_manager.setUiScalePercent(scale_combo->currentData().toInt());
    }
}

void SCadMainWindow::applyUiScale(int scale_percent)
{
    const auto scaled = [scale_percent](int value)
    {
        return qRound(static_cast<double>(value) * scale_percent / 100.0);
    };

    SARibbonBar* ribbon = ribbonBar();
    ribbon->setTitleBarHeight(scaled(34));
    ribbon->setTabBarHeight(scaled(28));
    ribbon->setCategoryHeight(scaled(128));
    ribbon->quickAccessBar()->setIconSize(QSize(scaled(24), scaled(24)));
    ribbon->setPanelToolButtonIconSize(QSize(scaled(28), scaled(28)),
                                       QSize(scaled(28), scaled(28)));

    if (auto* drawing_toolbar =
            findChild<QToolBar*>(QStringLiteral("smartDrawingToolBar")))
    {
        drawing_toolbar->setIconSize(QSize(scaled(30), scaled(30)));
        drawing_toolbar->setFixedWidth(scaled(64));
    }
    for (const auto& icon_button : m_icon_status_buttons)
    {
        icon_button.first->setIconSize(QSize(scaled(22), scaled(22)));
        icon_button.first->setFixedSize(scaled(30), scaled(29));
    }
    for (const auto& icon_dock : m_icon_docks)
    {
        icon_dock.first->tabWidget()->setIconSize(QSize(scaled(22), scaled(22)));
    }
    for (QPushButton* button : findChildren<QPushButton*>())
    {
        if (!button->icon().isNull())
        {
            button->setIconSize(QSize(scaled(22), scaled(22)));
        }
    }
    if (m_workspace)
    {
        m_workspace->applyUiScale(scale_percent);
    }
}

} // namespace vectorPath
