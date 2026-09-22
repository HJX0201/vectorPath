#include "vp_selection_context_bar.h"

#include <QAction>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QScreen>
#include <QToolButton>

namespace Vp
{

VpSelectionContextBar::VpSelectionContextBar(QWidget* parent)
    : QFrame(parent, Qt::Popup | Qt::FramelessWindowHint)
{
    setObjectName(QStringLiteral("smartSelectionContextBar"));
    setFrameShape(QFrame::StyledPanel);
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(5, 5, 5, 5);
    m_layout->setSpacing(3);
}

void VpSelectionContextBar::addAction(QAction* action)
{
    auto* button = new QToolButton(this);
    button->setDefaultAction(action);
    button->setAutoRaise(true);
    button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    button->setIconSize(QSize(20, 20));
    m_layout->addWidget(button);
    connect(action, &QAction::triggered, this, &QWidget::hide);
}

void VpSelectionContextBar::addSeparator()
{
    auto* separator = new QFrame(this);
    separator->setFrameShape(QFrame::VLine);
    separator->setFrameShadow(QFrame::Sunken);
    m_layout->addWidget(separator);
}

void VpSelectionContextBar::popupAt(const QPoint& global_position)
{
    adjustSize();
    QScreen* screen = QGuiApplication::screenAt(global_position);
    const QRect available = screen ? screen->availableGeometry() : QRect(global_position, size());
    QPoint position = global_position + QPoint(8, 8);
    position.setX(std::min(position.x(), available.right() - width()));
    position.setY(std::min(position.y(), available.bottom() - height()));
    position.setX(std::max(position.x(), available.left()));
    position.setY(std::max(position.y(), available.top()));
    move(position);
    show();
    raise();
    activateWindow();
}

} // namespace Vp
