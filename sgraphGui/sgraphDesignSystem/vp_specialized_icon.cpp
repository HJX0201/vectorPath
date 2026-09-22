#include "vp_specialized_icon.h"

#include "vp_dimension_icon.h"
#include "vp_hatch_icon.h"
#include "vp_output_icon.h"
#include "vp_quick_transform_icon.h"
#include "vp_shape_boolean_icon.h"

namespace Vp
{

bool drawSpecializedIcon(QPainter& painter, VpIconType icon_type, const QColor& foreground,
                         const QColor& accent)
{
    switch (icon_type)
    {
    case VpIconType::QuickReverse:
    case VpIconType::RotateClockwise90:
    case VpIconType::RotateClockwise180:
    case VpIconType::RotateCounterclockwise90:
    case VpIconType::RotateCounterclockwise180:
    case VpIconType::MirrorVertical:
    case VpIconType::MirrorHorizontal:
        drawQuickTransformIcon(painter, icon_type, foreground, accent);
        return true;
    case VpIconType::ShapeStar:
    case VpIconType::ShapeTriangle:
    case VpIconType::ShapePentagon:
    case VpIconType::ShapeHexagon:
    case VpIconType::ShapeOctagon:
    case VpIconType::ShapeDiamond:
    case VpIconType::BooleanUnion:
    case VpIconType::BooleanIntersection:
    case VpIconType::BooleanDifference:
    case VpIconType::BooleanXor:
    case VpIconType::BooleanComplement:
        drawShapeBooleanIcon(painter, icon_type, foreground, accent);
        return true;
    case VpIconType::Dimension:
    case VpIconType::DimensionAligned:
    case VpIconType::DimensionAngular:
    case VpIconType::DimensionRadius:
    case VpIconType::DimensionDiameter:
    case VpIconType::DimensionArcLength:
    case VpIconType::DimensionOrdinate:
    case VpIconType::DimensionStyle:
        drawDimensionIcon(painter, icon_type, accent);
        return true;
    case VpIconType::Hatch:
    case VpIconType::HatchPattern:
    case VpIconType::HatchGradient:
    case VpIconType::HatchEdit:
        drawHatchIcon(painter, icon_type, accent);
        return true;
    case VpIconType::PageSetup:
    case VpIconType::PlotStyle:
    case VpIconType::PlotPreview:
    case VpIconType::Print:
    case VpIconType::ExportPdf:
    case VpIconType::Publish:
        drawOutputIcon(painter, icon_type, accent);
        return true;
    default:
        return false;
    }
}

} // namespace Vp
