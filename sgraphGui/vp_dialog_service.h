#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QStringList>

class QWidget;

namespace Vp
{

class VpDialog : public QDialog
{
  public:
    explicit VpDialog(QWidget* parent = nullptr);
};

class VpDialogService final
{
  public:
    static QString getText(QWidget* parent, const QString& title, const QString& label,
                           QLineEdit::EchoMode echo_mode = QLineEdit::Normal,
                           const QString& initial_text = {}, bool* is_accepted = nullptr);
    static QString getItem(QWidget* parent, const QString& title, const QString& label,
                           const QStringList& items, int current_index = 0, bool is_editable = true,
                           bool* is_accepted = nullptr);

    static QMessageBox::StandardButton
    information(QWidget* parent, const QString& title, const QString& text,
                QMessageBox::StandardButtons buttons = QMessageBox::Ok,
                QMessageBox::StandardButton default_button = QMessageBox::NoButton);
    static QMessageBox::StandardButton
    warning(QWidget* parent, const QString& title, const QString& text,
            QMessageBox::StandardButtons buttons = QMessageBox::Ok,
            QMessageBox::StandardButton default_button = QMessageBox::NoButton);
    static QMessageBox::StandardButton
    critical(QWidget* parent, const QString& title, const QString& text,
             QMessageBox::StandardButtons buttons = QMessageBox::Ok,
             QMessageBox::StandardButton default_button = QMessageBox::NoButton);
    static QMessageBox::StandardButton
    question(QWidget* parent, const QString& title, const QString& text,
             QMessageBox::StandardButtons buttons = QMessageBox::Yes | QMessageBox::No,
             QMessageBox::StandardButton default_button = QMessageBox::NoButton);

  private:
    static QMessageBox::StandardButton showMessage(QMessageBox::Icon icon, QWidget* parent,
                                                   const QString& title, const QString& text,
                                                   QMessageBox::StandardButtons buttons,
                                                   QMessageBox::StandardButton default_button);
};

} // namespace Vp
