#include "s_hatch_icon.h"

#include <QLinearGradient>
#include <QPainter>

namespace vectorPath
{

void drawHatchIcon(QPainter& painter, SIconType icon_type, const QColor& accent_color)
{
    painter.save();
    const QRectF bounds(10, 10, 44, 44);
    painter.drawRect(bounds);
    painter.setClipRect(bounds.adjusted(2, 2, -2, -2));
    if (icon_type == SIconType::HatchGradient)
    {
        QLinearGradient gradient(bounds.topLeft(), bounds.bottomRight());
        gradient.setColorAt(0.0, accent_color);
        gradient.setColorAt(1.0, Qt::transparent);
        painter.fillRect(bounds, gradient);
    }
    else
    {
        painter.setPen(QPen(accent_color, 3.0));
        for (int offset = -28; offset < 88; offset += 10)
        {
            painter.drawLine(QPointF(offset, 58), QPointF(offset + 58, 0));
        }
        if (icon_type == SIconType::HatchPattern)
        {
            for (int offset = -28; offset < 88; offset += 16)
            {
                painter.drawLine(QPointF(offset, 0), QPointF(offset + 58, 58));
            }
        }
    }
    painter.setClipping(false);
    if (icon_type == SIconType::HatchEdit)
    {
        painter.setPen(QPen(accent_color, 4.0, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(38, 51), QPointF(55, 34));
        painter.drawLine(QPointF(42, 55), QPointF(59, 38));
    }
    painter.restore();
}

} // namespace vectorPath
