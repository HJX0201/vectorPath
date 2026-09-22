#include "vp_cad_document.h"
#include "vp_cad_viewport.h"
#include "vp_document_transaction.h"
#include "vp_standard_shape.h"

namespace Vp
{

void VpCadViewport::setStandardShapeType(VpStandardShapeType shape_type)
{
    m_standard_shape_type = shape_type;
    setToolMode(VpToolMode::StandardShape);
}

VpStandardShapeType VpCadViewport::standardShapeType() const noexcept
{
    return m_standard_shape_type;
}

void VpCadViewport::acceptStandardShapePoint(const VpPoint2d& world_point)
{
    if (!m_document)
    {
        return;
    }
    if (!m_first_point)
    {
        m_first_point = world_point;
        emit commandMessage(tr("标准图形：指定外接圆半径点："));
        update();
        return;
    }

    std::vector<VpPoint2d> vertices =
        standardShapeVertices(m_standard_shape_type, *m_first_point, world_point);
    if (!vertices.empty())
    {
        auto transaction = m_document->beginTransaction(tr("创建标准图形"));
        transaction->addPolyline(std::move(vertices), true);
        transaction->commit();
    }
    m_first_point.reset();
    emit commandMessage(tr("标准图形：指定中心点："));
    update();
}

} // namespace Vp
