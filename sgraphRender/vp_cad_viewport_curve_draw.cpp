#include "vp_cad_document.h"
#include "vp_cad_viewport.h"
#include "vp_document_transaction.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace Vp
{

void VpCadViewport::acceptEllipsePoint(const VpPoint2d& world_point)
{
    m_input_points.push_back(world_point);
    if (m_input_points.size() == 1)
    {
        emit commandMessage(tr("ELLIPSE 指定第一条轴的第二个端点："));
    }
    else if (m_input_points.size() == 2)
    {
        emit commandMessage(tr("ELLIPSE 指定另一条半轴长度："));
    }
    else
    {
        const VpPoint2d center{(m_input_points[0].x + m_input_points[1].x) * 0.5,
                               (m_input_points[0].y + m_input_points[1].y) * 0.5};
        const VpPoint2d major_axis{m_input_points[1].x - center.x, m_input_points[1].y - center.y};
        const double major_length = std::hypot(major_axis.x, major_axis.y);
        const double minor_length = std::min(distance(center, world_point), major_length);
        if (major_length > 1.0e-9 && minor_length > 1.0e-9)
        {
            const VpPoint2d minor_axis{-major_axis.y * minor_length / major_length,
                                       major_axis.x * minor_length / major_length};
            auto transaction = m_document->beginTransaction(tr("创建椭圆"));
            transaction->addEllipse(center, major_axis, minor_axis);
            transaction->commit();
        }
        else
        {
            emit commandMessage(tr("ELLIPSE 两条半轴长度必须大于零。"));
        }
        m_input_points.clear();
        emit commandMessage(tr("ELLIPSE 指定第一条轴的第一个端点："));
    }
    update();
}

void VpCadViewport::acceptSplinePoint(const VpPoint2d& world_point)
{
    m_input_points.push_back(world_point);
    if (m_input_points.size() < 4)
    {
        emit commandMessage(tr("SPLINE 指定第 %1 个控制点：").arg(m_input_points.size() + 1));
    }
    else
    {
        const std::array<VpPoint2d, 4> control_points{m_input_points[0], m_input_points[1],
                                                      m_input_points[2], m_input_points[3]};
        auto transaction = m_document->beginTransaction(tr("创建样条曲线"));
        transaction->addSpline(control_points);
        transaction->commit();
        m_input_points.clear();
        emit commandMessage(tr("SPLINE 指定第一个控制点："));
    }
    update();
}

} // namespace Vp
