#include "s_import_icon.h"

#include <QPainter>
#include <QPainterPath>

namespace vectorPath
{

bool drawImportIcon(QPainter& painter, SIconType icon_type,
                    const QColor& foreground, const QColor& accent)
{
    if (icon_type != SIconType::ImportSvg && icon_type != SIconType::ImportBitmap &&
        icon_type != SIconType::SvgFill && icon_type != SIconType::SvgDeduplicate)
    {
        return false;
    }
    QPainterPath file;
    file.moveTo(12, 7);
    file.lineTo(40, 7);
    file.lineTo(53, 20);
    file.lineTo(53, 57);
    file.lineTo(12, 57);
    file.closeSubpath();
    painter.setPen(QPen(foreground, 3.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(file);
    painter.drawLine(QPointF(40, 7), QPointF(40, 20));
    painter.drawLine(QPointF(40, 20), QPointF(53, 20));
    painter.setPen(QPen(accent, 3.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    if (icon_type == SIconType::ImportSvg)
    {
        painter.drawLine(QPointF(18, 35), QPointF(24, 29));
        painter.drawLine(QPointF(18, 35), QPointF(24, 41));
        painter.drawLine(QPointF(47, 35), QPointF(41, 29));
        painter.drawLine(QPointF(47, 35), QPointF(41, 41));
        painter.drawLine(QPointF(36, 27), QPointF(29, 43));
    }
    else if (icon_type == SIconType::ImportBitmap)
    {
        painter.drawRect(QRectF(18, 27, 25, 20));
        painter.drawLine(QPointF(26, 27), QPointF(26, 47));
        painter.drawLine(QPointF(35, 27), QPointF(35, 47));
        painter.drawLine(QPointF(18, 34), QPointF(43, 34));
        painter.drawLine(QPointF(18, 41), QPointF(43, 41));
        painter.setBrush(accent);
        painter.drawRect(QRectF(27, 35, 7, 6));
    }
    else if (icon_type == SIconType::SvgFill)
    {
        painter.drawRect(QRectF(18, 27, 28, 22));
        for (int y = 31; y <= 45; y += 5)
        {
            painter.drawLine(QPointF(20, y), QPointF(44, y));
        }
    }
    else
    {
        painter.drawRect(QRectF(18, 27, 20, 18));
        painter.drawRect(QRectF(28, 35, 20, 18));
        painter.setPen(QPen(foreground, 4.0, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(29, 35), QPointF(38, 35));
        painter.drawLine(QPointF(38, 35), QPointF(38, 44));
    }
    return true;
}

} // namespace vectorPath
