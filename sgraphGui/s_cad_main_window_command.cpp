#include "s_cad_document.h"
#include "s_cad_main_window.h"
#include "s_cad_viewport.h"
#include "s_cad_workspace_widget.h"
#include "s_command_line_widget.h"

#include <QColor>
#include <QStringList>
#include <algorithm>

namespace smartCam
{

bool SCadMainWindow::executeGripCommand(const QString& normalized_command)
{
    SGripOperation operation = SGripOperation::Stretch;
    if (normalized_command == QLatin1String("GRIP_STRETCH") ||
        normalized_command == QLatin1String("GRIP"))
    {
        operation = SGripOperation::Stretch;
    }
    else if (normalized_command == QLatin1String("GRIP_MOVE") ||
             normalized_command == QLatin1String("GM"))
    {
        operation = SGripOperation::Move;
    }
    else if (normalized_command == QLatin1String("GRIP_ROTATE") ||
             normalized_command == QLatin1String("GR"))
    {
        operation = SGripOperation::Rotate;
    }
    else if (normalized_command == QLatin1String("GRIP_SCALE") ||
             normalized_command == QLatin1String("GS"))
    {
        operation = SGripOperation::Scale;
    }
    else if (normalized_command == QLatin1String("GRIP_MIRROR") ||
             normalized_command == QLatin1String("GMI"))
    {
        operation = SGripOperation::Mirror;
    }
    else
    {
        return false;
    }
    m_workspace->viewport()->beginGripEdit(operation);
    return true;
}

bool SCadMainWindow::executeLayerCommand(const QString& simplified_command)
{
    const QStringList parts = simplified_command.split(QLatin1Char(' '), QString::SkipEmptyParts);
    if (parts.isEmpty() ||
        parts[0].compare(QLatin1String("LAYER"), Qt::CaseInsensitive) != 0)
    {
        return false;
    }

    const QString operation = parts.value(1).toUpper();
    if (operation == QLatin1String("MERGECOLOR") && parts.size() == 2)
    {
        const int merged_count = m_document->mergeLayersByColor();
        m_command_line->appendMessage(
            merged_count > 0
                ? tr("已合并 %1 个同色图层。").arg(merged_count)
                : tr("没有可合并的同色普通图层。"));
        return true;
    }
    if (parts.size() < 3)
    {
        return false;
    }
    const QString layer_name = parts[2];
    bool is_success = false;
    if (operation == QLatin1String("DELETE") && parts.size() == 3)
    {
        is_success = m_document->removeLayer(layer_name);
    }
    else if (operation == QLatin1String("CLEAR") && parts.size() == 3)
    {
        is_success = clearLayerEntities(layer_name);
    }
    else if (operation == QLatin1String("DISPLAY") && parts.size() == 4)
    {
        const QString display_state = parts[3].toUpper();
        if (display_state == QLatin1String("ON") || display_state == QLatin1String("OFF"))
        {
            const SLayerRecord* layer_record = m_document->layer(layer_name);
            const bool is_visible = display_state == QLatin1String("ON");
            is_success = layer_record &&
                         (layer_record->is_visible == is_visible ||
                          m_document->setLayerVisible(layer_name, is_visible));
        }
    }
    else if (operation == QLatin1String("COLOR") && parts.size() == 4)
    {
        const QColor color(parts[3]);
        if (color.isValid())
        {
            const SLayerRecord* layer_record = m_document->layer(layer_name);
            is_success = layer_record &&
                         (layer_record->color == color ||
                          m_document->setLayerColor(layer_name, color));
        }
    }
    else
    {
        return false;
    }
    if (!is_success)
    {
        m_command_line->appendMessage(
            tr("图层操作失败。可用命令：LAYER DELETE|CLEAR 图层，"
               "LAYER DISPLAY 图层 ON|OFF，LAYER COLOR 图层 #RRGGBB，"
               "LAYER MERGECOLOR。"));
    }
    return true;
}

bool SCadMainWindow::executeArrayParameterCommand(const QString& normalized_command)
{
    const QStringList parts = normalized_command.split(QLatin1Char(' '), QString::SkipEmptyParts);
    if (normalized_command.startsWith(QLatin1String("ARRAYEDIT ")) ||
        normalized_command.startsWith(QLatin1String("AE ")))
    {
        SCadViewport* viewport = m_workspace->viewport();
        bool is_success = false;
        if (parts.size() == 6 && parts[1] == QLatin1String("RECT"))
        {
            bool is_column_valid = false;
            bool is_row_valid = false;
            bool is_column_spacing_valid = false;
            bool is_row_spacing_valid = false;
            const int column_count = parts[2].toInt(&is_column_valid);
            const int row_count = parts[3].toInt(&is_row_valid);
            const double column_spacing = parts[4].toDouble(&is_column_spacing_valid);
            const double row_spacing = parts[5].toDouble(&is_row_spacing_valid);
            is_success = is_column_valid && is_row_valid && is_column_spacing_valid &&
                         is_row_spacing_valid &&
                         viewport->editSelectedRectangularArray(column_count, row_count,
                                                                column_spacing, row_spacing);
        }
        else if (parts.size() == 4 && parts[1] == QLatin1String("POLAR"))
        {
            bool is_count_valid = false;
            bool is_angle_valid = false;
            const int item_count = parts[2].toInt(&is_count_valid);
            const double fill_angle = parts[3].toDouble(&is_angle_valid);
            is_success = is_count_valid && is_angle_valid &&
                         viewport->editSelectedPolarArray(item_count, fill_angle);
        }
        else if (parts.size() == 4 && parts[1] == QLatin1String("PATH"))
        {
            bool is_count_valid = false;
            const int item_count = parts[2].toInt(&is_count_valid);
            const bool is_align_valid =
                parts[3] == QLatin1String("ALIGN") || parts[3] == QLatin1String("NOALIGN");
            is_success =
                is_count_valid && is_align_valid &&
                viewport->editSelectedPathArray(item_count, parts[3] == QLatin1String("ALIGN"));
        }
        if (!is_success)
        {
            m_command_line->appendMessage(
                tr("ARRAYEDIT 格式：RECT 列 行 列距 行距；POLAR 项目数 填充角；"
                   "PATH 项目数 ALIGN|NOALIGN。"));
        }
        return true;
    }
    if (normalized_command.startsWith(QLatin1String("ARRAYPATH ")) ||
        normalized_command.startsWith(QLatin1String("PA ")))
    {
        bool is_count_valid = false;
        const int item_count = parts.size() == 3 ? parts[1].toInt(&is_count_valid) : 0;
        const bool is_align_value = parts.size() == 3 && (parts[2] == QLatin1String("ALIGN") ||
                                                          parts[2] == QLatin1String("NOALIGN"));
        if (is_count_valid && is_align_value)
        {
            m_workspace->viewport()->setToolMode(SToolMode::ArrayPath);
            m_workspace->viewport()->setPathArrayParameters(item_count,
                                                            parts[2] == QLatin1String("ALIGN"));
        }
        else
        {
            m_command_line->appendMessage(tr("ARRAYPATH 格式无效，例如：ARRAYPATH 6 ALIGN"));
        }
        return true;
    }
    if (normalized_command.startsWith(QLatin1String("ARRAYPOLAR ")) ||
        normalized_command.startsWith(QLatin1String("AP ")))
    {
        bool is_count_valid = false;
        bool is_angle_valid = false;
        const int item_count = parts.size() == 3 ? parts[1].toInt(&is_count_valid) : 0;
        const double fill_angle = parts.size() == 3 ? parts[2].toDouble(&is_angle_valid) : 0.0;
        if (is_count_valid && is_angle_valid)
        {
            m_workspace->viewport()->setToolMode(SToolMode::ArrayPolar);
            m_workspace->viewport()->setPolarArrayParameters(item_count, fill_angle);
        }
        else
        {
            m_command_line->appendMessage(tr("ARRAYPOLAR 格式无效，例如：ARRAYPOLAR 6 360"));
        }
        return true;
    }
    if (normalized_command.startsWith(QLatin1String("ARRAYRECT ")) ||
        normalized_command.startsWith(QLatin1String("AR ")))
    {
        bool is_column_valid = false;
        bool is_row_valid = false;
        const int column_count = parts.size() == 3 ? parts[1].toInt(&is_column_valid) : 0;
        const int row_count = parts.size() == 3 ? parts[2].toInt(&is_row_valid) : 0;
        if (is_column_valid && is_row_valid)
        {
            m_workspace->viewport()->setToolMode(SToolMode::ArrayRect);
            m_workspace->viewport()->setRectangularArrayCounts(column_count, row_count);
        }
        else
        {
            m_command_line->appendMessage(tr("ARRAYRECT 格式无效，例如：ARRAYRECT 4 3"));
        }
        return true;
    }
    return false;
}

} // namespace smartCam
