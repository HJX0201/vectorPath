#pragma once

#include <QStringList>
#include <QWidget>

class QCompleter;
class QEvent;
class QLineEdit;
class QTextEdit;

namespace Vp
{

// 命令行输入框与消息显示区。
class VpCommandLineWidget final : public QWidget
{
    Q_OBJECT

  public:
    // 构造命令行控件。
    explicit VpCommandLineWidget(QWidget* parent = nullptr);

    // 追加一行消息。
    void appendMessage(const QString& message);
    // 把焦点移到输入框。
    void focusInput();

  signals:
    // 用户提交命令时发出。
    void commandSubmitted(const QString& command);
    // 命令输入变化时发出。
    void commandInputChanged(const QString& command);

  protected:
    // 拦截输入框按键，用于历史与补全。
    bool eventFilter(QObject* watched, QEvent* event) override;

  private:
    QTextEdit* m_history = nullptr;
    QLineEdit* m_input = nullptr;
    QCompleter* m_completer = nullptr;
    QStringList m_submitted_commands;
    int m_history_index = 0;
};

} // namespace Vp
