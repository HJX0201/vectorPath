#pragma once

#include <QStringList>
#include <QWidget>

class QCompleter;
class QEvent;
class QLineEdit;
class QTextEdit;

namespace Vp
{

class VpCommandLineWidget final : public QWidget
{
    Q_OBJECT

  public:
    explicit VpCommandLineWidget(QWidget* parent = nullptr);

    void appendMessage(const QString& message);
    void focusInput();

  signals:
    void commandSubmitted(const QString& command);
    void commandInputChanged(const QString& command);

  protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

  private:
    QTextEdit* m_history = nullptr;
    QLineEdit* m_input = nullptr;
    QCompleter* m_completer = nullptr;
    QStringList m_submitted_commands;
    int m_history_index = 0;
};

} // namespace Vp
