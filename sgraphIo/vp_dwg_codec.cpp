#include "vp_dwg_codec.h"

#include "vp_dwg_dxf_adapter.h"
#include "vp_dxf_codec.h"
#include "vp_qt_text.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSaveFile>
#include <QTemporaryDir>

namespace Vp
{
namespace
{

QString bundledToolDirectory()
{
    const QString deployed_directory =
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("libredwg"));
    if (QFileInfo::exists(QDir(deployed_directory).filePath(QStringLiteral("dwgread.exe"))))
    {
        return deployed_directory;
    }

    const QString configured_directory =
        QProcessEnvironment::systemEnvironment().value(QStringLiteral("VECTORPATH_LIBREDWG_DIR"));
    if (!configured_directory.isEmpty() &&
        QFileInfo::exists(QDir(configured_directory).filePath(QStringLiteral("dwgread.exe"))))
    {
        return QDir::cleanPath(configured_directory);
    }
    return {};
}

struct VpProcessResult
{
    bool is_success = false;
    QString standard_output;
    QString error_output;
};

VpProcessResult runLibreDwgTool(const QString& executable_name, const QStringList& arguments,
                                int timeout_ms = 120'000)
{
    const QString tool_directory = bundledToolDirectory();
    const QString program = QDir(tool_directory).filePath(executable_name);
    if (tool_directory.isEmpty() || !QFileInfo::exists(program))
    {
        return {false, {}, QObject::tr("未找到 LibreDWG 工具：%1").arg(program)};
    }
    QProcess process;
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("PATH"), tool_directory + QDir::listSeparator() +
                                                   environment.value(QStringLiteral("PATH")));
    process.setProcessEnvironment(environment);
    process.setWorkingDirectory(tool_directory);
    process.start(program, arguments, QIODevice::ReadOnly);
    if (!process.waitForStarted(10'000))
    {
        return {false, {}, QObject::tr("LibreDWG 启动失败：%1").arg(process.errorString())};
    }
    if (!process.waitForFinished(timeout_ms))
    {
        process.kill();
        process.waitForFinished(5'000);
        return {false, {}, QObject::tr("LibreDWG 转换超时。")};
    }
    const QString standard_output = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    const QString error_output = QString::fromUtf8(process.readAllStandardError()).trimmed();
    return {process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0,
            standard_output, error_output};
}

VpResult<void> copyAtomically(const QString& source_path, const QString& target_path)
{
    QFile source_file(source_path);
    if (!source_file.open(QIODevice::ReadOnly))
    {
        return VpResult<void>::failure(
            toCoreText(QObject::tr("无法读取临时 DWG：%1").arg(source_file.errorString())));
    }
    QSaveFile target_file(target_path);
    if (!target_file.open(QIODevice::WriteOnly))
    {
        return VpResult<void>::failure(
            toCoreText(QObject::tr("无法写入 DWG：%1").arg(target_file.errorString())));
    }
    while (!source_file.atEnd())
    {
        const QByteArray block = source_file.read(1024 * 1024);
        if (block.isEmpty() && source_file.error() != QFile::NoError)
        {
            return VpResult<void>::failure(
                toCoreText(QObject::tr("读取临时 DWG 失败：%1").arg(source_file.errorString())));
        }
        if (target_file.write(block) != block.size())
        {
            return VpResult<void>::failure(
                toCoreText(QObject::tr("写入 DWG 失败：%1").arg(target_file.errorString())));
        }
    }
    if (!target_file.commit())
    {
        return VpResult<void>::failure(
            toCoreText(QObject::tr("提交 DWG 失败：%1").arg(target_file.errorString())));
    }
    return VpResult<void>::success();
}

} // namespace

QString VpDwgCodec::id() const
{
    return QStringLiteral("vectorPath.dwg.libredwg");
}

QString VpDwgCodec::displayName() const
{
    return QStringLiteral("AutoCAD DWG（GNU LibreDWG 0.14）");
}

QStringList VpDwgCodec::extensions() const
{
    return {QStringLiteral("dwg")};
}

bool VpDwgCodec::isAvailable() const
{
    const QDir tool_directory(bundledToolDirectory());
    return QFileInfo::exists(tool_directory.filePath(QStringLiteral("dwgread.exe"))) &&
           QFileInfo::exists(tool_directory.filePath(QStringLiteral("dwg2dxf.exe"))) &&
           QFileInfo::exists(tool_directory.filePath(QStringLiteral("dxf2dwg.exe")));
}

QString VpDwgCodec::toolVersion() const
{
    const VpProcessResult result =
        runLibreDwgTool(QStringLiteral("dwgread.exe"), {QStringLiteral("--version")}, 10'000);
    return result.is_success ? result.standard_output : QString();
}

VpResult<VpFileCompatibilityReport> VpDwgCodec::read(const QString& file_path,
                                                     VpCadDocument& document) const
{
    if (!isAvailable())
    {
        return VpResult<VpFileCompatibilityReport>::failure(
            toCoreText(QObject::tr("LibreDWG 0.14 x64 不可用，无法读取 DWG。")));
    }
    QTemporaryDir temporary_directory;
    if (!temporary_directory.isValid())
    {
        return VpResult<VpFileCompatibilityReport>::failure(
            toCoreText(QObject::tr("无法创建 DWG 转换临时目录。")));
    }
    const QString dxf_path = temporary_directory.filePath(QStringLiteral("drawing.dxf"));
    const VpProcessResult conversion =
        runLibreDwgTool(QStringLiteral("dwg2dxf.exe"),
                        {QStringLiteral("-y"), QStringLiteral("--as"), QStringLiteral("r2000"),
                         QStringLiteral("-o"), dxf_path, QFileInfo(file_path).absoluteFilePath()});
    if (!conversion.is_success || !QFileInfo::exists(dxf_path))
    {
        return VpResult<VpFileCompatibilityReport>::failure(
            toCoreText(QObject::tr("DWG 解码失败：%1").arg(conversion.error_output)));
    }
    const VpDxfCodec dxf_codec;
    VpResult<VpFileCompatibilityReport> result = dxf_codec.read(dxf_path, document);
    if (!result)
    {
        return result;
    }
    result.value().warnings.prepend(
        QObject::tr("DWG 已由 GNU LibreDWG 0.14 转换为 DXF 后导入；代理对象和未映射字段可能仅"
                    "保留在原文件中。"));
    if (!conversion.error_output.isEmpty())
    {
        result.value().warnings.append(conversion.error_output.left(2'000));
    }
    return result;
}

VpResult<VpFileCompatibilityReport> VpDwgCodec::write(const QString& file_path,
                                                      const VpCadDocument& document) const
{
    if (!isAvailable())
    {
        return VpResult<VpFileCompatibilityReport>::failure(
            toCoreText(QObject::tr("LibreDWG 0.14 x64 不可用，无法写入 DWG。")));
    }
    QTemporaryDir temporary_directory;
    if (!temporary_directory.isValid())
    {
        return VpResult<VpFileCompatibilityReport>::failure(
            toCoreText(QObject::tr("无法创建 DWG 转换临时目录。")));
    }
    const QString source_dxf_path =
        temporary_directory.filePath(QStringLiteral("drawing_source.dxf"));
    const QString dxf_path = temporary_directory.filePath(QStringLiteral("drawing_r2000.dxf"));
    const QString dwg_path = temporary_directory.filePath(QStringLiteral("drawing.dwg"));
    const VpDxfCodec dxf_codec;
    VpResult<VpFileCompatibilityReport> result = dxf_codec.write(source_dxf_path, document);
    if (!result)
    {
        return result;
    }
    const VpResult<void> preparation_result =
        prepareLibreDwgR2000Dxf(source_dxf_path, dxf_path, document);
    if (!preparation_result)
    {
        return VpResult<VpFileCompatibilityReport>::failure(
            toCoreText(toQtError(preparation_result)));
    }
    const VpProcessResult conversion =
        runLibreDwgTool(QStringLiteral("dxf2dwg.exe"),
                        {QStringLiteral("-y"), QStringLiteral("--as"), QStringLiteral("r2000"),
                         QStringLiteral("-o"), dwg_path, dxf_path});
    if (!conversion.is_success || !QFileInfo::exists(dwg_path))
    {
        return VpResult<VpFileCompatibilityReport>::failure(
            toCoreText(QObject::tr("DWG 编码失败：%1").arg(conversion.error_output)));
    }
    const VpResult<void> copy_result = copyAtomically(dwg_path, file_path);
    if (!copy_result)
    {
        return VpResult<VpFileCompatibilityReport>::failure(toCoreText(toQtError(copy_result)));
    }
    result.value().warnings.prepend(
        QObject::tr("DWG 已按 R2000 格式导出；布局、富文本、关联阵列和代理对象可能已"
                    "降级或展开。"));
    if (!conversion.error_output.isEmpty())
    {
        result.value().warnings.append(conversion.error_output.left(2'000));
    }
    return result;
}

} // namespace Vp
