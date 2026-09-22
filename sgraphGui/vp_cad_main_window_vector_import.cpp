#include "vp_cad_document.h"
#include "vp_cad_main_window.h"
#include "vp_cad_viewport.h"
#include "vp_cad_workspace_widget.h"
#include "vp_command_line_widget.h"
#include "vp_dialog_service.h"
#include "vp_qt_text.h"
#include "vp_svg_document_operations.h"
#include "vp_vector_document_import.h"
#include "vp_vector_import_dialog.h"

#include <QFile>
#include <QFileDialog>
#include <QImage>
#include <vector>

namespace Vp
{

void VpCadMainWindow::openSvgAsNewDocument()
{
    if (!maybeSave())
    {
        return;
    }
    const QString file_path = QFileDialog::getOpenFileName(
        this, tr("打开 SVG 为新图纸"), {}, tr("SVG 矢量图 (*.svg);;所有文件 (*.*)"));
    if (!file_path.isEmpty())
    {
        loadSvgAsNewDocument(file_path);
    }
}

void VpCadMainWindow::openBitmapAsNewDocument()
{
    if (!maybeSave())
    {
        return;
    }
    const QString file_path = QFileDialog::getOpenFileName(
        this, tr("打开位图并矢量化"), {},
        tr("支持的位图 (*.png *.bmp *.jpg *.jpeg *.tif *.tiff *.webp *.gif);;"
           "所有文件 (*.*)"));
    if (!file_path.isEmpty())
    {
        loadBitmapAsNewDocument(file_path);
    }
}

bool VpCadMainWindow::loadSvgAsNewDocument(const QString& file_path)
{
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly))
    {
        VpDialogService::critical(this, tr("SVG 导入失败"), file.errorString());
        return false;
    }
    VpVectorImportDialog dialog(file.readAll(), this);
    if (dialog.exec() != QDialog::Accepted)
    {
        return false;
    }
    return replaceWithVectorGeometry(dialog.importGeometry(), file_path);
}

bool VpCadMainWindow::loadBitmapAsNewDocument(const QString& file_path)
{
    const QImage bitmap(file_path);
    if (bitmap.isNull())
    {
        VpDialogService::critical(this, tr("位图导入失败"), tr("无法解码所选位图文件。"));
        return false;
    }
    VpVectorImportDialog dialog(bitmap, this);
    if (dialog.exec() != QDialog::Accepted)
    {
        return false;
    }
    return replaceWithVectorGeometry(dialog.importGeometry(), file_path);
}

bool VpCadMainWindow::replaceWithVectorGeometry(const VpVectorImportGeometry& geometry,
                                                const QString& source_description)
{
    if (geometry.entities.empty())
    {
        VpDialogService::critical(this, tr("矢量导入失败"), tr("设置结果中没有可导入的实体。"));
        return false;
    }
    m_document->clear();
    const VpResult<VpVectorDocumentImportReport> result =
        importVectorGeometry(*m_document, geometry);
    if (!result)
    {
        VpDialogService::critical(this, tr("矢量导入失败"), toQtError(result));
        return false;
    }
    m_workspace->viewport()->cancelCommand();
    m_workspace->viewport()->zoomExtents();
    m_command_line->appendMessage(tr("已从 %1 创建新图纸：%2 个原生实体，%3 个颜色图层。")
                                      .arg(source_description)
                                      .arg(result.value().imported_entity_count)
                                      .arg(result.value().created_layer_count));
    for (const QString& warning : geometry.warnings)
    {
        m_command_line->appendMessage(tr("兼容性：%1").arg(warning));
    }
    return true;
}

void VpCadMainWindow::fillImportedSvg()
{
    std::vector<VpEntityId> target_entity_ids;
    const QVector<quint64> selected_entity_ids = m_workspace->viewport()->selectedEntityIds();
    if (selected_entity_ids.size() == 1)
    {
        target_entity_ids.push_back(static_cast<VpEntityId>(selected_entity_ids.front()));
    }
    VpSvgVectorData vector_data = svgColorBlockVectorData(*m_document, target_entity_ids);
    if (vector_data.regions.empty() && !target_entity_ids.empty())
    {
        target_entity_ids.clear();
        vector_data = svgColorBlockVectorData(*m_document);
    }
    if (vector_data.regions.empty())
    {
        VpDialogService::information(this, tr("SVG 填充"), tr("当前图纸没有可填充的 SVG 色块。"));
        return;
    }

    VpVectorImportDialog dialog(std::move(vector_data), this);
    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }
    const VpResult<VpSvgFillReport> result =
        fillSvgColorBlocks(*m_document, dialog.vectorSettings(), target_entity_ids);
    if (!result)
    {
        VpDialogService::critical(this, tr("SVG 填充失败"), toQtError(result));
        return;
    }
    m_workspace->viewport()->update();
    m_command_line->appendMessage(
        tr("SVG 填充完成：处理 %1 个色块，生成 %2 条原生多段线，替换 %3 个旧填充实体。")
            .arg(result.value().source_region_count)
            .arg(result.value().created_polyline_count)
            .arg(result.value().replaced_entity_count));
}

void VpCadMainWindow::deduplicateImportedSvg(VpSvgLayerPriority priority)
{
    const VpResult<VpSvgDeduplicateReport> result =
        deduplicateSvgColorBlocks(*m_document, priority);
    if (!result)
    {
        VpDialogService::critical(this, tr("SVG 去重失败"), toQtError(result));
        return;
    }
    m_workspace->viewport()->update();
    const QString priority_text = priority == VpSvgLayerPriority::UpperFirst
                                      ? tr("优先上层（图层正序）")
                                      : tr("优先下层（图层逆序）");
    m_command_line->appendMessage(
        tr("SVG 去重完成：%1，处理 %2 个图层、%3 个原始色块，得到 %4 个色块。")
            .arg(priority_text)
            .arg(result.value().processed_layer_count)
            .arg(result.value().source_region_count)
            .arg(result.value().result_region_count));
}

} // namespace Vp
