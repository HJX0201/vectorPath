#include "vp_qt_geometry.h"

namespace Vp
{

QPointF toQtPoint(const VpPoint2d& point) noexcept
{
    return {point.x, point.y};
}

VpPoint2d toCorePoint(const QPointF& point) noexcept
{
    return {point.x(), point.y()};
}

QString formatPoint(const VpPoint2d& point, int precision)
{
    return QStringLiteral("%1, %2").arg(point.x, 0, 'f', precision).arg(point.y, 0, 'f', precision);
}

} // namespace Vp
