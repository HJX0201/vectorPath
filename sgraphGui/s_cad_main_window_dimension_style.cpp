#include "s_cad_document.h"
#include "s_cad_main_window.h"
#include "s_dialog_service.h"
#include "s_cad_viewport.h"
#include "s_cad_workspace_widget.h"
#include "s_command_line_widget.h"
#include "s_dimension_style_record.h"
#include "s_document_transaction.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <algorithm>

namespace smartCam
{

void SCadMainWindow::showDimensionStyleManager()
{
    SDialog dialog(this);
    dialog.setWindowTitle(tr("标注样式管理器"));
    dialog.resize(580, 500);
    auto* layout = new QVBoxLayout(&dialog);
    auto* current_label = new QLabel(&dialog);
    auto* style_combo = new QComboBox(&dialog);
    layout->addWidget(current_label);
    layout->addWidget(style_combo);

    auto* form = new QFormLayout();
    auto* name_edit = new QLineEdit(&dialog);
    auto* text_height_box = new QDoubleSpinBox(&dialog);
    auto* arrow_size_box = new QDoubleSpinBox(&dialog);
    auto* overall_scale_box = new QDoubleSpinBox(&dialog);
    auto* linear_scale_box = new QDoubleSpinBox(&dialog);
    auto* linear_precision_box = new QSpinBox(&dialog);
    auto* angular_precision_box = new QSpinBox(&dialog);
    auto* prefix_edit = new QLineEdit(&dialog);
    auto* suffix_edit = new QLineEdit(&dialog);
    auto* suppress_zeros_check = new QCheckBox(tr("抑制末尾零"), &dialog);
    for (QDoubleSpinBox* box :
         {text_height_box, arrow_size_box, overall_scale_box, linear_scale_box})
    {
        box->setRange(0.001, 1.0e6);
        box->setDecimals(3);
    }
    linear_precision_box->setRange(0, 8);
    angular_precision_box->setRange(0, 8);
    form->addRow(tr("样式名称："), name_edit);
    form->addRow(tr("文字高度："), text_height_box);
    form->addRow(tr("箭头大小："), arrow_size_box);
    form->addRow(tr("总体比例："), overall_scale_box);
    form->addRow(tr("测量比例："), linear_scale_box);
    form->addRow(tr("线性精度："), linear_precision_box);
    form->addRow(tr("角度精度："), angular_precision_box);
    form->addRow(tr("前缀："), prefix_edit);
    form->addRow(tr("后缀："), suffix_edit);
    form->addRow(tr("零抑制："), suppress_zeros_check);
    layout->addLayout(form);

    const auto load_style = [this, style_combo, name_edit, text_height_box, arrow_size_box,
                             overall_scale_box, linear_scale_box, linear_precision_box,
                             angular_precision_box, prefix_edit, suffix_edit, suppress_zeros_check,
                             current_label]()
    {
        const SDimensionStyleRecord* style = m_document->dimensionStyle(style_combo->currentText());
        if (!style)
        {
            return;
        }
        name_edit->setText(style->name);
        text_height_box->setValue(style->text_height);
        arrow_size_box->setValue(style->arrow_size);
        overall_scale_box->setValue(style->overall_scale);
        linear_scale_box->setValue(style->linear_scale);
        linear_precision_box->setValue(style->linear_precision);
        angular_precision_box->setValue(style->angular_precision);
        prefix_edit->setText(style->prefix);
        suffix_edit->setText(style->suffix);
        suppress_zeros_check->setChecked(style->suppress_trailing_zeros);
        current_label->setText(tr("当前标注样式：%1").arg(m_document->currentDimensionStyleName()));
    };
    const auto refresh = [this, style_combo, load_style]()
    {
        const QString selected_name = style_combo->currentText();
        style_combo->clear();
        for (const SDimensionStyleRecord& style : m_document->dimensionStyles())
        {
            style_combo->addItem(style.name);
        }
        const int selected_index = style_combo->findText(selected_name, Qt::MatchFixedString);
        style_combo->setCurrentIndex(std::max(0, selected_index));
        load_style();
    };
    connect(style_combo, qOverload<int>(&QComboBox::currentIndexChanged), &dialog,
            [load_style](int)
            {
                load_style();
            });

    auto* action_layout = new QHBoxLayout();
    auto* save_button = new QPushButton(tr("添加/更新"), &dialog);
    auto* current_button = new QPushButton(tr("设为当前"), &dialog);
    auto* delete_button = new QPushButton(tr("删除"), &dialog);
    action_layout->addWidget(save_button);
    action_layout->addWidget(current_button);
    action_layout->addWidget(delete_button);
    action_layout->addStretch();
    layout->addLayout(action_layout);
    connect(save_button, &QPushButton::clicked, &dialog,
            [this, name_edit, text_height_box, arrow_size_box, overall_scale_box, linear_scale_box,
             linear_precision_box, angular_precision_box, prefix_edit, suffix_edit,
             suppress_zeros_check, refresh]()
            {
                SDimensionStyleRecord style;
                style.name = name_edit->text();
                style.text_height = text_height_box->value();
                style.arrow_size = arrow_size_box->value();
                style.overall_scale = overall_scale_box->value();
                style.linear_scale = linear_scale_box->value();
                style.linear_precision = linear_precision_box->value();
                style.angular_precision = angular_precision_box->value();
                style.prefix = prefix_edit->text();
                style.suffix = suffix_edit->text();
                style.suppress_trailing_zeros = suppress_zeros_check->isChecked();
                if (!m_document->addOrUpdateDimensionStyle(style))
                {
                    SDialogService::warning(this, tr("标注样式"), tr("样式参数无效。"));
                    return;
                }
                refresh();
            });
    connect(current_button, &QPushButton::clicked, &dialog,
            [this, style_combo, refresh]()
            {
                if (m_document->setCurrentDimensionStyle(style_combo->currentText()))
                {
                    refresh();
                }
            });
    connect(delete_button, &QPushButton::clicked, &dialog,
            [this, style_combo, refresh]()
            {
                if (!m_document->removeDimensionStyle(style_combo->currentText()))
                {
                    SDialogService::warning(
                        this, tr("标注样式"),
                        tr("Standard、当前样式或已被标注引用的样式不能删除。"));
                    return;
                }
                refresh();
            });
    auto* close_buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(close_buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(close_buttons);
    refresh();
    dialog.exec();
}

bool SCadMainWindow::executeDimensionStyleCommand(const QString& command)
{
    const QStringList parts = command.split(QLatin1Char(' '), QString::SkipEmptyParts);
    if (parts.isEmpty())
    {
        return false;
    }
    const bool is_style_command =
        parts.front().compare(QLatin1String("DIMSTYLE"), Qt::CaseInsensitive) == 0;
    const bool is_override_command =
        parts.front().compare(QLatin1String("DIMOVERRIDE"), Qt::CaseInsensitive) == 0;
    if (!is_style_command && !is_override_command)
    {
        return false;
    }
    if (is_style_command && parts.size() == 1)
    {
        showDimensionStyleManager();
        return true;
    }
    if (is_style_command && parts.size() == 3 &&
        parts[1].compare(QLatin1String("SET"), Qt::CaseInsensitive) == 0)
    {
        const bool is_success = m_document->setCurrentDimensionStyle(parts[2]);
        m_command_line->appendMessage(is_success ? tr("当前标注样式：%1").arg(parts[2])
                                                 : tr("DIMSTYLE SET 失败：样式不存在。"));
        return true;
    }
    if (is_style_command && parts.size() == 3 &&
        parts[1].compare(QLatin1String("DELETE"), Qt::CaseInsensitive) == 0)
    {
        m_command_line->appendMessage(m_document->removeDimensionStyle(parts[2])
                                          ? tr("标注样式已删除：%1").arg(parts[2])
                                          : tr("DIMSTYLE DELETE 失败：样式受保护或正在使用。"));
        return true;
    }
    if (is_style_command && parts.size() >= 5 &&
        parts[1].compare(QLatin1String("ADD"), Qt::CaseInsensitive) == 0)
    {
        bool is_text_height_valid = false;
        bool is_arrow_size_valid = false;
        bool is_scale_valid = parts.size() < 6;
        bool is_linear_precision_valid = parts.size() < 7;
        bool is_angular_precision_valid = parts.size() < 8;
        SDimensionStyleRecord style;
        style.name = parts[2];
        style.text_height = parts[3].toDouble(&is_text_height_valid);
        style.arrow_size = parts[4].toDouble(&is_arrow_size_valid);
        style.overall_scale = parts.size() >= 6 ? parts[5].toDouble(&is_scale_valid) : 1.0;
        style.linear_precision = parts.size() >= 7 ? parts[6].toInt(&is_linear_precision_valid) : 3;
        style.angular_precision =
            parts.size() >= 8 ? parts[7].toInt(&is_angular_precision_valid) : 2;
        const bool is_success = is_text_height_valid && is_arrow_size_valid && is_scale_valid &&
                                is_linear_precision_valid && is_angular_precision_valid &&
                                m_document->addOrUpdateDimensionStyle(style);
        m_command_line->appendMessage(
            is_success ? tr("标注样式已添加或更新：%1").arg(style.name)
                       : tr("用法：DIMSTYLE ADD 名称 文字高度 箭头大小 [总体比例] [线性精度] "
                            "[角度精度]。"));
        return true;
    }
    if (is_override_command)
    {
        const QVector<quint64> selected_ids = m_workspace->viewport()->selectedEntityIds();
        auto transaction = m_document->beginTransaction(tr("替代标注属性"));
        int changed_count = 0;
        for (quint64 selected_id : selected_ids)
        {
            const auto iterator =
                std::find_if(m_document->entities().begin(), m_document->entities().end(),
                             [selected_id](const SEntityRecord& entity)
                             {
                                 return entity.id == selected_id;
                             });
            if (iterator == m_document->entities().end() ||
                iterator->type != SEntityType::LinearDimension)
            {
                continue;
            }
            SEntityRecord replacement = *iterator;
            auto& dimension = std::get<SLinearDimensionEntity>(replacement.geometry);
            if (parts.size() == 3 &&
                parts[1].compare(QLatin1String("STYLE"), Qt::CaseInsensitive) == 0 &&
                m_document->dimensionStyle(parts[2]))
            {
                dimension.style_name = m_document->dimensionStyle(parts[2])->name;
            }
            else if (parts.size() >= 3 &&
                     parts[1].compare(QLatin1String("TEXT"), Qt::CaseInsensitive) == 0)
            {
                dimension.text_override = command.section(QLatin1Char(' '), 2);
            }
            else if (parts.size() == 2 &&
                     parts[1].compare(QLatin1String("CLEAR"), Qt::CaseInsensitive) == 0)
            {
                dimension.style_name = m_document->currentDimensionStyleName();
                dimension.text_override.clear();
            }
            else
            {
                continue;
            }
            if (transaction->replaceEntity(iterator->id, std::move(replacement)))
            {
                ++changed_count;
            }
        }
        if (changed_count > 0)
        {
            transaction->commit();
        }
        m_command_line->appendMessage(changed_count > 0
                                          ? tr("已更新 %1 个标注。").arg(changed_count)
                                          : tr("DIMOVERRIDE 未更新标注；请先选择标注并检查参数。"));
        return true;
    }
    m_command_line->appendMessage(
        tr("DIMSTYLE 用法：DIMSTYLE、ADD、SET、DELETE；DIMOVERRIDE 用法：STYLE、TEXT、CLEAR。"));
    return true;
}

} // namespace smartCam
