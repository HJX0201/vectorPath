#pragma once

#include "vp_geometry_types.h"

#include <QPointF>
#include <QString>

namespace Vp
{

// 把核心坐标转换为 Qt 点。
QPointF toQtPoint(const VpPoint2d& point) noexcept;
// 把 Qt 点转换为核心坐标。
VpPoint2d toCorePoint(const QPointF& point) noexcept;
// 按精度格式化点坐标用于显示。
QString formatPoint(const VpPoint2d& point, int precision = 3);

} // namespace Vp
