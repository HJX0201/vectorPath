#pragma once

#include <QPointF>
#include <QString>
#include <cmath>

namespace smartCam
{

struct SPoint2d
{
    double x = 0.0;
    double y = 0.0;

    QPointF toPointF() const noexcept
    {
        return {x, y};
    }
    static SPoint2d fromPointF(const QPointF& point) noexcept
    {
        return {point.x(), point.y()};
    }
};

double distance(const SPoint2d& first_point, const SPoint2d& second_point) noexcept;
QString formatPoint(const SPoint2d& point, int precision = 3);

} // namespace smartCam
