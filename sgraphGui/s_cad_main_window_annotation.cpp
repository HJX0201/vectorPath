#include "s_cad_main_window.h"
#include "s_cad_viewport.h"
#include "s_cad_workspace_widget.h"
#include "s_dialog_service.h"
#include "s_icon_provider.h"

#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTextCharFormat>
#include <QTextEdit>
#include <QToolButton>
#include <QVBoxLayout>
#include <SARibbonPanel.h>

namespace smartCam
{

void SCadMainWindow::configureAnnotationPanel(SARibbonPanel* annotation_panel)
{
    QAction* mtext_action = createAction(tr("多行文字"), SIconType::MText);
    QAction* leader_action = createAction(tr("多重引线"), SIconType::Leader);
    QAction* style_action = createAction(tr("文字样式"), SIconType::TextStyle);
    registerShortcutAction(mtext_action, QStringLiteral("annotation.mtext"));
    registerShortcutAction(leader_action, QStringLiteral("annotation.leader"));
    registerShortcutAction(style_action, QStringLiteral("annotation.text_style"));
    connect(mtext_action, &QAction::triggered, this,
            [this]()
            {
                beginMTextCommand();
            });
    connect(leader_action, &QAction::triggered, this,
            [this]()
            {
                beginLeaderCommand();
            });
    connect(style_action, &QAction::triggered, this, &SCadMainWindow::showTextStyleManager);
    annotation_panel->addLargeAction(mtext_action);
    annotation_panel->addLargeAction(leader_action);
    annotation_panel->addLargeAction(style_action);
}

void SCadMainWindow::beginMTextCommand(const QString& initial_text)
{
    SDialog dialog(this);
    dialog.setWindowTitle(tr("创建多行文字"));
    dialog.resize(620, 430);
    auto* layout = new QVBoxLayout(&dialog);
    auto* format_layout = new QHBoxLayout();
    auto* bold_button = new QToolButton(&dialog);
    auto* italic_button = new QToolButton(&dialog);
    auto* underline_button = new QToolButton(&dialog);
    bold_button->setText(QStringLiteral("B"));
    italic_button->setText(QStringLiteral("I"));
    underline_button->setText(QStringLiteral("U"));
    bold_button->setCheckable(true);
    italic_button->setCheckable(true);
    underline_button->setCheckable(true);
    format_layout->addWidget(bold_button);
    format_layout->addWidget(italic_button);
    format_layout->addWidget(underline_button);
    format_layout->addStretch();
    layout->addLayout(format_layout);
    auto* editor = new QTextEdit(&dialog);
    editor->setAcceptRichText(true);
    editor->setPlaceholderText(tr("输入多行文字；可使用粗体、斜体和下划线。"));
    editor->setPlainText(initial_text);
    layout->addWidget(editor, 1);

    const auto merge_format = [editor](const QTextCharFormat& format)
    {
        QTextCursor cursor = editor->textCursor();
        if (!cursor.hasSelection())
        {
            cursor.select(QTextCursor::WordUnderCursor);
        }
        cursor.mergeCharFormat(format);
        editor->mergeCurrentCharFormat(format);
    };
    connect(bold_button, &QToolButton::toggled, &dialog,
            [merge_format](bool is_checked)
            {
                QTextCharFormat format;
                format.setFontWeight(is_checked ? QFont::Bold : QFont::Normal);
                merge_format(format);
            });
    connect(italic_button, &QToolButton::toggled, &dialog,
            [merge_format](bool is_checked)
            {
                QTextCharFormat format;
                format.setFontItalic(is_checked);
                merge_format(format);
            });
    connect(underline_button, &QToolButton::toggled, &dialog,
            [merge_format](bool is_checked)
            {
                QTextCharFormat format;
                format.setFontUnderline(is_checked);
                merge_format(format);
            });

    auto* options_layout = new QFormLayout();
    auto* width_box = new QDoubleSpinBox(&dialog);
    auto* height_box = new QDoubleSpinBox(&dialog);
    auto* alignment_combo = new QComboBox(&dialog);
    width_box->setRange(0.01, 1.0e9);
    width_box->setDecimals(3);
    width_box->setValue(40.0);
    height_box->setRange(0.01, 1.0e9);
    height_box->setDecimals(3);
    height_box->setValue(2.5);
    alignment_combo->addItem(tr("左对齐"), static_cast<int>(STextHorizontalAlignment::Left));
    alignment_combo->addItem(tr("居中"), static_cast<int>(STextHorizontalAlignment::Center));
    alignment_combo->addItem(tr("右对齐"), static_cast<int>(STextHorizontalAlignment::Right));
    alignment_combo->addItem(tr("两端对齐"), static_cast<int>(STextHorizontalAlignment::Justified));
    options_layout->addRow(tr("边界宽度："), width_box);
    options_layout->addRow(tr("文字高度："), height_box);
    options_layout->addRow(tr("对齐："), alignment_combo);
    layout->addLayout(options_layout);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    if (dialog.exec() != QDialog::Accepted || editor->toPlainText().trimmed().isEmpty())
    {
        return;
    }
    m_workspace->viewport()->setPendingMText(
        editor->toHtml(), width_box->value(), height_box->value(),
        static_cast<STextHorizontalAlignment>(alignment_combo->currentData().toInt()));
    m_workspace->viewport()->setToolMode(SToolMode::MText);
}

void SCadMainWindow::beginLeaderCommand(const QString& initial_text)
{
    SDialog dialog(this);
    dialog.setWindowTitle(tr("创建多重引线"));
    auto* layout = new QFormLayout(&dialog);
    auto* text_edit = new QLineEdit(initial_text, &dialog);
    auto* height_box = new QDoubleSpinBox(&dialog);
    auto* arrow_box = new QDoubleSpinBox(&dialog);
    height_box->setRange(0.01, 1.0e9);
    height_box->setValue(2.5);
    arrow_box->setRange(0.01, 1.0e9);
    arrow_box->setValue(2.5);
    layout->addRow(tr("文字："), text_edit);
    layout->addRow(tr("文字高度："), height_box);
    layout->addRow(tr("箭头大小："), arrow_box);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addRow(buttons);
    if (dialog.exec() != QDialog::Accepted || text_edit->text().trimmed().isEmpty())
    {
        return;
    }
    m_workspace->viewport()->setPendingLeader(text_edit->text(), height_box->value(),
                                              arrow_box->value());
    m_workspace->viewport()->setToolMode(SToolMode::Leader);
}

bool SCadMainWindow::executeAnnotationCommand(const QString& command)
{
    const QString simplified = command.simplified();
    const QString normalized = simplified.toUpper();
    if (normalized.startsWith(QLatin1String("TEXTALIGN ")))
    {
        const QString alignment_name = normalized.mid(10).trimmed();
        STextHorizontalAlignment alignment = STextHorizontalAlignment::Left;
        if (alignment_name == QLatin1String("CENTER") || alignment_name == QLatin1String("C"))
        {
            alignment = STextHorizontalAlignment::Center;
        }
        else if (alignment_name == QLatin1String("RIGHT") || alignment_name == QLatin1String("R"))
        {
            alignment = STextHorizontalAlignment::Right;
        }
        else if (alignment_name != QLatin1String("LEFT") && alignment_name != QLatin1String("L"))
        {
            return false;
        }
        m_workspace->viewport()->setPendingTextAlignment(alignment);
        return true;
    }
    if (normalized == QLatin1String("MTEXT") || normalized == QLatin1String("MT"))
    {
        beginMTextCommand();
        return true;
    }
    if (normalized.startsWith(QLatin1String("MTEXT ")) ||
        normalized.startsWith(QLatin1String("MT ")))
    {
        const int command_length = normalized.startsWith(QLatin1String("MTEXT ")) ? 5 : 2;
        beginMTextCommand(simplified.mid(command_length).trimmed());
        return true;
    }
    if (normalized == QLatin1String("MLEADER") || normalized == QLatin1String("ML"))
    {
        beginLeaderCommand();
        return true;
    }
    if (normalized.startsWith(QLatin1String("MLEADER ")) ||
        normalized.startsWith(QLatin1String("ML ")))
    {
        const int command_length = normalized.startsWith(QLatin1String("MLEADER ")) ? 7 : 2;
        beginLeaderCommand(simplified.mid(command_length).trimmed());
        return true;
    }
    return executeTextStyleCommand(simplified);
}

} // namespace smartCam
