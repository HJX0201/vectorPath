#include "s_icon_provider.h"

#include <QPainter>
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
    painter.setPen(QPen(foreground, 4.0, Qt::SolidLine, Qt::RoundCap, Qt::MiterJoin));
    painter.setBrush(Qt::NoBrush);

    switch (icon_type)
    {
    case SWindowControlIconType::Minimize:
        painter.drawLine(QPointF(15.0, 45.0), QPointF(49.0, 45.0));
        break;
    case SWindowControlIconType::Maximize:
        painter.drawRect(QRectF(15.0, 15.0, 34.0, 34.0));
        break;
    case SWindowControlIconType::Restore:
        painter.drawRect(QRectF(22.0, 14.0, 28.0, 28.0));
        painter.setBrush(QColor(0, 0, 0, 0));
        painter.drawRect(QRectF(14.0, 22.0, 28.0, 28.0));
        break;
    case SWindowControlIconType::Close:
        painter.drawLine(QPointF(16.0, 16.0), QPointF(48.0, 48.0));
        painter.drawLine(QPointF(48.0, 16.0), QPointF(16.0, 48.0));
        break;
    }

    return QIcon(pixmap);
}

} // namespace smartGraphics
