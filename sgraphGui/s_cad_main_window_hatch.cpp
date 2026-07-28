#include "s_cad_main_window.h"
#include "s_cad_viewport.h"
#include "s_cad_workspace_widget.h"
#include "s_dialog_service.h"
#include "s_command_line_widget.h"
#include "s_icon_provider.h"

#include <QAction>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <SARibbonPanel.h>

namespace smartGraphics
{
namespace
{

QString colorButtonStyle(const QColor& color)
{
    return QStringLiteral("QPushButton { background:%1; min-height:24px; }").arg(color.name());
}

bool parseHatchCommand(const QStringList& parts, int option_index, SHatchEntity& settings)
{
    if (parts.size() <= option_index)
    {
        return true;
    }
    const QString option = parts[option_index].toUpper();
    if (option == QLatin1String("SOLID"))
    {
        settings.fill_type = SHatchFillType::Solid;
        return parts.size() == option_index + 1;
    }
    if (option == QLatin1String("PATTERN") && parts.size() >= option_index + 2)
    {
        bool is_scale_valid = parts.size() < option_index + 3;
        bool is_angle_valid = parts.size() < option_index + 4;
        settings.fill_type = SHatchFillType::Pattern;
        settings.pattern_name = parts[option_index + 1].toUpper();
        settings.pattern_scale = parts.size() >= option_index + 3
                                     ? parts[option_index + 2].toDouble(&is_scale_valid)
                                     : 1.0;
        settings.pattern_angle = parts.size() >= option_index + 4
                                     ? parts[option_index + 3].toDouble(&is_angle_valid)
                                     : 45.0;
        return is_scale_valid && is_angle_valid && settings.pattern_scale > 0.0;
    }
    if (option == QLatin1String("GRADIENT") && parts.size() >= option_index + 3)
    {
        bool is_angle_valid = parts.size() < option_index + 4;
        const QColor start_color(parts[option_index + 1]);
        const QColor end_color(parts[option_index + 2]);
        settings.fill_type = SHatchFillType::Gradient;
        settings.gradient_start = start_color;
        settings.gradient_end = end_color;
        settings.pattern_angle = parts.size() >= option_index + 4
                                     ? parts[option_index + 3].toDouble(&is_angle_valid)
                                     : 0.0;
        return start_color.isValid() && end_color.isValid() && is_angle_valid;
    }
    return false;
}

} // namespace

void SCadMainWindow::configureHatchPanel(SARibbonPanel* annotation_panel)
{
    QAction* solid_action = createAction(tr("实体填充"), SIconType::Hatch);
    QAction* pattern_action = createAction(tr("图案填充"), SIconType::HatchPattern);
    QAction* gradient_action = createAction(tr("渐变填充"), SIconType::HatchGradient);
    QAction* edit_action = createAction(tr("编辑填充"), SIconType::HatchEdit);
    registerShortcutAction(solid_action, QStringLiteral("hatch.solid"));
    registerShortcutAction(pattern_action, QStringLiteral("hatch.pattern"));
    registerShortcutAction(gradient_action, QStringLiteral("hatch.gradient"));
    registerShortcutAction(edit_action, QStringLiteral("hatch.edit"));
    connect(solid_action, &QAction::triggered, this,
            [this]()
            {
                SHatchEntity settings = m_workspace->viewport()->hatchSettings();
                settings.fill_type = SHatchFillType::Solid;
                m_workspace->viewport()->setHatchSettings(settings);
                m_workspace->viewport()->setToolMode(SToolMode::Hatch);
            });
    connect(pattern_action, &QAction::triggered, this,
            [this]()
            {
                showHatchSettings(false);
            });
    connect(gradient_action, &QAction::triggered, this,
            [this]()
            {
                SHatchEntity settings = m_workspace->viewport()->hatchSettings();
                settings.fill_type = SHatchFillType::Gradient;
                m_workspace->viewport()->setHatchSettings(settings);
                showHatchSettings(false);
            });
    connect(edit_action, &QAction::triggered, this,
            [this]()
            {
                showHatchSettings(true);
            });
    annotation_panel->addLargeAction(solid_action);
    annotation_panel->addLargeAction(pattern_action);
    annotation_panel->addLargeAction(gradient_action);
    annotation_panel->addLargeAction(edit_action);
}

void SCadMainWindow::showHatchSettings(bool edit_selected)
{
    SDialog dialog(this);
    dialog.setWindowTitle(edit_selected ? tr("编辑填充") : tr("填充设置"));
    dialog.resize(460, 330);
    auto* layout = new QVBoxLayout(&dialog);
    auto* form = new QFormLayout();
    auto* fill_type_combo = new QComboBox(&dialog);
    fill_type_combo->addItem(tr("实体"), static_cast<int>(SHatchFillType::Solid));
    fill_type_combo->addItem(tr("图案"), static_cast<int>(SHatchFillType::Pattern));
    fill_type_combo->addItem(tr("渐变"), static_cast<int>(SHatchFillType::Gradient));
    auto* pattern_combo = new QComboBox(&dialog);
    pattern_combo->addItems({QStringLiteral("ANSI31"), QStringLiteral("ANSI32"),
                             QStringLiteral("ANSI37"), QStringLiteral("CROSS"),
                             QStringLiteral("DOTS")});
    auto* scale_box = new QDoubleSpinBox(&dialog);
    auto* angle_box = new QDoubleSpinBox(&dialog);
    scale_box->setRange(0.001, 1.0e6);
    scale_box->setDecimals(3);
    angle_box->setRange(-3600.0, 3600.0);
    angle_box->setDecimals(2);
    auto* colors_layout = new QHBoxLayout();
    auto* start_color_button = new QPushButton(tr("起始色"), &dialog);
    auto* end_color_button = new QPushButton(tr("结束色"), &dialog);
    colors_layout->addWidget(start_color_button);
    colors_layout->addWidget(end_color_button);
    form->addRow(tr("填充类型："), fill_type_combo);
    form->addRow(tr("图案："), pattern_combo);
    form->addRow(tr("比例："), scale_box);
    form->addRow(tr("角度："), angle_box);
    form->addRow(tr("渐变颜色："), colors_layout);
    layout->addLayout(form);

    SHatchEntity settings = m_workspace->viewport()->hatchSettings();
    fill_type_combo->setCurrentIndex(
        fill_type_combo->findData(static_cast<int>(settings.fill_type)));
    pattern_combo->setCurrentText(settings.pattern_name);
    scale_box->setValue(settings.pattern_scale);
    angle_box->setValue(settings.pattern_angle);
    start_color_button->setStyleSheet(colorButtonStyle(settings.gradient_start));
    end_color_button->setStyleSheet(colorButtonStyle(settings.gradient_end));
    connect(start_color_button, &QPushButton::clicked, &dialog,
            [&dialog, &settings, start_color_button]()
            {
                const QColor color = QColorDialog::getColor(settings.gradient_start, &dialog);
                if (color.isValid())
                {
                    settings.gradient_start = color;
                    start_color_button->setStyleSheet(colorButtonStyle(color));
                }
            });
    connect(end_color_button, &QPushButton::clicked, &dialog,
            [&dialog, &settings, end_color_button]()
            {
                const QColor color = QColorDialog::getColor(settings.gradient_end, &dialog);
                if (color.isValid())
                {
                    settings.gradient_end = color;
                    end_color_button->setStyleSheet(colorButtonStyle(color));
                }
            });
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }
    settings.fill_type = static_cast<SHatchFillType>(fill_type_combo->currentData().toInt());
    settings.pattern_name = pattern_combo->currentText();
    settings.pattern_scale = scale_box->value();
    settings.pattern_angle = angle_box->value();
    m_workspace->viewport()->setHatchSettings(settings);
    if (edit_selected)
    {
        m_command_line->appendMessage(m_workspace->viewport()->editSelectedHatches(settings)
                                          ? tr("已更新所选填充。")
                                          : tr("请先选择一个或多个填充。"));
    }
    else
    {
        m_workspace->viewport()->setToolMode(SToolMode::Hatch);
    }
}

bool SCadMainWindow::executeHatchCommand(const QString& command)
{
    const QStringList parts = command.split(QLatin1Char(' '), QString::SkipEmptyParts);
    if (parts.isEmpty())
    {
        return false;
    }
    const bool is_create = parts.front().compare(QLatin1String("HATCH"), Qt::CaseInsensitive) == 0;
    const bool is_edit =
        parts.front().compare(QLatin1String("HATCHEDIT"), Qt::CaseInsensitive) == 0 ||
        parts.front().compare(QLatin1String("HE"), Qt::CaseInsensitive) == 0;
    if (!is_create && !is_edit)
    {
        return false;
    }
    SHatchEntity settings = m_workspace->viewport()->hatchSettings();
    if (!parseHatchCommand(parts, 1, settings))
    {
        m_command_line->appendMessage(
            tr("用法：HATCH/HATCHEDIT SOLID、PATTERN 名称 [比例] [角度]，或 GRADIENT "
               "#起始色 #结束色 [角度]。"));
        return true;
    }
    m_workspace->viewport()->setHatchSettings(settings);
    if (is_edit)
    {
        m_command_line->appendMessage(m_workspace->viewport()->editSelectedHatches(settings)
                                          ? tr("已更新所选填充。")
                                          : tr("HATCHEDIT 需要先选择填充。"));
    }
    else
    {
        m_workspace->viewport()->setToolMode(SToolMode::Hatch);
    }
    return true;
}

} // namespace smartGraphics
