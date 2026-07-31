#include "s_icon_provider.h"

#include "s_file_icon.h"
#include "s_import_icon.h"
#include "s_specialized_icon.h"

#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPolygonF>
#include <cmath>

namespace vectorPath
{

QIcon SIconProvider::createIcon(SIconType icon_type, const QColor& foreground, const QColor& accent)
{
    const QColor accent_color = accent.isValid() ? accent : foreground;
    QPixmap pixmap(64, 64);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(foreground, 4.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    if (drawImportIcon(painter, icon_type, foreground, accent_color))
    {
        return QIcon(pixmap);
    }
    if (drawSpecializedIcon(painter, icon_type, foreground, accent_color))
    {
        return QIcon(pixmap);
    }

    switch (icon_type)
    {
    case SIconType::Application:
        painter.setPen(QPen(accent_color, 5.0, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(10, 47), QPointF(49, 9));
        painter.drawArc(QRectF(20, 18, 35, 35), 20 * 16, 270 * 16);
        painter.drawEllipse(QPointF(12, 49), 4, 4);
        break;
    case SIconType::NewFile:
        drawFileIcon(painter, true);
        break;
    case SIconType::OpenFile:
    {
        QPolygonF outline;
        outline << QPointF(8, 49) << QPointF(15, 25) << QPointF(31, 25) << QPointF(37, 32)
                << QPointF(56, 32) << QPointF(49, 52) << QPointF(8, 52);
        painter.drawPolyline(outline);
        break;
    }
    case SIconType::Save:
    case SIconType::SaveAs:
        painter.drawRoundedRect(QRectF(11, 8, 42, 48), 4, 4);
        painter.drawRect(QRectF(19, 9, 25, 17));
        painter.drawRect(QRectF(19, 37, 26, 18));
        if (icon_type == SIconType::SaveAs)
        {
            painter.setPen(QPen(accent_color, 4.0, Qt::SolidLine, Qt::RoundCap));
            painter.drawLine(QPointF(41, 31), QPointF(55, 31));
            drawArrowHead(painter, QPointF(55, 31), true);
        }
        break;
    case SIconType::ExportDxf:
    case SIconType::ExportDwg:
        drawFileIcon(painter, false);
        painter.setPen(QPen(accent_color, 4.0, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(25, 38), QPointF(49, 38));
        drawArrowHead(painter, QPointF(49, 38), true);
        break;
    case SIconType::Recovery:
        painter.drawArc(QRectF(12, 12, 40, 40), 35 * 16, 285 * 16);
        painter.setPen(QPen(accent_color, 4.0, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(13, 13), QPointF(13, 27));
        painter.drawLine(QPointF(13, 13), QPointF(27, 13));
        painter.drawLine(QPointF(32, 21), QPointF(32, 34));
        painter.drawLine(QPointF(32, 34), QPointF(42, 39));
        break;
    case SIconType::Audit:
    {
        QPainterPath shield(QPointF(32, 7));
        shield.lineTo(QPointF(53, 15));
        shield.lineTo(QPointF(49, 42));
        shield.quadTo(QPointF(43, 54), QPointF(32, 59));
        shield.quadTo(QPointF(21, 54), QPointF(15, 42));
        shield.lineTo(QPointF(11, 15));
        shield.closeSubpath();
        painter.drawPath(shield);
        painter.setPen(QPen(accent_color, 5.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(QPointF(21, 32), QPointF(29, 40));
        painter.drawLine(QPointF(29, 40), QPointF(44, 23));
        break;
    }
    case SIconType::DrawLine:
        painter.drawLine(QPointF(12, 51), QPointF(52, 12));
        painter.setBrush(accent_color);
        painter.drawEllipse(QPointF(12, 51), 4, 4);
        painter.drawEllipse(QPointF(52, 12), 4, 4);
        break;
    case SIconType::DrawCircle:
        painter.drawEllipse(QRectF(11, 11, 42, 42));
        painter.setPen(QPen(accent_color, 4.0));
        painter.drawPoint(QPointF(32, 32));
        break;
    case SIconType::DrawPolyline:
    {
        QPolygonF polyline;
        polyline << QPointF(8, 46) << QPointF(22, 17) << QPointF(39, 43) << QPointF(56, 13);
        painter.drawPolyline(polyline);
        painter.setBrush(accent_color);
        for (const QPointF& point : polyline)
        {
            painter.drawEllipse(point, 3, 3);
        }
        break;
    }
    case SIconType::PolylineEdit:
    {
        QPolygonF polyline;
        polyline << QPointF(7, 45) << QPointF(20, 18) << QPointF(35, 39) << QPointF(51, 13);
        painter.drawPolyline(polyline);
        painter.setBrush(accent_color);
        painter.drawEllipse(QPointF(20, 18), 4, 4);
        painter.drawEllipse(QPointF(35, 39), 4, 4);
        painter.setPen(QPen(accent_color, 4.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(QPointF(38, 53), QPointF(54, 37));
        painter.drawLine(QPointF(38, 53), QPointF(36, 57));
        break;
    }
    case SIconType::DrawEllipse:
        painter.save();
        painter.translate(32.0, 32.0);
        painter.rotate(-28.0);
        painter.drawEllipse(QRectF(-24.0, -14.0, 48.0, 28.0));
        painter.restore();
        painter.setBrush(accent_color);
        painter.drawEllipse(QPointF(32, 32), 3, 3);
        break;
    case SIconType::DrawSpline:
    {
        QPainterPath path(QPointF(7, 46));
        path.cubicTo(QPointF(19, 4), QPointF(43, 60), QPointF(57, 16));
        painter.drawPath(path);
        painter.setBrush(accent_color);
        painter.drawEllipse(QPointF(7, 46), 3, 3);
        painter.drawEllipse(QPointF(57, 16), 3, 3);
        break;
    }
    case SIconType::SplineEdit:
    {
        QPainterPath path(QPointF(7, 46));
        path.cubicTo(QPointF(19, 4), QPointF(43, 60), QPointF(57, 16));
        painter.drawPath(path);
        painter.setPen(QPen(accent_color, 1.5, Qt::DashLine));
        QPolygonF control_polygon;
        control_polygon << QPointF(7, 46) << QPointF(19, 4) << QPointF(43, 60) << QPointF(57, 16);
        painter.drawPolyline(control_polygon);
        painter.setBrush(accent_color);
        for (const QPointF& point : control_polygon)
        {
            painter.drawRect(QRectF(point.x() - 3.0, point.y() - 3.0, 6.0, 6.0));
        }
        break;
    }
    case SIconType::DrawArc:
        painter.drawArc(QRectF(10, 10, 44, 44), 15 * 16, 220 * 16);
        painter.setBrush(accent_color);
        painter.drawEllipse(QPointF(14, 46), 3, 3);
        painter.drawEllipse(QPointF(52, 25), 3, 3);
        break;
    case SIconType::DrawRectangle:
        painter.drawRect(QRectF(10, 15, 44, 34));
        painter.setBrush(accent_color);
        painter.drawEllipse(QPointF(10, 49), 3, 3);
        painter.drawEllipse(QPointF(54, 15), 3, 3);
        break;
    case SIconType::Select:
    {
        QPainterPath cursor;
        cursor.moveTo(12, 8);
        cursor.lineTo(46, 36);
        cursor.lineTo(31, 38);
        cursor.lineTo(40, 55);
        cursor.lineTo(31, 59);
        cursor.lineTo(23, 42);
        cursor.lineTo(12, 53);
        cursor.closeSubpath();
        painter.drawPath(cursor);
        break;
    }
    case SIconType::Move:
        painter.drawLine(QPointF(32, 8), QPointF(32, 56));
        painter.drawLine(QPointF(8, 32), QPointF(56, 32));
        drawArrowHead(painter, QPointF(56, 32), true);
        drawArrowHead(painter, QPointF(8, 32), false);
        painter.drawLine(QPointF(32, 8), QPointF(25, 17));
        painter.drawLine(QPointF(32, 8), QPointF(39, 17));
        painter.drawLine(QPointF(32, 56), QPointF(25, 47));
        painter.drawLine(QPointF(32, 56), QPointF(39, 47));
        break;
    case SIconType::Copy:
        painter.drawRoundedRect(QRectF(9, 17, 31, 35), 3, 3);
        painter.setPen(QPen(accent_color, 4.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawRoundedRect(QRectF(24, 9, 31, 35), 3, 3);
        break;
    case SIconType::Rotate:
        painter.drawArc(QRectF(12, 12, 40, 40), 35 * 16, 280 * 16);
        painter.drawLine(QPointF(51, 16), QPointF(51, 31));
        painter.drawLine(QPointF(51, 16), QPointF(36, 16));
        break;
    case SIconType::Scale:
        painter.drawRect(QRectF(18, 18, 28, 28));
        painter.setPen(QPen(accent_color, 4.0, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(18, 18), QPointF(8, 8));
        painter.drawLine(QPointF(8, 8), QPointF(8, 20));
        painter.drawLine(QPointF(8, 8), QPointF(20, 8));
        painter.drawLine(QPointF(46, 46), QPointF(56, 56));
        painter.drawLine(QPointF(56, 56), QPointF(44, 56));
        painter.drawLine(QPointF(56, 56), QPointF(56, 44));
        break;
    case SIconType::Mirror:
    {
        painter.setPen(QPen(accent_color, 3.0, Qt::DashLine));
        painter.drawLine(QPointF(32, 7), QPointF(32, 57));
        painter.setPen(QPen(foreground, 4.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        QPolygonF left_shape;
        left_shape << QPointF(10, 50) << QPointF(26, 14) << QPointF(26, 50);
        QPolygonF right_shape;
        right_shape << QPointF(54, 50) << QPointF(38, 14) << QPointF(38, 50);
        painter.drawPolygon(left_shape);
        painter.drawPolygon(right_shape);
        break;
    }
    case SIconType::Trim:
        painter.drawLine(QPointF(10, 47), QPointF(53, 14));
        painter.drawLine(QPointF(11, 16), QPointF(29, 30));
        painter.drawLine(QPointF(37, 37), QPointF(53, 50));
        painter.setPen(QPen(accent_color, 4.0));
        painter.drawLine(QPointF(30, 31), QPointF(36, 36));
        break;
    case SIconType::Extend:
        painter.drawLine(QPointF(49, 9), QPointF(49, 55));
        painter.drawLine(QPointF(10, 47), QPointF(32, 30));
        painter.setPen(QPen(accent_color, 4.0, Qt::DashLine, Qt::RoundCap));
        painter.drawLine(QPointF(32, 30), QPointF(49, 17));
        painter.setPen(QPen(accent_color, 4.0, Qt::SolidLine, Qt::RoundCap));
        drawArrowHead(painter, QPointF(47, 19), true);
        break;
    case SIconType::Break:
        painter.drawLine(QPointF(8, 32), QPointF(24, 32));
        painter.drawLine(QPointF(40, 32), QPointF(56, 32));
        painter.setPen(QPen(accent_color, 4.0, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(24, 22), QPointF(31, 32));
        painter.drawLine(QPointF(31, 32), QPointF(24, 42));
        painter.drawLine(QPointF(40, 22), QPointF(33, 32));
        painter.drawLine(QPointF(33, 32), QPointF(40, 42));
        break;
    case SIconType::Join:
        painter.drawLine(QPointF(8, 46), QPointF(25, 29));
        painter.drawLine(QPointF(25, 29), QPointF(39, 39));
        painter.drawLine(QPointF(39, 39), QPointF(56, 15));
        painter.setBrush(accent_color);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(25, 29), 4.5, 4.5);
        painter.drawEllipse(QPointF(39, 39), 4.5, 4.5);
        break;
    case SIconType::Explode:
        painter.drawLine(QPointF(25, 25), QPointF(11, 11));
        painter.drawLine(QPointF(39, 25), QPointF(53, 11));
        painter.drawLine(QPointF(25, 39), QPointF(11, 53));
        painter.drawLine(QPointF(39, 39), QPointF(53, 53));
        painter.setPen(QPen(accent_color, 4.0, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(32, 7), QPointF(32, 20));
        painter.drawLine(QPointF(32, 44), QPointF(32, 57));
        painter.drawLine(QPointF(7, 32), QPointF(20, 32));
        painter.drawLine(QPointF(44, 32), QPointF(57, 32));
        break;
    case SIconType::Stretch:
        painter.drawLine(QPointF(10, 45), QPointF(27, 45));
        painter.drawLine(QPointF(27, 45), QPointF(27, 19));
        painter.drawLine(QPointF(27, 19), QPointF(10, 19));
        painter.setPen(QPen(accent_color, 4.0, Qt::DashLine, Qt::RoundCap));
        painter.drawLine(QPointF(27, 19), QPointF(51, 11));
        painter.drawLine(QPointF(27, 45), QPointF(51, 53));
        painter.setPen(QPen(accent_color, 4.0, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(42, 32), QPointF(56, 32));
        drawArrowHead(painter, QPointF(56, 32), true);
        break;
    case SIconType::Lengthen:
        painter.drawLine(QPointF(8, 43), QPointF(34, 30));
        painter.setPen(QPen(accent_color, 4.0, Qt::DashLine, Qt::RoundCap));
        painter.drawLine(QPointF(34, 30), QPointF(55, 19));
        painter.setPen(QPen(accent_color, 4.0, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(39, 43), QPointF(56, 43));
        drawArrowHead(painter, QPointF(56, 43), true);
        painter.setBrush(accent_color);
        painter.drawEllipse(QPointF(34, 30), 3.5, 3.5);
        break;
    case SIconType::Grip:
        painter.drawPolyline(QPolygonF({QPointF(10, 48), QPointF(25, 18), QPointF(43, 43)}));
        painter.setPen(QPen(accent_color, 2.5));
        painter.setBrush(accent_color);
        painter.drawRect(QRectF(7, 45, 7, 7));
        painter.drawRect(QRectF(21.5, 14.5, 7, 7));
        painter.drawRect(QRectF(39.5, 39.5, 7, 7));
        painter.setPen(QPen(accent_color, 4.0, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(43, 43), QPointF(56, 30));
        drawArrowHead(painter, QPointF(56, 30), true);
        break;
    case SIconType::Fillet:
        painter.drawLine(QPointF(9, 53), QPointF(27, 53));
        painter.drawLine(QPointF(53, 9), QPointF(53, 27));
        painter.setPen(QPen(accent_color, 4.0, Qt::SolidLine, Qt::RoundCap));
        painter.drawArc(QRectF(27, 27, 26, 26), 90 * 16, 90 * 16);
        painter.setBrush(accent_color);
        painter.drawEllipse(QPointF(27, 53), 3.5, 3.5);
        painter.drawEllipse(QPointF(53, 27), 3.5, 3.5);
        break;
    case SIconType::Chamfer:
        painter.drawLine(QPointF(9, 53), QPointF(26, 53));
        painter.drawLine(QPointF(53, 9), QPointF(53, 26));
        painter.setPen(QPen(accent_color, 4.0, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(26, 53), QPointF(53, 26));
        painter.setBrush(accent_color);
        painter.drawEllipse(QPointF(26, 53), 3.5, 3.5);
        painter.drawEllipse(QPointF(53, 26), 3.5, 3.5);
        break;
    case SIconType::Blend:
    {
        QPainterPath path(QPointF(8, 48));
        path.cubicTo(QPointF(23, 48), QPointF(41, 16), QPointF(56, 16));
        painter.drawPath(path);
        painter.setPen(QPen(accent_color, 2.5, Qt::DashLine));
        painter.drawLine(QPointF(8, 48), QPointF(23, 48));
        painter.drawLine(QPointF(41, 16), QPointF(56, 16));
        painter.setBrush(accent_color);
        painter.drawEllipse(QPointF(8, 48), 3.5, 3.5);
        painter.drawEllipse(QPointF(56, 16), 3.5, 3.5);
        break;
    }
    case SIconType::Align:
        painter.drawLine(QPointF(9, 48), QPointF(31, 48));
        painter.drawLine(QPointF(33, 16), QPointF(55, 16));
        painter.setPen(QPen(accent_color, 3.5, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(20, 38), QPointF(44, 24));
        drawArrowHead(painter, QPointF(44, 24), true);
        painter.setBrush(accent_color);
        painter.drawEllipse(QPointF(20, 38), 3.5, 3.5);
        break;
    case SIconType::ArrayRect:
        for (int row = 0; row < 2; ++row)
        {
            for (int column = 0; column < 3; ++column)
            {
                painter.setPen(QPen(column == 0 && row == 0 ? foreground : accent_color, 3.0));
                painter.drawRect(QRectF(8 + (column * 19), 13 + (row * 25), 11, 13));
            }
        }
        break;
    case SIconType::ArrayPolar:
    {
        const QPointF center(32, 32);
        painter.setPen(QPen(accent_color, 3.0));
        painter.drawEllipse(center, 4.0, 4.0);
        for (int item = 0; item < 6; ++item)
        {
            const double angle = item * 3.14159265358979323846 / 3.0;
            const QPointF point(center.x() + (22.0 * std::cos(angle)),
                                center.y() + (22.0 * std::sin(angle)));
            painter.setPen(QPen(item == 0 ? foreground : accent_color, 3.0));
            painter.drawRect(QRectF(point.x() - 4.0, point.y() - 4.0, 8.0, 8.0));
        }
        break;
    }
    case SIconType::ArrayPath:
    {
        QPainterPath path;
        path.moveTo(7, 48);
        path.cubicTo(20, 10, 42, 55, 57, 15);
        painter.setPen(QPen(foreground, 3.0, Qt::DashLine));
        painter.drawPath(path);
        painter.setPen(QPen(accent_color, 3.0));
        painter.drawRect(QRectF(10, 36, 9, 9));
        painter.drawRect(QRectF(29, 28, 9, 9));
        painter.drawRect(QRectF(47, 13, 9, 9));
        break;
    }
    case SIconType::ArrayEdit:
        painter.setPen(QPen(foreground, 3.0));
        painter.drawRect(QRectF(9, 11, 12, 12));
        painter.drawRect(QRectF(27, 11, 12, 12));
        painter.drawRect(QRectF(9, 29, 12, 12));
        painter.setPen(QPen(accent_color, 5.0, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(31, 47), QPointF(51, 27));
        painter.setPen(QPen(foreground, 2.5));
        painter.drawLine(QPointF(47, 25), QPointF(53, 31));
        break;
    case SIconType::Offset:
        painter.drawPolyline(QPolygonF(QRectF(10, 16, 30, 31)));
        painter.setPen(QPen(accent_color, 4.0, Qt::DashLine));
        painter.drawPolyline(QPolygonF(QRectF(23, 9, 31, 31)));
        break;
    case SIconType::Undo:
    case SIconType::Redo:
    {
        const bool is_redo = icon_type == SIconType::Redo;
        painter.drawArc(QRectF(14, 16, 37, 35), is_redo ? -70 * 16 : 30 * 16, 220 * 16);
        const QPointF tip = is_redo ? QPointF(48, 17) : QPointF(16, 17);
        painter.drawLine(tip, tip + QPointF(0, 15));
        painter.drawLine(tip, tip + QPointF(is_redo ? -14 : 14, 0));
        break;
    }
    case SIconType::ZoomExtents:
        painter.drawEllipse(QRectF(10, 10, 33, 33));
        painter.drawLine(QPointF(39, 39), QPointF(55, 55));
        painter.setPen(QPen(accent_color, 3.0));
        painter.drawRect(QRectF(18, 18, 17, 17));
        break;
    case SIconType::Pan:
        painter.drawRoundedRect(QRectF(16, 25, 31, 30), 10, 10);
        painter.drawLine(QPointF(22, 31), QPointF(22, 15));
        painter.drawLine(QPointF(30, 27), QPointF(30, 10));
        painter.drawLine(QPointF(38, 28), QPointF(38, 14));
        break;
    case SIconType::Text:
        painter.drawLine(QPointF(13, 14), QPointF(51, 14));
        painter.drawLine(QPointF(32, 14), QPointF(32, 53));
        painter.drawLine(QPointF(22, 53), QPointF(42, 53));
        break;
    case SIconType::MText:
        painter.drawLine(QPointF(10, 14), QPointF(54, 14));
        painter.drawLine(QPointF(10, 25), QPointF(47, 25));
        painter.drawLine(QPointF(10, 36), QPointF(54, 36));
        painter.drawLine(QPointF(10, 47), QPointF(39, 47));
        painter.setPen(QPen(accent_color, 3.0));
        painter.drawRect(QRectF(7, 8, 50, 47));
        break;
    case SIconType::Leader:
    {
        QPolygonF leader;
        leader << QPointF(8, 52) << QPointF(24, 34) << QPointF(40, 34);
        painter.drawPolyline(leader);
        painter.setBrush(accent_color);
        QPolygonF arrow;
        arrow << QPointF(8, 52) << QPointF(12, 40) << QPointF(20, 48);
        painter.drawPolygon(arrow);
        painter.drawLine(QPointF(40, 26), QPointF(57, 26));
        painter.drawLine(QPointF(40, 35), QPointF(54, 35));
        painter.drawLine(QPointF(40, 44), QPointF(57, 44));
        break;
    }
    case SIconType::TextStyle:
        painter.drawLine(QPointF(9, 13), QPointF(43, 13));
        painter.drawLine(QPointF(26, 13), QPointF(26, 52));
        painter.drawLine(QPointF(15, 52), QPointF(37, 52));
        painter.setPen(QPen(accent_color, 3.0));
        painter.drawEllipse(QPointF(50, 42), 8, 8);
        painter.drawLine(QPointF(50, 30), QPointF(50, 34));
        painter.drawLine(QPointF(50, 50), QPointF(50, 55));
        break;
    case SIconType::ThemeDark:
        painter.setBrush(foreground);
        painter.drawEllipse(QRectF(12, 9, 42, 42));
        painter.setBrush(QColor(0, 0, 0, 0));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QRectF(29, 4, 31, 40));
        break;
    case SIconType::ThemeLight:
        painter.drawEllipse(QRectF(20, 20, 24, 24));
        for (int angle = 0; angle < 360; angle += 45)
        {
            const double radians = angle * 3.141592653589793 / 180.0;
            painter.drawLine(QPointF(32 + std::cos(radians) * 18, 32 + std::sin(radians) * 18),
                             QPointF(32 + std::cos(radians) * 26, 32 + std::sin(radians) * 26));
        }
        break;
    case SIconType::ThemeHighContrast:
        painter.setBrush(foreground);
        painter.drawEllipse(QRectF(10, 10, 44, 44));
        painter.setBrush(accent_color);
        painter.setPen(Qt::NoPen);
        painter.drawPie(QRectF(10, 10, 44, 44), -90 * 16, 180 * 16);
        break;
    case SIconType::Theme:
        painter.drawEllipse(QRectF(9, 9, 46, 46));
        painter.setBrush(foreground);
        painter.drawPie(QRectF(9, 9, 46, 46), 90 * 16, 180 * 16);
        painter.setBrush(accent_color);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(32, 20), 5, 5);
        painter.drawEllipse(QPointF(32, 44), 5, 5);
        break;
    case SIconType::Viewport:
        painter.drawRoundedRect(QRectF(7, 10, 50, 39), 4, 4);
        painter.drawLine(QPointF(32, 49), QPointF(32, 57));
        painter.drawLine(QPointF(21, 57), QPointF(43, 57));
        painter.setPen(QPen(accent_color, 3.0));
        painter.drawLine(QPointF(15, 40), QPointF(45, 18));
        break;
    case SIconType::Layers:
        for (int index = 0; index < 3; ++index)
        {
            const double offset = index * 9.0;
            QPolygonF layer;
            layer << QPointF(10, 20 + offset) << QPointF(32, 8 + offset) << QPointF(54, 20 + offset)
                  << QPointF(32, 32 + offset);
            painter.drawPolyline(layer);
        }
        break;
    case SIconType::Properties:
        for (int row = 0; row < 3; ++row)
        {
            const double y = 15 + row * 17;
            painter.drawLine(QPointF(12, y), QPointF(52, y));
            painter.setBrush(accent_color);
            painter.drawEllipse(QPointF(row == 1 ? 41 : 24, y), 4, 4);
            painter.setBrush(Qt::NoBrush);
        }
        break;
    case SIconType::ExternalReference:
        painter.drawRoundedRect(QRectF(8, 19, 30, 32), 3, 3);
        painter.drawRoundedRect(QRectF(26, 10, 30, 32), 3, 3);
        painter.setPen(QPen(accent_color, 4.0));
        painter.drawLine(QPointF(22, 36), QPointF(42, 25));
        break;
    case SIconType::ToolPalette:
        painter.drawRoundedRect(QRectF(9, 9, 46, 46), 4, 4);
        painter.drawLine(QPointF(9, 24), QPointF(55, 24));
        painter.drawLine(QPointF(25, 24), QPointF(25, 55));
        painter.setPen(QPen(accent_color, 4.0));
        painter.drawEllipse(QPointF(40, 39), 8, 8);
        break;
    case SIconType::DesignCenter:
        painter.drawEllipse(QRectF(10, 10, 44, 44));
        painter.drawEllipse(QRectF(22, 22, 20, 20));
        painter.setPen(QPen(accent_color, 4.0));
        painter.drawLine(QPointF(32, 5), QPointF(32, 14));
        painter.drawLine(QPointF(32, 50), QPointF(32, 59));
        painter.drawLine(QPointF(5, 32), QPointF(14, 32));
        painter.drawLine(QPointF(50, 32), QPointF(59, 32));
        break;
    case SIconType::SheetSet:
        painter.drawRect(QRectF(16, 8, 34, 45));
        painter.drawLine(QPointF(22, 20), QPointF(44, 20));
        painter.drawLine(QPointF(22, 29), QPointF(44, 29));
        painter.drawLine(QPointF(22, 38), QPointF(38, 38));
        painter.setPen(QPen(accent_color, 4.0));
        painter.drawLine(QPointF(9, 15), QPointF(9, 57));
        break;
    case SIconType::CompatibilityReport:
        drawFileIcon(painter, false);
        painter.setPen(QPen(accent_color, 4.0));
        painter.drawLine(QPointF(32, 27), QPointF(32, 42));
        painter.drawPoint(QPointF(32, 49));
        break;
    case SIconType::CommandLine:
        painter.drawRoundedRect(QRectF(7, 12, 50, 40), 5, 5);
        painter.drawLine(QPointF(15, 24), QPointF(24, 32));
        painter.drawLine(QPointF(24, 32), QPointF(15, 40));
        painter.setPen(QPen(accent_color, 4.0));
        painter.drawLine(QPointF(31, 40), QPointF(47, 40));
        break;
    case SIconType::ModelSpace:
        painter.drawLine(QPointF(11, 49), QPointF(32, 11));
        painter.drawLine(QPointF(32, 11), QPointF(53, 49));
        painter.drawLine(QPointF(53, 49), QPointF(11, 49));
        painter.setPen(QPen(accent_color, 3.0));
        painter.drawLine(QPointF(32, 11), QPointF(32, 49));
        break;
    case SIconType::LayoutSpace:
        painter.drawRect(QRectF(9, 9, 46, 46));
        painter.drawRect(QRectF(16, 17, 32, 25));
        painter.setPen(QPen(accent_color, 3.0));
        painter.drawLine(QPointF(20, 49), QPointF(44, 49));
        break;
    case SIconType::Snap:
        painter.drawLine(QPointF(10, 50), QPointF(51, 11));
        painter.setPen(QPen(accent_color, 4.0));
        painter.setBrush(accent_color);
        painter.drawEllipse(QPointF(31, 31), 6, 6);
        break;
    case SIconType::Grid:
        for (int value = 13; value <= 51; value += 13)
        {
            painter.drawLine(QPointF(value, 9), QPointF(value, 55));
            painter.drawLine(QPointF(9, value), QPointF(55, value));
        }
        break;
    case SIconType::Ortho:
        painter.drawLine(QPointF(13, 11), QPointF(13, 51));
        painter.drawLine(QPointF(13, 51), QPointF(54, 51));
        painter.setPen(QPen(accent_color, 4.0));
        painter.drawRect(QRectF(13, 39, 12, 12));
        break;
    case SIconType::Tracking:
        painter.setPen(QPen(foreground, 3.0, Qt::DashLine));
        painter.drawLine(QPointF(10, 50), QPointF(51, 10));
        painter.drawLine(QPointF(10, 10), QPointF(51, 51));
        painter.setPen(QPen(accent_color, 4.0));
        painter.drawEllipse(QPointF(31, 31), 5, 5);
        break;
    case SIconType::Lineweight:
        painter.setPen(QPen(foreground, 2.0));
        painter.drawLine(QPointF(11, 15), QPointF(53, 15));
        painter.setPen(QPen(foreground, 4.0));
        painter.drawLine(QPointF(11, 31), QPointF(53, 31));
        painter.setPen(QPen(accent_color, 7.0));
        painter.drawLine(QPointF(11, 49), QPointF(53, 49));
        break;
    case SIconType::Settings:
        painter.drawEllipse(QRectF(11, 11, 42, 42));
        painter.drawEllipse(QRectF(23, 23, 18, 18));
        for (int angle = 0; angle < 360; angle += 60)
        {
            const double radians = angle * 3.141592653589793 / 180.0;
            painter.drawLine(QPointF(32 + std::cos(radians) * 20, 32 + std::sin(radians) * 20),
                             QPointF(32 + std::cos(radians) * 27, 32 + std::sin(radians) * 27));
        }
        break;
    case SIconType::UiScale:
        painter.drawRect(QRectF(10, 13, 44, 38));
        painter.drawLine(QPointF(18, 23), QPointF(46, 23));
        painter.setPen(QPen(accent_color, 3.5, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(18, 41), QPointF(46, 41));
        painter.drawLine(QPointF(18, 36), QPointF(18, 46));
        painter.drawLine(QPointF(46, 36), QPointF(46, 46));
        break;
    case SIconType::Simulation:
        painter.drawEllipse(QRectF(9, 9, 46, 46));
        painter.setPen(QPen(accent_color, 4.0));
        painter.drawLine(QPointF(18, 45), QPointF(31, 31));
        painter.drawLine(QPointF(31, 31), QPointF(47, 19));
        painter.drawEllipse(QPointF(31, 31), 4, 4);
        break;
    case SIconType::Play:
        painter.setBrush(accent_color);
        painter.drawPolygon(QPolygonF({QPointF(20, 12), QPointF(52, 32), QPointF(20, 52)}));
        break;
    case SIconType::Pause:
        painter.setBrush(accent_color);
        painter.drawRoundedRect(QRectF(17, 12, 10, 40), 2, 2);
        painter.drawRoundedRect(QRectF(37, 12, 10, 40), 2, 2);
        break;
    case SIconType::Step:
        painter.setBrush(accent_color);
        painter.drawPolygon(QPolygonF({QPointF(13, 13), QPointF(43, 32), QPointF(13, 51)}));
        painter.drawRect(QRectF(46, 12, 6, 40));
        break;
    case SIconType::SpeedUp:
    case SIconType::SpeedDown:
        painter.drawArc(QRectF(10, 10, 44, 44), 25 * 16, 245 * 16);
        painter.setPen(QPen(accent_color, 4.0));
        painter.drawLine(QPointF(32, 32), QPointF(icon_type == SIconType::SpeedUp ? 47 : 20, 20));
        break;
    case SIconType::Direction:
        painter.drawLine(QPointF(10, 20), QPointF(52, 20));
        painter.drawLine(QPointF(52, 20), QPointF(43, 12));
        painter.drawLine(QPointF(10, 44), QPointF(52, 44));
        painter.drawLine(QPointF(10, 44), QPointF(19, 36));
        break;
    case SIconType::Sort:
        painter.drawLine(QPointF(14, 14), QPointF(50, 14));
        painter.drawLine(QPointF(14, 27), QPointF(42, 27));
        painter.drawLine(QPointF(14, 40), QPointF(34, 40));
        painter.setPen(QPen(accent_color, 4.0));
        painter.drawLine(QPointF(50, 22), QPointF(50, 52));
        painter.drawLine(QPointF(50, 52), QPointF(42, 44));
        break;
    case SIconType::Help:
        painter.drawEllipse(QRectF(10, 10, 44, 44));
        painter.drawArc(QRectF(23, 18, 18, 17), 0, 210 * 16);
        painter.drawLine(QPointF(31, 35), QPointF(31, 42));
        painter.drawPoint(QPointF(31, 49));
        break;
    default:
        break;
    }

    return QIcon(pixmap);
}

} // namespace vectorPath
