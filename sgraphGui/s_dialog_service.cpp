#include "s_dialog_service.h"

#include <QAbstractButton>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

namespace vectorPath
{
namespace
{

constexpr int kStandardDialogMinimumWidth = 520;
constexpr int kStandardDialogMinimumHeight = 320;
constexpr int kInputDialogMinimumHeight = 220;
constexpr int kMessageDialogMinimumHeight = 240;

void translateStandardButtons(QDialogButtonBox& button_box)
{
    const std::pair<QDialogButtonBox::StandardButton, QString> translations[]{
        {QDialogButtonBox::Ok, QObject::tr("确定")},
        {QDialogButtonBox::Cancel, QObject::tr("取消")},
        {QDialogButtonBox::Save, QObject::tr("保存")},
        {QDialogButtonBox::Discard, QObject::tr("不保存")},
        {QDialogButtonBox::Close, QObject::tr("关闭")},
        {QDialogButtonBox::Yes, QObject::tr("是")},
        {QDialogButtonBox::No, QObject::tr("否")},
        {QDialogButtonBox::Apply, QObject::tr("应用")},
        {QDialogButtonBox::Reset, QObject::tr("重置")},
        {QDialogButtonBox::RestoreDefaults, QObject::tr("恢复默认")},
    };
    for (const auto& translation : translations)
    {
        if (QPushButton* button = button_box.button(translation.first))
        {
            button->setText(translation.second);
        }
    }
}

void addInputDialogButtons(QDialog& dialog, QVBoxLayout& layout)
{
    layout.addStretch();
    auto* buttons =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    translateStandardButtons(*buttons);
    layout.addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
}

QStyle::StandardPixmap messageIconPixmap(QMessageBox::Icon icon)
{
    switch (icon)
    {
    case QMessageBox::Information:
        return QStyle::SP_MessageBoxInformation;
    case QMessageBox::Warning:
        return QStyle::SP_MessageBoxWarning;
    case QMessageBox::Critical:
        return QStyle::SP_MessageBoxCritical;
    case QMessageBox::Question:
        return QStyle::SP_MessageBoxQuestion;
    case QMessageBox::NoIcon:
        break;
    }
    return QStyle::SP_CustomBase;
}

} // namespace

SDialog::SDialog(QWidget* parent) : QDialog(parent)
{
    setMinimumSize(kStandardDialogMinimumWidth, kStandardDialogMinimumHeight);
}

QString SDialogService::getText(QWidget* parent, const QString& title, const QString& label,
                                QLineEdit::EchoMode echo_mode, const QString& initial_text,
                                bool* is_accepted)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.setMinimumSize(kStandardDialogMinimumWidth, kInputDialogMinimumHeight);
    QVBoxLayout layout(&dialog);
    auto* label_widget = new QLabel(label, &dialog);
    label_widget->setWordWrap(true);
    layout.addWidget(label_widget);
    auto* text_edit = new QLineEdit(initial_text, &dialog);
    text_edit->setObjectName(QStringLiteral("s_dialog_text_input"));
    text_edit->setEchoMode(echo_mode);
    text_edit->selectAll();
    layout.addWidget(text_edit);
    addInputDialogButtons(dialog, layout);

    const bool accepted = dialog.exec() == QDialog::Accepted;
    if (is_accepted)
    {
        *is_accepted = accepted;
    }
    return accepted ? text_edit->text() : QString{};
}

QString SDialogService::getItem(QWidget* parent, const QString& title, const QString& label,
                                const QStringList& items, int current_index, bool is_editable,
                                bool* is_accepted)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.setMinimumSize(kStandardDialogMinimumWidth, kInputDialogMinimumHeight);
    QVBoxLayout layout(&dialog);
    auto* label_widget = new QLabel(label, &dialog);
    label_widget->setWordWrap(true);
    layout.addWidget(label_widget);
    auto* item_combo = new QComboBox(&dialog);
    item_combo->setObjectName(QStringLiteral("s_dialog_item_input"));
    item_combo->setEditable(is_editable);
    item_combo->addItems(items);
    if (current_index >= 0 && current_index < items.size())
    {
        item_combo->setCurrentIndex(current_index);
    }
    layout.addWidget(item_combo);
    addInputDialogButtons(dialog, layout);

    const bool accepted = dialog.exec() == QDialog::Accepted;
    if (is_accepted)
    {
        *is_accepted = accepted;
    }
    return accepted ? item_combo->currentText() : QString{};
}

QMessageBox::StandardButton SDialogService::information(
    QWidget* parent, const QString& title, const QString& text,
    QMessageBox::StandardButtons buttons, QMessageBox::StandardButton default_button)
{
    return showMessage(QMessageBox::Information, parent, title, text, buttons, default_button);
}

QMessageBox::StandardButton SDialogService::warning(
    QWidget* parent, const QString& title, const QString& text,
    QMessageBox::StandardButtons buttons, QMessageBox::StandardButton default_button)
{
    return showMessage(QMessageBox::Warning, parent, title, text, buttons, default_button);
}

QMessageBox::StandardButton SDialogService::critical(
    QWidget* parent, const QString& title, const QString& text,
    QMessageBox::StandardButtons buttons, QMessageBox::StandardButton default_button)
{
    return showMessage(QMessageBox::Critical, parent, title, text, buttons, default_button);
}

QMessageBox::StandardButton SDialogService::question(
    QWidget* parent, const QString& title, const QString& text,
    QMessageBox::StandardButtons buttons, QMessageBox::StandardButton default_button)
{
    return showMessage(QMessageBox::Question, parent, title, text, buttons, default_button);
}

QMessageBox::StandardButton SDialogService::showMessage(
    QMessageBox::Icon icon, QWidget* parent, const QString& title, const QString& text,
    QMessageBox::StandardButtons buttons, QMessageBox::StandardButton default_button)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.setMinimumSize(kStandardDialogMinimumWidth, kMessageDialogMinimumHeight);

    QVBoxLayout dialog_layout(&dialog);
    auto* message_layout = new QHBoxLayout();
    if (icon != QMessageBox::NoIcon)
    {
        auto* icon_label = new QLabel(&dialog);
        const QIcon message_icon = dialog.style()->standardIcon(messageIconPixmap(icon));
        icon_label->setPixmap(message_icon.pixmap(32, 32));
        icon_label->setAlignment(Qt::AlignTop);
        message_layout->addWidget(icon_label);
    }
    auto* message_label = new QLabel(text, &dialog);
    message_label->setWordWrap(true);
    message_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    message_layout->addWidget(message_label, 1);
    dialog_layout.addLayout(message_layout);
    dialog_layout.addStretch();

    const auto dialog_buttons = static_cast<QDialogButtonBox::StandardButtons>(
        static_cast<int>(buttons));
    auto* button_box = new QDialogButtonBox(dialog_buttons, &dialog);
    translateStandardButtons(*button_box);
    dialog_layout.addWidget(button_box);

    if (default_button != QMessageBox::NoButton)
    {
        const auto dialog_default_button =
            static_cast<QDialogButtonBox::StandardButton>(default_button);
        QPushButton* default_push_button = button_box->button(dialog_default_button);
        if (default_push_button)
        {
            default_push_button->setDefault(true);
        }
    }

    QMessageBox::StandardButton selected_button = QMessageBox::NoButton;
    QObject::connect(button_box, &QDialogButtonBox::clicked, &dialog,
                     [&](QAbstractButton* button)
    {
        selected_button = static_cast<QMessageBox::StandardButton>(
            button_box->standardButton(button));
        dialog.accept();
    });
    dialog.exec();
    return selected_button;
}

} // namespace vectorPath
