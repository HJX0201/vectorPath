#include "s_cad_document.h"
#include "s_cad_main_window.h"
#include "s_command_line_widget.h"
#include "s_dialog_service.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFontComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <algorithm>
#include <functional>

namespace smartGraphics
{

void SCadMainWindow::showTextStyleManager()
{
    SDialog dialog(this);
    dialog.setWindowTitle(tr("文字样式管理器"));
    dialog.resize(560, 380);
    auto* layout = new QVBoxLayout(&dialog);
    auto* current_label = new QLabel(&dialog);
    layout->addWidget(current_label);
    auto* style_combo = new QComboBox(&dialog);
    layout->addWidget(style_combo);
    auto* form = new QFormLayout();
    auto* name_edit = new QLineEdit(&dialog);
    auto* font_combo = new QFontComboBox(&dialog);
    auto* height_box = new QDoubleSpinBox(&dialog);
    auto* width_box = new QDoubleSpinBox(&dialog);
    auto* oblique_box = new QDoubleSpinBox(&dialog);
    auto* bold_check = new QCheckBox(tr("粗体"), &dialog);
    auto* italic_check = new QCheckBox(tr("斜体"), &dialog);
    height_box->setRange(0.0, 1.0e9);
    height_box->setDecimals(3);
    width_box->setRange(0.01, 100.0);
    width_box->setDecimals(3);
    oblique_box->setRange(-84.9, 84.9);
    oblique_box->setDecimals(1);
    form->addRow(tr("样式名称："), name_edit);
    form->addRow(tr("字体："), font_combo);
    form->addRow(tr("固定高度（0=可变）："), height_box);
    form->addRow(tr("宽度因子："), width_box);
    form->addRow(tr("倾斜角："), oblique_box);
    auto* flags_layout = new QHBoxLayout();
    flags_layout->addWidget(bold_check);
    flags_layout->addWidget(italic_check);
    flags_layout->addStretch();
    form->addRow(tr("字体效果："), flags_layout);
    layout->addLayout(form);

    const auto load_style = [this, style_combo, name_edit, font_combo, height_box, width_box,
                             oblique_box, bold_check, italic_check, current_label]()
    {
        const STextStyleRecord* style = m_document->textStyle(style_combo->currentText());
        if (!style)
        {
            return;
        }
        name_edit->setText(style->name);
        font_combo->setCurrentFont(QFont(style->font_family));
        height_box->setValue(style->fixed_height);
        width_box->setValue(style->width_factor);
        oblique_box->setValue(style->oblique_angle);
        bold_check->setChecked(style->is_bold);
        italic_check->setChecked(style->is_italic);
        current_label->setText(tr("当前文字样式：%1").arg(m_document->currentTextStyleName()));
    };
    const auto refresh = [this, style_combo, load_style]()
    {
        const QString selected_name = style_combo->currentText();
        style_combo->clear();
        for (const STextStyleRecord& style : m_document->textStyles())
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
            [this, name_edit, font_combo, height_box, width_box, oblique_box, bold_check,
             italic_check, refresh]()
            {
                STextStyleRecord style;
                style.name = name_edit->text();
                style.font_family = font_combo->currentFont().family();
                style.fixed_height = height_box->value();
                style.width_factor = width_box->value();
                style.oblique_angle = oblique_box->value();
                style.is_bold = bold_check->isChecked();
                style.is_italic = italic_check->isChecked();
                if (!m_document->addOrUpdateTextStyle(style))
                {
                    SDialogService::warning(this, tr("文字样式"), tr("样式参数无效。"));
                    return;
                }
                refresh();
            });
    connect(current_button, &QPushButton::clicked, &dialog,
            [this, style_combo, refresh]()
            {
                if (m_document->setCurrentTextStyle(style_combo->currentText()))
                {
                    refresh();
                }
            });
    connect(delete_button, &QPushButton::clicked, &dialog,
            [this, style_combo, refresh]()
            {
                if (!m_document->removeTextStyle(style_combo->currentText()))
                {
                    SDialogService::warning(
                        this, tr("文字样式"),
                        tr("Standard、当前样式或已被实体引用的样式不能删除。"));
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

bool SCadMainWindow::executeTextStyleCommand(const QString& command)
{
    const QStringList parts = command.split(QLatin1Char(' '), QString::SkipEmptyParts);
    if (parts.isEmpty() || parts.front().compare(QLatin1String("STYLE"), Qt::CaseInsensitive) != 0)
    {
        return false;
    }
    if (parts.size() == 1)
    {
        showTextStyleManager();
        return true;
    }
    if (parts.size() == 3 && parts[1].compare(QLatin1String("SET"), Qt::CaseInsensitive) == 0)
    {
        const bool is_success = m_document->setCurrentTextStyle(parts[2]);
        m_command_line->appendMessage(is_success ? tr("当前文字样式：%1").arg(parts[2])
                                                 : tr("STYLE SET 失败：样式不存在。"));
        return true;
    }
    if (parts.size() == 3 && parts[1].compare(QLatin1String("DELETE"), Qt::CaseInsensitive) == 0)
    {
        m_command_line->appendMessage(m_document->removeTextStyle(parts[2])
                                          ? tr("文字样式已删除：%1").arg(parts[2])
                                          : tr("STYLE DELETE 失败：样式受保护或正在使用。"));
        return true;
    }
    if (parts.size() >= 5 && parts[1].compare(QLatin1String("ADD"), Qt::CaseInsensitive) == 0)
    {
        bool is_height_valid = false;
        bool is_width_valid = parts.size() < 6;
        STextStyleRecord style;
        style.name = parts[2];
        style.font_family = QString(parts[3]).replace(QLatin1Char('_'), QLatin1Char(' '));
        style.fixed_height = parts[4].toDouble(&is_height_valid);
        style.width_factor = parts.size() >= 6 ? parts[5].toDouble(&is_width_valid) : 1.0;
        const bool is_success =
            is_height_valid && is_width_valid && m_document->addOrUpdateTextStyle(style);
        m_command_line->appendMessage(
            is_success ? tr("文字样式已添加或更新：%1").arg(style.name)
                       : tr("STYLE ADD 用法：STYLE ADD 名称 字体族 固定高度 [宽度因子]。"));
        return true;
    }
    m_command_line->appendMessage(
        tr("STYLE 用法：STYLE、STYLE SET 名称、STYLE DELETE 名称或 STYLE ADD ..."));
    return true;
}

} // namespace smartGraphics
