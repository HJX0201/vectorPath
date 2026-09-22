#include "vp_cad_document.h"
#include "vp_cad_viewport.h"
#include "vp_document_transaction.h"

#include <cmath>

namespace Vp
{
namespace
{

constexpr double kPointTolerance = 1.0e-9;

bool isKeyword(const QString& normalized_keyword, const char* full_name, const char* alias)
{
    return normalized_keyword == QLatin1String(full_name) ||
           normalized_keyword == QLatin1String(alias);
}

} // namespace

bool VpCadViewport::submitCommandKeyword(const QString& keyword)
{
    const QString normalized_keyword = keyword.simplified().toUpper();
    if (normalized_keyword == QLatin1String("CANCEL") || normalized_keyword == QLatin1String("ESC"))
    {
        cancelCommand();
        return true;
    }

    if (m_tool_mode == VpToolMode::Leader && isKeyword(normalized_keyword, "UNDO", "U"))
    {
        if (!m_input_points.empty())
        {
            m_input_points.pop_back();
        }
        emit commandMessage(m_input_points.empty()
                                ? tr("MLEADER 指定箭头位置：")
                                : (m_input_points.size() == 1 ? tr("MLEADER 指定引线折点：")
                                                              : tr("MLEADER 指定文字落点：")));
        update();
        return true;
    }

    if (m_tool_mode == VpToolMode::Circle)
    {
        const QStringList circle_parts =
            normalized_keyword.split(QLatin1Char(' '), QString::SkipEmptyParts);
        if (!circle_parts.empty() && circle_parts.front() == QLatin1String("TTR"))
        {
            if (circle_parts.size() == 1)
            {
                setCircleConstruction(VpCircleConstruction::TangentTangentRadius);
            }
            else
            {
                bool is_radius_valid = false;
                const double radius =
                    circle_parts.size() == 2 ? circle_parts[1].toDouble(&is_radius_valid) : 0.0;
                if (is_radius_valid && radius > 1.0e-9)
                {
                    setCircleTangentRadius(radius);
                }
                else
                {
                    emit commandMessage(tr("CIRCLE TTR 用法：TTR 正半径。"));
                }
            }
            return true;
        }
        if (normalized_keyword == QLatin1String("TTT"))
        {
            setCircleConstruction(VpCircleConstruction::TangentTangentTangent);
            return true;
        }
        if (normalized_keyword == QLatin1String("2P"))
        {
            setCircleConstruction(VpCircleConstruction::TwoPoint);
            return true;
        }
        if (normalized_keyword == QLatin1String("3P"))
        {
            setCircleConstruction(VpCircleConstruction::ThreePoint);
            return true;
        }
        if (isKeyword(normalized_keyword, "DIAMETER", "D"))
        {
            setCircleConstruction(VpCircleConstruction::CenterDiameter);
            return true;
        }
        if (isKeyword(normalized_keyword, "RADIUS", "R"))
        {
            setCircleConstruction(VpCircleConstruction::CenterRadius);
            return true;
        }
    }

    if (m_tool_mode == VpToolMode::Arc)
    {
        const bool is_parameter_method =
            m_arc_construction == VpArcConstruction::StartCenterAngle ||
            m_arc_construction == VpArcConstruction::CenterStartAngle ||
            m_arc_construction == VpArcConstruction::StartEndAngle ||
            m_arc_construction == VpArcConstruction::StartEndDirection ||
            m_arc_construction == VpArcConstruction::StartEndRadius;
        if (is_parameter_method && m_input_points.size() >= 2)
        {
            bool is_value_valid = false;
            const double value = normalized_keyword.toDouble(&is_value_valid);
            if (is_value_valid && std::isfinite(value))
            {
                m_arc_construction_parameter = value;
                completeArcConstruction();
                return true;
            }
        }
        if (normalized_keyword == QLatin1String("3P"))
        {
            setArcConstruction(VpArcConstruction::ThreePoint);
            return true;
        }
        if (normalized_keyword == QLatin1String("CSE") ||
            normalized_keyword == QLatin1String("CENTER"))
        {
            setArcConstruction(VpArcConstruction::CenterStartEnd);
            return true;
        }
        if (normalized_keyword == QLatin1String("SCE"))
        {
            setArcConstruction(VpArcConstruction::StartCenterEnd);
            return true;
        }
        const QStringList arc_parts =
            normalized_keyword.split(QLatin1Char(' '), QString::SkipEmptyParts);
        VpArcConstruction construction = VpArcConstruction::ThreePoint;
        bool is_parameter_keyword = true;
        if (!arc_parts.empty() && arc_parts.front() == QLatin1String("SCA"))
        {
            construction = VpArcConstruction::StartCenterAngle;
        }
        else if (!arc_parts.empty() && arc_parts.front() == QLatin1String("CSA"))
        {
            construction = VpArcConstruction::CenterStartAngle;
        }
        else if (!arc_parts.empty() && arc_parts.front() == QLatin1String("SEA"))
        {
            construction = VpArcConstruction::StartEndAngle;
        }
        else if (!arc_parts.empty() && arc_parts.front() == QLatin1String("SED"))
        {
            construction = VpArcConstruction::StartEndDirection;
        }
        else if (!arc_parts.empty() && arc_parts.front() == QLatin1String("SER"))
        {
            construction = VpArcConstruction::StartEndRadius;
        }
        else
        {
            is_parameter_keyword = false;
        }
        if (is_parameter_keyword)
        {
            bool is_parameter_valid = false;
            const double parameter =
                arc_parts.size() == 2 ? arc_parts[1].toDouble(&is_parameter_valid) : 0.0;
            if (arc_parts.size() == 1)
            {
                setArcConstruction(construction);
            }
            else if (is_parameter_valid && std::isfinite(parameter))
            {
                setArcConstructionParameter(construction, parameter);
            }
            else
            {
                emit commandMessage(tr("ARC 参数方法格式：SCA|CSA|SEA|SED|SER [数值]。"));
            }
            return true;
        }
    }

    if (m_tool_mode == VpToolMode::ArrayEdit)
    {
        const QStringList parts =
            normalized_keyword.split(QLatin1Char(' '), QString::SkipEmptyParts);
        bool is_valid = false;
        if (parts.size() == 5 && parts[0] == QLatin1String("RECT"))
        {
            bool is_row_valid = false;
            bool is_column_spacing_valid = false;
            bool is_row_spacing_valid = false;
            const int column_count = parts[1].toInt(&is_valid);
            const int row_count = parts[2].toInt(&is_row_valid);
            const double column_spacing = parts[3].toDouble(&is_column_spacing_valid);
            const double row_spacing = parts[4].toDouble(&is_row_spacing_valid);
            if (is_valid && is_row_valid && is_column_spacing_valid && is_row_spacing_valid)
            {
                editSelectedRectangularArray(column_count, row_count, column_spacing, row_spacing);
            }
            else
            {
                emit commandMessage(tr("ARRAYEDIT RECT 格式：RECT 列 行 列距 行距。"));
            }
            return true;
        }
        if (parts.size() == 3 && parts[0] == QLatin1String("POLAR"))
        {
            bool is_angle_valid = false;
            const int item_count = parts[1].toInt(&is_valid);
            const double fill_angle = parts[2].toDouble(&is_angle_valid);
            if (is_valid && is_angle_valid)
            {
                editSelectedPolarArray(item_count, fill_angle);
            }
            else
            {
                emit commandMessage(tr("ARRAYEDIT POLAR 格式：POLAR 项目数 填充角。"));
            }
            return true;
        }
        if (parts.size() == 3 && parts[0] == QLatin1String("PATH"))
        {
            is_valid = parts[2] == QLatin1String("ALIGN") || parts[2] == QLatin1String("NOALIGN");
            bool is_count_valid = false;
            const int item_count = parts[1].toInt(&is_count_valid);
            if (is_count_valid && is_valid)
            {
                editSelectedPathArray(item_count, parts[2] == QLatin1String("ALIGN"));
            }
            else
            {
                emit commandMessage(tr("ARRAYEDIT PATH 格式：PATH 项目数 ALIGN|NOALIGN。"));
            }
            return true;
        }
    }

    if (m_tool_mode == VpToolMode::Polyline)
    {
        const QStringList keyword_parts =
            normalized_keyword.split(QLatin1Char(' '), QString::SkipEmptyParts);
        if (!keyword_parts.empty() && (keyword_parts.front() == QLatin1String("WIDTH") ||
                                       keyword_parts.front() == QLatin1String("W")))
        {
            bool first_valid = false;
            bool second_valid = false;
            const double start_width =
                keyword_parts.size() >= 2 ? keyword_parts[1].toDouble(&first_valid) : 0.0;
            const double end_width =
                keyword_parts.size() >= 3 ? keyword_parts[2].toDouble(&second_valid) : start_width;
            if (keyword_parts.size() == 2)
            {
                second_valid = first_valid;
            }
            if (!first_valid || !second_valid || start_width < 0.0 || end_width < 0.0 ||
                start_width > 1.0e9 || end_width > 1.0e9)
            {
                emit commandMessage(tr("PLINE WIDTH 用法：WIDTH 起始宽度 [终止宽度]。"));
                return true;
            }
            m_polyline_start_width = start_width;
            m_polyline_end_width = end_width;
            emit commandMessage(tr("PLINE 下一段宽度：%1 → %2。指定下一点：")
                                    .arg(start_width, 0, 'f', 3)
                                    .arg(end_width, 0, 'f', 3));
            update();
            return true;
        }
        if (isKeyword(normalized_keyword, "ARC", "A"))
        {
            m_polyline_arc_mode = true;
            m_polyline_arc_point.reset();
            emit commandMessage(m_input_points.empty() ? tr("PLINE 请先指定起点：")
                                                       : tr("PLINE 圆弧模式：指定弧上点："));
            update();
            return true;
        }
        if (isKeyword(normalized_keyword, "LINE", "L"))
        {
            m_polyline_arc_mode = false;
            m_polyline_arc_point.reset();
            emit commandMessage(m_input_points.empty() ? tr("PLINE 请先指定起点：")
                                                       : tr("PLINE 直线模式：指定下一点："));
            update();
            return true;
        }
        if (isKeyword(normalized_keyword, "CLOSE", "C"))
        {
            if (m_input_points.size() < 3)
            {
                emit commandMessage(tr("PLINE 至少需要三个顶点才能闭合。"));
            }
            else
            {
                completePolyline(true);
            }
            return true;
        }
        if (isKeyword(normalized_keyword, "UNDO", "U"))
        {
            if (m_polyline_arc_point)
            {
                m_polyline_arc_point.reset();
            }
            else if (!m_input_points.empty())
            {
                if (m_input_points.size() > 1 && !m_polyline_bulges.empty())
                {
                    m_polyline_bulges.pop_back();
                    m_polyline_start_widths.pop_back();
                    m_polyline_end_widths.pop_back();
                }
                m_input_points.pop_back();
            }
            emit commandMessage(m_input_points.empty()
                                    ? tr("PLINE 已放弃最后一点，请指定起点：")
                                    : tr("PLINE 已放弃最后一点，指定下一点或 [闭合(C)/放弃(U)]："));
            update();
            return true;
        }
        if (isKeyword(normalized_keyword, "DONE", "ENTER"))
        {
            completePolyline(false);
            return true;
        }
    }

    if (m_tool_mode == VpToolMode::PolylineEdit && handlePolylineEditKeyword(normalized_keyword))
    {
        return true;
    }

    if (m_tool_mode == VpToolMode::SplineEdit && handleSplineEditKeyword(normalized_keyword))
    {
        return true;
    }

    if (m_tool_mode == VpToolMode::Line)
    {
        if (isKeyword(normalized_keyword, "CLOSE", "C"))
        {
            if (!m_document || !m_first_point || !m_command_start_point || m_line_segment_count < 2)
            {
                emit commandMessage(tr("LINE 至少需要一条线段才能闭合。"));
                return true;
            }
            auto transaction = m_document->beginTransaction(tr("闭合直线链"));
            transaction->addLine(*m_first_point, *m_command_start_point);
            transaction->commit();
            m_first_point.reset();
            m_command_start_point.reset();
            m_line_segment_count = 0;
            m_tool_mode = VpToolMode::Select;
            emit toolModeChanged(m_tool_mode);
            emit commandMessage(tr("LINE 线链已闭合。"));
            update();
            return true;
        }
        if (isKeyword(normalized_keyword, "UNDO", "U"))
        {
            if (!m_document || !m_document->canUndo() || !m_first_point || !m_command_start_point ||
                m_line_segment_count <= 0 || m_document->entities().empty())
            {
                emit commandMessage(tr("LINE 没有可放弃的线段。"));
                return true;
            }
            const VpEntityRecord& last_entity = m_document->entities().back();
            if (last_entity.type != VpEntityType::Line)
            {
                emit commandMessage(tr("LINE 没有可放弃的线段。"));
                return true;
            }
            const auto& last_line = std::get<VpLineEntity>(last_entity.geometry);
            if (distance(last_line.end_point, *m_first_point) > kPointTolerance)
            {
                emit commandMessage(tr("LINE 没有可放弃的线段。"));
                return true;
            }
            m_first_point = last_line.start_point;
            --m_line_segment_count;
            m_document->undo();
            emit commandMessage(tr("LINE 已放弃最后一段，指定下一点或 [闭合(C)/放弃(U)]："));
            update();
            return true;
        }
    }

    if ((m_tool_mode == VpToolMode::Arc || m_tool_mode == VpToolMode::Circle ||
         m_tool_mode == VpToolMode::Spline || dimensionTypeForToolMode(m_tool_mode)) &&
        isKeyword(normalized_keyword, "UNDO", "U"))
    {
        if (!m_input_points.empty())
        {
            m_input_points.pop_back();
        }
        emit commandMessage(tr("已放弃最后一个输入点。"));
        update();
        return true;
    }
    if (m_tool_mode == VpToolMode::Join && isKeyword(normalized_keyword, "DONE", "ENTER"))
    {
        completeJoin();
        return true;
    }
    return false;
}

} // namespace Vp
