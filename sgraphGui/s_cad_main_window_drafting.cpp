#include "s_cad_document.h"
#include "s_cad_main_window.h"
#include "s_cad_viewport.h"
#include "s_cad_workspace_widget.h"
#include "s_command_line_widget.h"
#include "s_document_transaction.h"
#include "s_drawing_settings.h"

#include <QStringList>
#include <cmath>

namespace smartGraphics
{
namespace
{

std::optional<SStandardShapeType> standardShapeFromCommand(const QString& command)
{
    if (command == QLatin1String("STAR"))
    {
        return SStandardShapeType::FivePointStar;
    }
    if (command == QLatin1String("TRIANGLE"))
    {
        return SStandardShapeType::Triangle;
    }
    if (command == QLatin1String("PENTAGON"))
    {
        return SStandardShapeType::Pentagon;
    }
    if (command == QLatin1String("HEXAGON"))
    {
        return SStandardShapeType::Hexagon;
    }
    if (command == QLatin1String("OCTAGON"))
    {
        return SStandardShapeType::Octagon;
    }
    if (command == QLatin1String("DIAMOND"))
    {
        return SStandardShapeType::Diamond;
    }
    return std::nullopt;
}

} // namespace

bool SCadMainWindow::executeDraftingCommand(const QString& normalized_command)
{
    SCadViewport* viewport = m_workspace->viewport();
    const std::optional<SStandardShapeType> standard_shape =
        standardShapeFromCommand(normalized_command);
    if (standard_shape)
    {
        viewport->setStandardShapeType(*standard_shape);
        return true;
    }
    if (normalized_command == QLatin1String("ELLIPSE") || normalized_command == QLatin1String("EL"))
    {
        viewport->setToolMode(SToolMode::Ellipse);
        return true;
    }
    if (normalized_command == QLatin1String("SPLINE") || normalized_command == QLatin1String("SPL"))
    {
        viewport->setToolMode(SToolMode::Spline);
        return true;
    }
    if (normalized_command == QLatin1String("CIRCLE 2P"))
    {
        viewport->setCircleConstruction(SCircleConstruction::TwoPoint);
        return true;
    }
    if (normalized_command == QLatin1String("CIRCLE 3P"))
    {
        viewport->setCircleConstruction(SCircleConstruction::ThreePoint);
        return true;
    }
    if (normalized_command == QLatin1String("CIRCLE D") ||
        normalized_command == QLatin1String("CIRCLE DIAMETER"))
    {
        viewport->setCircleConstruction(SCircleConstruction::CenterDiameter);
        return true;
    }
    if (normalized_command == QLatin1String("CIRCLE TTT"))
    {
        viewport->setCircleConstruction(SCircleConstruction::TangentTangentTangent);
        return true;
    }
    if (normalized_command.startsWith(QLatin1String("CIRCLE TTR")))
    {
        const QString radius_text = normalized_command.mid(10).trimmed();
        bool is_radius_valid = false;
        const double radius = radius_text.toDouble(&is_radius_valid);
        if (is_radius_valid && radius > 1.0e-9)
        {
            viewport->setCircleTangentRadius(radius);
        }
        else
        {
            m_command_line->appendMessage(tr("CIRCLE TTR 格式：CIRCLE TTR 正半径"));
        }
        return true;
    }
    if (normalized_command == QLatin1String("ARC 3P"))
    {
        viewport->setArcConstruction(SArcConstruction::ThreePoint);
        return true;
    }
    if (normalized_command == QLatin1String("ARC CSE") ||
        normalized_command == QLatin1String("ARC CENTER"))
    {
        viewport->setArcConstruction(SArcConstruction::CenterStartEnd);
        return true;
    }
    if (normalized_command == QLatin1String("ARC SCE"))
    {
        viewport->setArcConstruction(SArcConstruction::StartCenterEnd);
        return true;
    }
    if (normalized_command.startsWith(QLatin1String("ARC SCA")) ||
        normalized_command.startsWith(QLatin1String("ARC CSA")) ||
        normalized_command.startsWith(QLatin1String("ARC SEA")) ||
        normalized_command.startsWith(QLatin1String("ARC SED")) ||
        normalized_command.startsWith(QLatin1String("ARC SER")))
    {
        const QStringList arc_parts =
            normalized_command.split(QLatin1Char(' '), QString::SkipEmptyParts);
        SArcConstruction construction = SArcConstruction::StartCenterAngle;
        if (arc_parts.size() >= 2 && arc_parts[1] == QLatin1String("CSA"))
        {
            construction = SArcConstruction::CenterStartAngle;
        }
        else if (arc_parts.size() >= 2 && arc_parts[1] == QLatin1String("SEA"))
        {
            construction = SArcConstruction::StartEndAngle;
        }
        else if (arc_parts.size() >= 2 && arc_parts[1] == QLatin1String("SED"))
        {
            construction = SArcConstruction::StartEndDirection;
        }
        else if (arc_parts.size() >= 2 && arc_parts[1] == QLatin1String("SER"))
        {
            construction = SArcConstruction::StartEndRadius;
        }
        bool is_parameter_valid = false;
        const double parameter =
            arc_parts.size() == 3 ? arc_parts[2].toDouble(&is_parameter_valid) : 0.0;
        if (arc_parts.size() == 2)
        {
            viewport->setArcConstruction(construction);
        }
        else if (is_parameter_valid && std::isfinite(parameter))
        {
            viewport->setArcConstructionParameter(construction, parameter);
        }
        else
        {
            m_command_line->appendMessage(tr("ARC 参数格式：ARC SCA|CSA|SEA|SED|SER [数值]。"));
        }
        return true;
    }
    if (normalized_command == QLatin1String("UNITS"))
    {
        showUnitsDialog();
        return true;
    }
    if (normalized_command == QLatin1String("SNAP"))
    {
        viewport->setGridSnapEnabled(!viewport->isGridSnapEnabled());
        return true;
    }

    const QStringList parts = normalized_command.split(QLatin1Char(' '), QString::SkipEmptyParts);
    if (parts.size() == 3 && parts[0] == QLatin1String("GRID") &&
        parts[1] == QLatin1String("ROTATION"))
    {
        bool is_valid = false;
        const double rotation = parts[2].toDouble(&is_valid);
        if (is_valid)
        {
            viewport->setGridRotation(rotation);
        }
        else
        {
            m_command_line->appendMessage(tr("栅格旋转格式：GRID ROTATION 角度"));
        }
        return true;
    }
    SDrawingSettings drawing_settings = m_document->drawingSettings();
    bool has_drawing_settings_command = false;
    bool is_drawing_settings_valid = true;
    if (parts.size() == 2 && parts[0] == QLatin1String("UNITS"))
    {
        const std::optional<SInsertionUnit> insertion_unit = insertionUnitFromKey(parts[1]);
        is_drawing_settings_valid = insertion_unit.has_value();
        if (insertion_unit)
        {
            drawing_settings.insertion_unit = *insertion_unit;
        }
        has_drawing_settings_command = true;
    }
    else if (parts.size() == 2 && parts[0] == QLatin1String("ANGLEFORMAT"))
    {
        const std::optional<SAngleFormat> angle_format = angleFormatFromKey(parts[1]);
        is_drawing_settings_valid = angle_format.has_value();
        if (angle_format)
        {
            drawing_settings.angle_format = *angle_format;
        }
        has_drawing_settings_command = true;
    }
    else if (parts.size() == 2 &&
             (parts[0] == QLatin1String("LUPREC") || parts[0] == QLatin1String("AUPREC")))
    {
        bool is_precision_valid = false;
        const int precision = parts[1].toInt(&is_precision_valid);
        is_drawing_settings_valid = is_precision_valid && precision >= 0 && precision <= 8;
        if (is_drawing_settings_valid && parts[0] == QLatin1String("LUPREC"))
        {
            drawing_settings.linear_precision = precision;
        }
        else if (is_drawing_settings_valid)
        {
            drawing_settings.angular_precision = precision;
        }
        has_drawing_settings_command = true;
    }
    if (has_drawing_settings_command)
    {
        if (!is_drawing_settings_valid)
        {
            m_command_line->appendMessage(tr("单位设置格式无效：UNITS MM|CM|M|INCH|FEET|UNITLESS，"
                                             "ANGLEFORMAT DECIMAL|DMS|GRADS|RADIANS，精度 0–8。"));
            return true;
        }
        auto transaction = m_document->beginTransaction(tr("更改图形单位"));
        if (transaction->setDrawingSettings(drawing_settings))
        {
            transaction->commit();
        }
        return true;
    }
    if (parts.size() == 2 && parts[0] == QLatin1String("SNAP") &&
        (parts[1] == QLatin1String("ON") || parts[1] == QLatin1String("OFF")))
    {
        viewport->setGridSnapEnabled(parts[1] == QLatin1String("ON"));
        return true;
    }
    if ((parts.size() == 2 && parts[0] == QLatin1String("SNAP")) ||
        (parts.size() == 3 && parts[0] == QLatin1String("GRID") &&
         parts[1] == QLatin1String("SPACING")))
    {
        bool is_valid = false;
        const double spacing = parts.back().toDouble(&is_valid);
        if (is_valid && spacing > 1.0e-9 && spacing <= 1.0e9)
        {
            viewport->setGridSnapSpacing(spacing);
            viewport->setGridSnapEnabled(true);
        }
        else
        {
            m_command_line->appendMessage(tr("捕捉间距必须大于零，例如：SNAP 5"));
        }
        return true;
    }
    return false;
}

} // namespace smartGraphics
