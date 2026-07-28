#include "s_specialized_icon.h"

#include "s_dimension_icon.h"
#include "s_hatch_icon.h"
#include "s_output_icon.h"
#include "s_quick_transform_icon.h"
#include "s_shape_boolean_icon.h"

namespace smartGraphics
{

bool drawSpecializedIcon(QPainter& painter, SIconType icon_type, const QColor& foreground,
                         const QColor& accent)
{
    switch (icon_type)
    {
    case SIconType::QuickReverse:
    case SIconType::RotateClockwise90:
    case SIconType::RotateClockwise180:
    case SIconType::RotateCounterclockwise90:
    case SIconType::RotateCounterclockwise180:
    case SIconType::MirrorVertical:
    case SIconType::MirrorHorizontal:
        drawQuickTransformIcon(painter, icon_type, foreground, accent);
        return true;
    case SIconType::ShapeStar:
    case SIconType::ShapeTriangle:
    case SIconType::ShapePentagon:
    case SIconType::ShapeHexagon:
    case SIconType::ShapeOctagon:
    case SIconType::ShapeDiamond:
    case SIconType::BooleanUnion:
    case SIconType::BooleanIntersection:
    case SIconType::BooleanDifference:
    case SIconType::BooleanXor:
    case SIconType::BooleanComplement:
        drawShapeBooleanIcon(painter, icon_type, foreground, accent);
        return true;
    case SIconType::Dimension:
    case SIconType::DimensionAligned:
    case SIconType::DimensionAngular:
    case SIconType::DimensionRadius:
    case SIconType::DimensionDiameter:
    case SIconType::DimensionArcLength:
    case SIconType::DimensionOrdinate:
    case SIconType::DimensionStyle:
        drawDimensionIcon(painter, icon_type, accent);
        return true;
    case SIconType::Hatch:
    case SIconType::HatchPattern:
    case SIconType::HatchGradient:
    case SIconType::HatchEdit:
        drawHatchIcon(painter, icon_type, accent);
        return true;
    case SIconType::PageSetup:
    case SIconType::PlotStyle:
    case SIconType::PlotPreview:
    case SIconType::Print:
    case SIconType::ExportPdf:
    case SIconType::Publish:
        drawOutputIcon(painter, icon_type, accent);
        return true;
    default:
        return false;
    }
}

} // namespace smartGraphics
