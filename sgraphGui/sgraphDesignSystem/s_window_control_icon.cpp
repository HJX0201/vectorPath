#include "s_icon_provider.h"

#include <QPainter>
#include <QPainterPath>
#include <QPixmap>

namespace smartGraphics
{

QIcon SIconProvider::createWindowControlIcon(SWindowControlIconType icon_type,
                                             const QColor& foreground)
{
    QPixmap pixmap(64, 64);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(foreground, 3.0, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
    painter.setBrush(Qt::NoBrush);

    switch (icon_type)
    {
    case SWindowControlIconType::Minimize:
        painter.drawLine(QPointF(14.0, 34.0), QPointF(50.0, 34.0));
        break;
    case SWindowControlIconType::Maximize:
        painter.drawRect(QRectF(15.0, 15.0, 34.0, 34.0));
        break;
    case SWindowControlIconType::Restore:
    {
        QPainterPath back_window;
        back_window.moveTo(26.0, 22.0);
        back_window.lineTo(26.0, 14.0);
        back_window.lineTo(50.0, 14.0);
        back_window.lineTo(50.0, 38.0);
        back_window.lineTo(42.0, 38.0);
        painter.drawPath(back_window);
        painter.drawRect(QRectF(14.0, 22.0, 28.0, 28.0));
        break;
    }
    case SWindowControlIconType::Close:
        painter.drawLine(QPointF(14.0, 14.0), QPointF(50.0, 50.0));
        painter.drawLine(QPointF(50.0, 14.0), QPointF(14.0, 50.0));
        break;
    }

    return QIcon(pixmap);
}

} // namespace smartGraphics
