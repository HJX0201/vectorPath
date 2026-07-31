#include "s_cad_document.h"
#include "s_cad_main_window.h"
#include "s_cad_viewport.h"
#include "s_cad_workspace_widget.h"
#include "s_command_line_widget.h"
#include "s_dialog_service.h"
#include "s_document_recovery_manager.h"
#include "s_dwg_codec.h"
#include "s_dxf_codec.h"

#include <DockManager.h>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>

namespace vectorPath
{
namespace
{

constexpr int kWorkspaceStateVersion = 8;

bool hasNativeDocumentSuffix(const QString& file_path)
{
    return file_path.endsWith(QStringLiteral(".vectorpath"), Qt::CaseInsensitive)
           || file_path.endsWith(QStringLiteral(".smartcam"), Qt::CaseInsensitive)
           || file_path.endsWith(QStringLiteral(".smartcad"), Qt::CaseInsensitive);
}

} // namespace

void SCadMainWindow::newDocument()
{
    if (!maybeSave())
    {
        return;
    }
    m_document->clear();
    m_workspace->viewport()->cancelCommand();
    m_workspace->viewport()->zoomExtents();
}

void SCadMainWindow::openDocument()
{
    if (!maybeSave())
    {
        return;
    }
    const QString file_path =
        QFileDialog::getOpenFileName(
            this, tr("打开图形"), {},
            tr("支持的图形 (*.vectorpath *.smartcam *.smartcad *.dxf *.dwg *.svg *.png *.bmp "
               "*.jpg *.jpeg *.tif *.tiff *.webp *.gif);;"
               "vectorPath 图形 (*.vectorpath *.smartcam *.smartcad);;"
               "DWG 图形 (*.dwg);;DXF 图形 (*.dxf);;SVG 矢量图 (*.svg);;"
               "位图 (*.png *.bmp *.jpg *.jpeg *.tif *.tiff *.webp *.gif);;"
               "所有文件 (*.*)"));
    if (file_path.isEmpty())
    {
        return;
    }
    const QString suffix = QFileInfo(file_path).suffix().toLower();
    if (suffix == QLatin1String("svg"))
    {
        loadSvgAsNewDocument(file_path);
        return;
    }
    const QStringList bitmap_suffixes{
        QStringLiteral("png"), QStringLiteral("bmp"), QStringLiteral("jpg"),
        QStringLiteral("jpeg"), QStringLiteral("tif"), QStringLiteral("tiff"),
        QStringLiteral("webp"), QStringLiteral("gif")};
    if (bitmap_suffixes.contains(suffix))
    {
        loadBitmapAsNewDocument(file_path);
        return;
    }
    if (file_path.endsWith(QStringLiteral(".dxf"), Qt::CaseInsensitive))
    {
        const SDxfCodec codec;
        const SResult<SFileCompatibilityReport> result = codec.read(file_path, *m_document);
        if (!result)
        {
            SDialogService::critical(this, tr("导入失败"), result.errorMessage());
            return;
        }
        m_command_line->appendMessage(tr("已导入 DXF：%1 个实体，跳过 %2 个。")
                                          .arg(result.value().imported_entity_count)
                                          .arg(result.value().skipped_entity_count));
        for (const QString& warning : result.value().warnings)
        {
            m_command_line->appendMessage(tr("警告：%1").arg(warning));
        }
    }
    else if (file_path.endsWith(QStringLiteral(".dwg"), Qt::CaseInsensitive))
    {
        const SDwgCodec codec;
        const SResult<SFileCompatibilityReport> result = codec.read(file_path, *m_document);
        if (!result)
        {
            SDialogService::critical(this, tr("DWG 导入失败"), result.errorMessage());
            return;
        }
        m_command_line->appendMessage(tr("已导入 DWG：%1 个实体，跳过 %2 个。")
                                          .arg(result.value().imported_entity_count)
                                          .arg(result.value().skipped_entity_count));
        for (const QString& warning : result.value().warnings)
        {
            m_command_line->appendMessage(tr("兼容性：%1").arg(warning));
        }
    }
    else
    {
        const SResult<void> result = m_document->load(file_path);
        if (!result)
        {
            SDialogService::critical(this, tr("打开失败"), result.errorMessage());
            return;
        }
        m_command_line->appendMessage(tr("已打开：%1").arg(file_path));
    }
    m_workspace->viewport()->zoomExtents();
}

bool SCadMainWindow::saveDocument()
{
    if (m_document->filePath().isEmpty())
    {
        return saveDocumentAs();
    }
    const SResult<void> result = m_document->save(m_document->filePath());
    if (!result)
    {
        SDialogService::critical(this, tr("保存失败"), result.errorMessage());
        return false;
    }
    m_command_line->appendMessage(tr("已保存：%1").arg(m_document->filePath()));
    m_recovery_manager->discardRecoveriesForSource(m_document->filePath());
    return true;
}

bool SCadMainWindow::saveDocumentAs()
{
    QString file_path = QFileDialog::getSaveFileName(
        this, tr("保存 vectorPath 图形"), m_document->filePath(),
        tr("vectorPath 图形 (*.vectorpath);;兼容 smartCam 图形 (*.smartcam);;"
           "兼容 smartCad 图形 (*.smartcad)"));
    if (file_path.isEmpty())
    {
        return false;
    }
    if (!hasNativeDocumentSuffix(file_path))
    {
        file_path += QStringLiteral(".vectorpath");
    }
    const SResult<void> result = m_document->save(file_path);
    if (!result)
    {
        SDialogService::critical(this, tr("保存失败"), result.errorMessage());
        return false;
    }
    m_command_line->appendMessage(tr("已保存：%1").arg(file_path));
    m_recovery_manager->discardRecoveriesForSource(file_path);
    return true;
}

void SCadMainWindow::exportDxf()
{
    QString file_path = QFileDialog::getSaveFileName(this, tr("导出 DXF"), {},
                                                     tr("AutoCAD DXF ASCII R2018 (*.dxf)"));
    if (file_path.isEmpty())
    {
        return;
    }
    if (!file_path.endsWith(QStringLiteral(".dxf"), Qt::CaseInsensitive))
    {
        file_path += QStringLiteral(".dxf");
    }

    const SDxfCodec codec;
    const SResult<SFileCompatibilityReport> result = codec.write(file_path, *m_document);
    if (!result)
    {
        SDialogService::critical(this, tr("DXF 导出失败"), result.errorMessage());
        return;
    }
    m_command_line->appendMessage(tr("DXF 已导出：%1 个实体，跳过 %2 个。")
                                      .arg(result.value().exported_entity_count)
                                      .arg(result.value().skipped_entity_count));
}

void SCadMainWindow::exportDwg()
{
    QString file_path = QFileDialog::getSaveFileName(this, tr("导出 DWG R2000"), {},
                                                     tr("AutoCAD DWG R2000 (*.dwg)"));
    if (file_path.isEmpty())
    {
        return;
    }
    if (!file_path.endsWith(QStringLiteral(".dwg"), Qt::CaseInsensitive))
    {
        file_path += QStringLiteral(".dwg");
    }
    const SDwgCodec codec;
    const SResult<SFileCompatibilityReport> result = codec.write(file_path, *m_document);
    if (!result)
    {
        SDialogService::critical(this, tr("DWG 导出失败"), result.errorMessage());
        return;
    }
    m_command_line->appendMessage(tr("DWG R2000 已导出：%1 个实体，跳过 %2 个。")
                                      .arg(result.value().exported_entity_count)
                                      .arg(result.value().skipped_entity_count));
    for (const QString& warning : result.value().warnings)
    {
        m_command_line->appendMessage(tr("兼容性：%1").arg(warning));
    }
}

bool SCadMainWindow::maybeSave()
{
    if (!m_document->isModified())
    {
        return true;
    }
    const QMessageBox::StandardButton answer = SDialogService::warning(
        this, tr("保存更改"), tr("当前图形包含未保存的更改。"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (answer == QMessageBox::Save)
    {
        return saveDocument();
    }
    return answer == QMessageBox::Discard;
}

void SCadMainWindow::updateWindowTitle()
{
    QString title = QStringLiteral("%1 — vectorPath").arg(m_document->displayName());
    if (m_document->isModified())
    {
        title.prepend(QLatin1Char('*'));
    }
    setWindowTitle(title);
    m_workspace->updateDocumentTitle();
}

void SCadMainWindow::restoreWorkspace()
{
    QSettings settings;
    restoreGeometry(settings.value(QStringLiteral("mainWindow/geometry")).toByteArray());
    const QByteArray dock_state =
        settings.value(QStringLiteral("workspace/dockState")).toByteArray();
    const int workspace_version = settings.value(QStringLiteral("workspace/version"), 0).toInt();
    bool is_dock_state_restored = false;
    if (!dock_state.isEmpty() && workspace_version == kWorkspaceStateVersion)
    {
        is_dock_state_restored =
            m_dock_manager->restoreState(dock_state, kWorkspaceStateVersion);
        if (!is_dock_state_restored)
        {
            settings.remove(QStringLiteral("workspace/dockState"));
        }
    }
    if (m_property_dock)
    {
        m_property_dock->toggleView(false);
    }
    configureDockSplitters();
    if (!is_dock_state_restored)
    {
        QTimer::singleShot(0, this, &SCadMainWindow::applyDefaultWorkspaceProportions);
    }
    SCadViewport* viewport = m_workspace->viewport();
    viewport->setGridSnapSpacing(
        settings.value(QStringLiteral("drafting/snapSpacing"), 10.0).toDouble());
    viewport->setGridRotation(
        settings.value(QStringLiteral("drafting/gridRotation"), 0.0).toDouble());
    viewport->setGridSnapEnabled(
        settings.value(QStringLiteral("drafting/snapEnabled"), false).toBool());
    viewport->setEndpointSnapEnabled(
        settings.value(QStringLiteral("drafting/endpointSnapEnabled"), true).toBool());
    viewport->setCenterSnapEnabled(
        settings.value(QStringLiteral("drafting/centerSnapEnabled"), true).toBool());
    viewport->setNodeDisplayVisible(
        settings.value(QStringLiteral("display/nodesVisible"), false).toBool());
    viewport->setDirectionDisplayVisible(
        settings.value(QStringLiteral("display/directionsVisible"), false).toBool());
    viewport->setSequenceDisplayVisible(
        settings.value(QStringLiteral("display/sequenceVisible"), false).toBool());
}

void SCadMainWindow::saveWorkspace()
{
    QSettings settings;
    settings.setValue(QStringLiteral("mainWindow/geometry"), saveGeometry());
    settings.setValue(QStringLiteral("workspace/version"), kWorkspaceStateVersion);
    settings.setValue(QStringLiteral("workspace/dockState"),
                      m_dock_manager->saveState(kWorkspaceStateVersion));
    settings.remove(QStringLiteral("workspace/docks"));
    const SCadViewport* viewport = m_workspace->viewport();
    settings.setValue(QStringLiteral("drafting/snapEnabled"), viewport->isGridSnapEnabled());
    settings.setValue(QStringLiteral("drafting/endpointSnapEnabled"),
                      viewport->isEndpointSnapEnabled());
    settings.setValue(QStringLiteral("drafting/centerSnapEnabled"),
                      viewport->isCenterSnapEnabled());
    settings.setValue(QStringLiteral("drafting/snapSpacing"), viewport->gridSnapSpacing());
    settings.setValue(QStringLiteral("drafting/gridRotation"), viewport->gridRotation());
    settings.setValue(QStringLiteral("display/nodesVisible"),
                      viewport->isNodeDisplayVisible());
    settings.setValue(QStringLiteral("display/directionsVisible"),
                      viewport->isDirectionDisplayVisible());
    settings.setValue(QStringLiteral("display/sequenceVisible"),
                      viewport->isSequenceDisplayVisible());
    settings.sync();
}

} // namespace vectorPath
