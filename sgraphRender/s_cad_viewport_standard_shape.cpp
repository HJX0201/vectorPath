#include "s_cad_viewport.h"

#include "s_cad_document.h"
#include "s_document_transaction.h"
#include "s_standard_shape.h"

namespace smartGraphics
{

void SCadViewport::setStandardShapeType(SStandardShapeType shape_type)
{
    m_standard_shape_type = shape_type;
    setToolMode(SToolMode::StandardShape);
}

SStandardShapeType SCadViewport::standardShapeType() const noexcept
{
    return m_standard_shape_type;
}

void SCadViewport::acceptStandardShapePoint(const SPoint2d& world_point)
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

    std::vector<SPoint2d> vertices =
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

} // namespace smartGraphics
