#include "s_cad_document.h"
#include "s_cad_main_window.h"
#include "s_cad_viewport.h"
#include "s_cad_workspace_widget.h"
#include "s_command_line_widget.h"
#include "s_svg_document_operations.h"
#include "s_theme_manager.h"

namespace vectorPath
{

void SCadMainWindow::executeCommand(const QString& command)
{
    if (executeCoordinateInput(command))
    {
        return;
    }
    const QString simplified_command = command.simplified();
    const QString normalized = simplified_command.toUpper();
    if (m_workspace->viewport()->submitCommandKeyword(normalized))
    {
        return;
    }
    if (executeAnnotationCommand(simplified_command))
    {
        return;
    }
    if (executeDimensionStyleCommand(simplified_command))
    {
        return;
    }
    if (executeHatchCommand(simplified_command))
    {
        return;
    }
    if (executeDraftingCommand(normalized))
    {
        return;
    }
    if (executeGripCommand(normalized))
    {
        return;
    }
    if (executeBooleanCommand(normalized))
    {
        return;
    }
    if (executeLayerCommand(simplified_command) || executeArrayParameterCommand(normalized))
    {
        return;
    }
    if (executeOutputCommand(simplified_command))
    {
        return;
    }
    if (executeShortcutCommand(simplified_command))
    {
        return;
    }
    if (executeViewCommand(normalized))
    {
        return;
    }
    if (executeSimulationCommand(normalized))
    {
        return;
    }
    if (executeQuickOperationCommand(normalized))
    {
        return;
    }
    if (normalized.startsWith(QLatin1String("CHAMFER D ")))
    {
        const QStringList parts = normalized.split(QLatin1Char(' '), QString::SkipEmptyParts);
        bool is_first_valid = false;
        bool is_second_valid = false;
        const double first_distance = parts.size() == 4 ? parts[2].toDouble(&is_first_valid) : 0.0;
        const double second_distance =
            parts.size() == 4 ? parts[3].toDouble(&is_second_valid) : 0.0;
        if (is_first_valid && is_second_valid)
        {
            m_workspace->viewport()->setToolMode(SToolMode::Chamfer);
            m_workspace->viewport()->setChamferDistances(first_distance, second_distance);
        }
        else
        {
            m_command_line->appendMessage(tr("CHAMFER 距离格式无效，例如：CHAMFER D 5 5"));
        }
        return;
    }
    if (normalized.startsWith(QLatin1String("FILLET R ")))
    {
        bool is_radius_valid = false;
        const double radius = normalized.mid(9).trimmed().toDouble(&is_radius_valid);
        if (is_radius_valid)
        {
            m_workspace->viewport()->setToolMode(SToolMode::Fillet);
            m_workspace->viewport()->setFilletRadius(radius);
        }
        else
        {
            m_command_line->appendMessage(tr("FILLET 半径格式无效，例如：FILLET R 5"));
        }
        return;
    }
    if (normalized.startsWith(QLatin1String("PEDIT ")) ||
        normalized.startsWith(QLatin1String("PE ")))
    {
        const int command_length = normalized.startsWith(QLatin1String("PEDIT ")) ? 5 : 2;
        m_workspace->viewport()->setToolMode(SToolMode::PolylineEdit);
        m_workspace->viewport()->submitCommandKeyword(normalized.mid(command_length).trimmed());
        return;
    }
    if (normalized.startsWith(QLatin1String("SPLINEDIT ")) ||
        normalized.startsWith(QLatin1String("SPE ")))
    {
        const int command_length = normalized.startsWith(QLatin1String("SPLINEDIT ")) ? 10 : 3;
        m_workspace->viewport()->setToolMode(SToolMode::SplineEdit);
        m_workspace->viewport()->submitCommandKeyword(normalized.mid(command_length).trimmed());
        return;
    }
    if (normalized == QLatin1String("LINE") || normalized == QLatin1String("L"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::Line);
    }
    else if (normalized == QLatin1String("CIRCLE") || normalized == QLatin1String("C"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::Circle);
    }
    else if (normalized == QLatin1String("PLINE") || normalized == QLatin1String("PL"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::Polyline);
    }
    else if (normalized == QLatin1String("PEDIT") || normalized == QLatin1String("PE"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::PolylineEdit);
    }
    else if (normalized == QLatin1String("SPLINEDIT") || normalized == QLatin1String("SPE"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::SplineEdit);
    }
    else if (normalized == QLatin1String("ARC") || normalized == QLatin1String("A"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::Arc);
    }
    else if (normalized == QLatin1String("RECTANG") || normalized == QLatin1String("REC"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::Rectangle);
    }
    else if (normalized == QLatin1String("MOVE") || normalized == QLatin1String("M"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::Move);
    }
    else if (normalized == QLatin1String("COPY") || normalized == QLatin1String("CO") ||
             normalized == QLatin1String("CP"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::Copy);
    }
    else if (normalized == QLatin1String("ROTATE") || normalized == QLatin1String("RO"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::Rotate);
    }
    else if (normalized == QLatin1String("SCALE") || normalized == QLatin1String("SC"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::Scale);
    }
    else if (normalized == QLatin1String("MIRROR") || normalized == QLatin1String("MI"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::Mirror);
    }
    else if (normalized == QLatin1String("ERASE") || normalized == QLatin1String("E") ||
             normalized == QLatin1String("DELETE"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::Erase);
    }
    else if (normalized == QLatin1String("OFFSET") || normalized == QLatin1String("O"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::Offset);
    }
    else if (normalized == QLatin1String("TRIM") || normalized == QLatin1String("TR"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::Trim);
    }
    else if (normalized == QLatin1String("EXTEND") || normalized == QLatin1String("EX"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::Extend);
    }
    else if (normalized == QLatin1String("BREAK") || normalized == QLatin1String("BR"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::Break);
    }
    else if (normalized == QLatin1String("JOIN") || normalized == QLatin1String("J"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::Join);
    }
    else if (normalized == QLatin1String("EXPLODE") || normalized == QLatin1String("X"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::Explode);
    }
    else if (normalized == QLatin1String("STRETCH") || normalized == QLatin1String("S"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::Stretch);
    }
    else if (normalized == QLatin1String("LENGTHEN") || normalized == QLatin1String("LEN"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::Lengthen);
    }
    else if (normalized == QLatin1String("FILLET") || normalized == QLatin1String("F"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::Fillet);
    }
    else if (normalized == QLatin1String("CHAMFER") || normalized == QLatin1String("CHA"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::Chamfer);
    }
    else if (normalized == QLatin1String("BLEND") || normalized == QLatin1String("BL"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::Blend);
    }
    else if (normalized == QLatin1String("ALIGN") || normalized == QLatin1String("AL"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::Align);
    }
    else if (normalized == QLatin1String("ARRAYRECT") || normalized == QLatin1String("AR"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::ArrayRect);
    }
    else if (normalized == QLatin1String("ARRAYPOLAR") || normalized == QLatin1String("AP"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::ArrayPolar);
    }
    else if (normalized == QLatin1String("ARRAYPATH") || normalized == QLatin1String("PA"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::ArrayPath);
    }
    else if (normalized == QLatin1String("ARRAYEDIT") || normalized == QLatin1String("AE"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::ArrayEdit);
    }
    else if (normalized == QLatin1String("TEXT") || normalized == QLatin1String("T"))
    {
        beginTextCommand();
    }
    else if (normalized == QLatin1String("DIMLINEAR") || normalized == QLatin1String("DLI"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::LinearDimension);
    }
    else if (normalized == QLatin1String("DIMALIGNED") || normalized == QLatin1String("DAL"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::AlignedDimension);
    }
    else if (normalized == QLatin1String("DIMANGULAR") || normalized == QLatin1String("DAN"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::AngularDimension);
    }
    else if (normalized == QLatin1String("DIMRADIUS") || normalized == QLatin1String("DRA"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::RadiusDimension);
    }
    else if (normalized == QLatin1String("DIMDIAMETER") || normalized == QLatin1String("DDI"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::DiameterDimension);
    }
    else if (normalized == QLatin1String("DIMARC") || normalized == QLatin1String("DAR"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::ArcLengthDimension);
    }
    else if (normalized == QLatin1String("DIMORDINATE") || normalized == QLatin1String("DOR"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::OrdinateDimension);
    }
    else if (normalized == QLatin1String("HATCH") || normalized == QLatin1String("H"))
    {
        m_workspace->viewport()->setToolMode(SToolMode::Hatch);
    }
    else if (normalized == QLatin1String("UNDO") || normalized == QLatin1String("U"))
    {
        m_document->undo();
    }
    else if (normalized == QLatin1String("REDO"))
    {
        m_document->redo();
    }
    else if (normalized == QLatin1String("GRID ON"))
    {
        m_workspace->viewport()->setGridVisible(true);
    }
    else if (normalized == QLatin1String("GRID OFF"))
    {
        m_workspace->viewport()->setGridVisible(false);
    }
    else if (normalized == QLatin1String("OSNAP ON"))
    {
        m_workspace->viewport()->setObjectSnapEnabled(true);
    }
    else if (normalized == QLatin1String("OSNAP OFF"))
    {
        m_workspace->viewport()->setObjectSnapEnabled(false);
    }
    else if (normalized == QLatin1String("ORTHO ON"))
    {
        m_workspace->viewport()->setOrthoEnabled(true);
    }
    else if (normalized == QLatin1String("ORTHO OFF"))
    {
        m_workspace->viewport()->setOrthoEnabled(false);
    }
    else if (normalized == QLatin1String("OTRACK ON"))
    {
        m_workspace->viewport()->setTrackingEnabled(true);
    }
    else if (normalized == QLatin1String("OTRACK OFF"))
    {
        m_workspace->viewport()->setTrackingEnabled(false);
    }
    else if (normalized == QLatin1String("NEW"))
    {
        newDocument();
    }
    else if (normalized == QLatin1String("OPEN"))
    {
        openDocument();
    }
    else if (normalized == QLatin1String("IMPORTSVG") ||
             normalized == QLatin1String("SVGIMPORT"))
    {
        openSvgAsNewDocument();
    }
    else if (normalized == QLatin1String("IMPORTBITMAP") ||
             normalized == QLatin1String("BITMAPIMPORT") ||
             normalized == QLatin1String("IMAGEIMPORT"))
    {
        openBitmapAsNewDocument();
    }
    else if (normalized == QLatin1String("SVGFILL"))
    {
        fillImportedSvg();
    }
    else if (normalized == QLatin1String("SVGDEDUP") ||
             normalized == QLatin1String("SVGDEDUP UPPER"))
    {
        deduplicateImportedSvg(SSvgLayerPriority::UpperFirst);
    }
    else if (normalized == QLatin1String("SVGDEDUP LOWER"))
    {
        deduplicateImportedSvg(SSvgLayerPriority::LowerFirst);
    }
    else if (normalized == QLatin1String("SAVE"))
    {
        saveDocument();
    }
    else if (normalized == QLatin1String("DXFOUT"))
    {
        exportDxf();
    }
    else if (normalized == QLatin1String("DWGOUT") || normalized == QLatin1String("EXPORTDWG"))
    {
        exportDwg();
    }
    else if (normalized == QLatin1String("DWGIN"))
    {
        openDocument();
    }
    else if (normalized == QLatin1String("RECOVER") ||
             normalized == QLatin1String("DRAWINGRECOVERY"))
    {
        showRecoveryManager();
    }
    else if (normalized == QLatin1String("AUTOSAVE"))
    {
        createRecoveryCopy();
    }
    else if (normalized == QLatin1String("AUDIT") || normalized == QLatin1String("AUDIT FIX"))
    {
        runDocumentAudit(true);
    }
    else if (normalized == QLatin1String("AUDIT CHECK"))
    {
        runDocumentAudit(false);
    }
    else if (normalized == QLatin1String("THEME DARK"))
    {
        m_theme_manager.applyTheme(SThemeMode::Dark);
    }
    else if (normalized == QLatin1String("THEME LIGHT"))
    {
        m_theme_manager.applyTheme(SThemeMode::Light);
    }
    else if (normalized == QLatin1String("THEME HIGH"))
    {
        m_theme_manager.applyTheme(SThemeMode::HighContrast);
    }
    else
    {
        m_command_line->appendMessage(tr("未知命令：%1").arg(command));
    }
}

} // namespace vectorPath
