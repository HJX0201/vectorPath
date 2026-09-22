#pragma once

#include "vp_geometry_types.h"

#include <QPointF>
#include <QString>

namespace Vp
{

QPointF toQtPoint(const VpPoint2d& point) noexcept;
VpPoint2d toCorePoint(const QPointF& point) noexcept;
QString formatPoint(const VpPoint2d& point, int precision = 3);

} // namespace Vp
