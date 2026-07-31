#include "s_command_line_widget.h"

#include "s_command_catalog.h"

#include <QCompleter>
#include <QEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QVBoxLayout>
#include <algorithm>

namespace vectorPath
{

SCommandLineWidget::SCommandLineWidget(QWidget* parent) : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    setObjectName(QStringLiteral("smartCommandLine"));
    layout->setContentsMargins(6, 2, 6, 3);
    layout->setSpacing(1);

    m_history = new QTextEdit(this);
    m_history->setReadOnly(true);
    m_history->setObjectName(QStringLiteral("smartCommandHistory"));
    m_history->setMinimumHeight(34);
    m_history->setAcceptRichText(false);
    m_history->setFrameShape(QFrame::NoFrame);
    m_history->setText(tr("vectorPath 已就绪。输入 LINE、CIRCLE、UNDO 或 ZOOM EXTENTS。"));

    m_input = new QLineEdit(this);
    m_input->setObjectName(QStringLiteral("smartCommandInput"));
    m_input->setPlaceholderText(tr("输入命令或按 Ctrl+K 搜索…"));
    m_input->setAccessibleName(tr("命令输入"));
    m_input->installEventFilter(this);
    m_completer = new QCompleter(commandCompletionEntries(), this);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->setFilterMode(Qt::MatchStartsWith);
    m_completer->setMaxVisibleItems(10);
    m_input->setCompleter(m_completer);
    connect(m_input, &QLineEdit::textChanged, this, &SCommandLineWidget::commandInputChanged);
    connect(m_input, &QLineEdit::returnPressed, this,
            [this]()
            {
                const QString command = m_input->text().trimmed();
                if (command.isEmpty())
                {
                    return;
                }
                if (m_submitted_commands.isEmpty() || m_submitted_commands.back() != command)
                {
                    m_submitted_commands.push_back(command);
                }
                m_history_index = m_submitted_commands.size();
                appendMessage(QStringLiteral("> %1").arg(command));
                m_input->clear();
                emit commandSubmitted(command);
            });

    layout->addWidget(m_history, 1);
    layout->addWidget(m_input);
}

bool SCommandLineWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (watched != m_input || event->type() != QEvent::KeyPress)
    {
        return QWidget::eventFilter(watched, event);
    }

    auto* key_event = static_cast<QKeyEvent*>(event);
    if (key_event->key() == Qt::Key_Tab || key_event->key() == Qt::Key_Backtab)
    {
        const QStringList matches = commandCompletionsForPrefix(m_input->text());
        if (!matches.isEmpty())
        {
            m_input->setText(matches.front());
            m_input->setCursorPosition(m_input->text().size());
        }
        return true;
    }
    if (key_event->key() == Qt::Key_Up && !m_submitted_commands.isEmpty())
    {
        m_history_index = std::max(0, m_history_index - 1);
        m_input->setText(m_submitted_commands[m_history_index]);
        return true;
    }
    if (key_event->key() == Qt::Key_Down && !m_submitted_commands.isEmpty())
    {
        m_history_index = std::min(m_submitted_commands.size(), m_history_index + 1);
        m_input->setText(m_history_index < m_submitted_commands.size()
                             ? m_submitted_commands[m_history_index]
                             : QString());
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void SCommandLineWidget::appendMessage(const QString& message)
{
    m_history->append(message.toHtmlEscaped());
}

void SCommandLineWidget::focusInput()
{
    m_input->setFocus();
    m_input->selectAll();
}

} // namespace vectorPath
