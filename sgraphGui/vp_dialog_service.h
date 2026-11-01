#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QStringList>

class QWidget;

namespace Vp
{

// 应用内对话框基类，统一对话框最小尺寸。
class VpDialog : public QDialog
{
  public:
    // 构造对话框并设置标准最小尺寸。
    explicit VpDialog(QWidget* parent = nullptr);
};

// 模态对话框的统一调用入口。
class VpDialogService final
{
  public:
    // 弹出单行文本输入框，返回输入内容。
    static QString getText(QWidget* parent, const QString& title, const QString& label,
                           QLineEdit::EchoMode echo_mode = QLineEdit::Normal,
                           const QString& initial_text = {}, bool* is_accepted = nullptr);
    // 弹出下拉选择框，返回选中项。
    static QString getItem(QWidget* parent, const QString& title, const QString& label,
                           const QStringList& items, int current_index = 0, bool is_editable = true,
                           bool* is_accepted = nullptr);

    // 弹出信息提示。
    static QMessageBox::StandardButton
    information(QWidget* parent, const QString& title, const QString& text,
                QMessageBox::StandardButtons buttons = QMessageBox::Ok,
                QMessageBox::StandardButton default_button = QMessageBox::NoButton);
    // 弹出警告提示。
    static QMessageBox::StandardButton
    warning(QWidget* parent, const QString& title, const QString& text,
            QMessageBox::StandardButtons buttons = QMessageBox::Ok,
            QMessageBox::StandardButton default_button = QMessageBox::NoButton);
    // 弹出错误提示。
    static QMessageBox::StandardButton
    critical(QWidget* parent, const QString& title, const QString& text,
             QMessageBox::StandardButtons buttons = QMessageBox::Ok,
             QMessageBox::StandardButton default_button = QMessageBox::NoButton);
    // 弹出询问对话框，返回用户选择。
    static QMessageBox::StandardButton
    question(QWidget* parent, const QString& title, const QString& text,
             QMessageBox::StandardButtons buttons = QMessageBox::Yes | QMessageBox::No,
             QMessageBox::StandardButton default_button = QMessageBox::NoButton);

  private:
    // 按图标类型弹出消息框。
    static QMessageBox::StandardButton showMessage(QMessageBox::Icon icon, QWidget* parent,
                                                   const QString& title, const QString& text,
                                                   QMessageBox::StandardButtons buttons,
                                                   QMessageBox::StandardButton default_button);
};

} // namespace Vp
