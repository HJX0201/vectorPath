#include "s_output_icon.h"

#include "s_icon_provider.h"

#include <QPainter>

namespace vectorPath
{

void drawOutputIcon(QPainter& painter, SIconType icon_type, const QColor& accent_color)
{
    if (icon_type == SIconType::PageSetup)
    {
        painter.drawRect(QRectF(14, 8, 36, 48));
        painter.setPen(QPen(accent_color, 3.0));
        painter.drawLine(QPointF(21, 20), QPointF(43, 20));
        painter.drawLine(QPointF(21, 29), QPointF(43, 29));
        painter.drawLine(QPointF(21, 38), QPointF(36, 38));
    }
    else if (icon_type == SIconType::PlotStyle)
    {
        painter.drawRoundedRect(QRectF(9, 10, 46, 44), 4, 4);
        painter.setPen(QPen(QColor(224, 72, 72), 4.0));
        painter.drawLine(QPointF(17, 21), QPointF(47, 21));
        painter.setPen(QPen(QColor(72, 164, 224), 4.0));
        painter.drawLine(QPointF(17, 32), QPointF(42, 32));
        painter.setPen(QPen(accent_color, 4.0));
        painter.drawLine(QPointF(17, 43), QPointF(50, 43));
    }
    else if (icon_type == SIconType::PlotPreview)
    {
        painter.drawRect(QRectF(9, 12, 34, 42));
        painter.drawEllipse(QRectF(34, 33, 20, 20));
        painter.setPen(QPen(accent_color, 4.0));
        painter.drawLine(QPointF(49, 49), QPointF(57, 57));
    }
    else if (icon_type == SIconType::Print)
    {
        painter.drawRect(QRectF(16, 8, 32, 16));
        painter.drawRoundedRect(QRectF(8, 21, 48, 27), 4, 4);
        painter.setPen(QPen(accent_color, 4.0));
        painter.drawRect(QRectF(16, 38, 32, 19));
    }
    else if (icon_type == SIconType::ExportPdf)
    {
        painter.drawRect(QRectF(10, 7, 35, 50));
        painter.setPen(QPen(accent_color, 4.0));
        painter.drawLine(QPointF(32, 25), QPointF(54, 25));
        painter.drawLine(QPointF(54, 25), QPointF(46, 17));
        painter.drawLine(QPointF(54, 25), QPointF(46, 33));
        painter.drawText(QRectF(14, 36, 28, 16), Qt::AlignCenter, QStringLiteral("PDF"));
    }
    else
    {
        painter.drawRect(QRectF(8, 11, 28, 38));
        painter.drawRect(QRectF(19, 7, 28, 38));
        painter.drawRect(QRectF(30, 15, 26, 38));
        painter.setPen(QPen(accent_color, 4.0));
        painter.drawLine(QPointF(19, 56), QPointF(47, 56));
    }
}

} // namespace vectorPath
