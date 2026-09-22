#include "vp_document_recovery_manager.h"

#include "vp_cad_document.h"
#include "vp_qt_text.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUuid>
#include <algorithm>

namespace Vp
{
namespace
{

constexpr int kDefaultAutosaveIntervalMilliseconds = 5 * 60 * 1000;

QString normalizedSourcePath(const QString& file_path)
{
    return file_path.isEmpty() ? QString() : QFileInfo(file_path).absoluteFilePath();
}

} // namespace

VpDocumentRecoveryManager::VpDocumentRecoveryManager(VpCadDocument* document, QObject* parent)
    : QObject(parent), m_document(document),
      m_recovery_directory(
          QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
              .filePath(QStringLiteral("recovery"))),
      m_untitled_session_key(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    m_timer.setInterval(kDefaultAutosaveIntervalMilliseconds);
    m_timer.setSingleShot(false);
    connect(&m_timer, &QTimer::timeout, this,
            [this]()
            {
                const VpResult<QString> result = autosaveNow();
                if (!result)
                {
                    emit recoveryFailed(toQtError(result));
                }
            });
}

void VpDocumentRecoveryManager::setRecoveryDirectory(QString directory_path)
{
    m_recovery_directory = QDir::cleanPath(std::move(directory_path));
}

QString VpDocumentRecoveryManager::recoveryDirectory() const
{
    return m_recovery_directory;
}

void VpDocumentRecoveryManager::setAutosaveIntervalMilliseconds(int interval_milliseconds)
{
    m_timer.setInterval(std::clamp(interval_milliseconds, 1000, 24 * 60 * 60 * 1000));
}

int VpDocumentRecoveryManager::autosaveIntervalMilliseconds() const noexcept
{
    return m_timer.interval();
}

void VpDocumentRecoveryManager::start()
{
    m_timer.start();
}

void VpDocumentRecoveryManager::stop()
{
    m_timer.stop();
}

QString VpDocumentRecoveryManager::recoveryKey() const
{
    if (!m_document || m_document->filePath().isEmpty())
    {
        return QStringLiteral("untitled_%1").arg(m_untitled_session_key);
    }
    const QByteArray digest =
        QCryptographicHash::hash(normalizedSourcePath(m_document->filePath()).toUtf8(),
                                 QCryptographicHash::Sha256)
            .toHex()
            .left(20);
    return QString::fromLatin1(digest);
}

VpResult<void> VpDocumentRecoveryManager::writeMetadata(const VpRecoveryEntry& entry) const
{
    QJsonObject metadata;
    metadata.insert(QStringLiteral("schemaVersion"), 1);
    metadata.insert(QStringLiteral("recoveryFile"), entry.recovery_file_path);
    metadata.insert(QStringLiteral("originalFile"), entry.original_file_path);
    metadata.insert(QStringLiteral("displayName"), entry.display_name);
    metadata.insert(QStringLiteral("createdAtMs"), entry.created_at_milliseconds);
    QSaveFile file(entry.metadata_file_path);
    if (!file.open(QIODevice::WriteOnly))
    {
        return VpResult<void>::failure(
            toCoreText(tr("无法写入恢复索引：%1").arg(file.errorString())));
    }
    const QByteArray data = QJsonDocument(metadata).toJson(QJsonDocument::Indented);
    if (file.write(data) != data.size() || !file.commit())
    {
        return VpResult<void>::failure(
            toCoreText(tr("恢复索引写入失败：%1").arg(file.errorString())));
    }
    return VpResult<void>::success();
}

VpResult<QString> VpDocumentRecoveryManager::autosaveNow()
{
    if (!m_document)
    {
        return VpResult<QString>::failure(toCoreText(tr("没有可自动保存的活动文档。")));
    }
    if (!m_document->isModified())
    {
        return VpResult<QString>::success({});
    }
    QDir directory(m_recovery_directory);
    if (!directory.exists() && !directory.mkpath(QStringLiteral(".")))
    {
        return VpResult<QString>::failure(
            toCoreText(tr("无法创建恢复目录：%1").arg(m_recovery_directory)));
    }
    const qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    const QString base_name = QStringLiteral("%1_%2").arg(recoveryKey()).arg(timestamp);
    VpRecoveryEntry entry;
    entry.recovery_file_path = directory.filePath(base_name + QStringLiteral(".vectorpath.sv$"));
    entry.metadata_file_path = directory.filePath(base_name + QStringLiteral(".recovery.json"));
    entry.original_file_path = normalizedSourcePath(m_document->filePath());
    entry.display_name = m_document->displayName();
    entry.created_at_milliseconds = timestamp;
    const VpResult<void> save_result = m_document->saveRecoveryCopy(entry.recovery_file_path);
    if (!save_result)
    {
        return VpResult<QString>::failure(toCoreText(toQtError(save_result)));
    }
    const VpResult<void> metadata_result = writeMetadata(entry);
    if (!metadata_result)
    {
        QFile::remove(entry.recovery_file_path);
        return VpResult<QString>::failure(toCoreText(toQtError(metadata_result)));
    }
    pruneRecoveries(entry.original_file_path, 5);
    emit recoveryCreated(entry.recovery_file_path);
    return VpResult<QString>::success(entry.recovery_file_path);
}

QVector<VpRecoveryEntry> VpDocumentRecoveryManager::availableRecoveries() const
{
    QVector<VpRecoveryEntry> entries;
    const QDir directory(m_recovery_directory);
    const QStringList metadata_files =
        directory.entryList({QStringLiteral("*.recovery.json")}, QDir::Files, QDir::Time);
    for (const QString& metadata_name : metadata_files)
    {
        const QString metadata_path = directory.filePath(metadata_name);
        QFile file(metadata_path);
        if (!file.open(QIODevice::ReadOnly))
        {
            continue;
        }
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
        if (!document.isObject())
        {
            continue;
        }
        const QJsonObject metadata = document.object();
        VpRecoveryEntry entry;
        entry.metadata_file_path = metadata_path;
        entry.recovery_file_path = metadata.value(QStringLiteral("recoveryFile")).toString();
        entry.original_file_path = metadata.value(QStringLiteral("originalFile")).toString();
        entry.display_name = metadata.value(QStringLiteral("displayName")).toString();
        entry.created_at_milliseconds =
            static_cast<qint64>(metadata.value(QStringLiteral("createdAtMs")).toDouble());
        if (!entry.recovery_file_path.isEmpty() && QFileInfo::exists(entry.recovery_file_path))
        {
            entries.push_back(std::move(entry));
        }
    }
    std::sort(entries.begin(), entries.end(),
              [](const VpRecoveryEntry& first_entry, const VpRecoveryEntry& second_entry)
              {
                  return first_entry.created_at_milliseconds > second_entry.created_at_milliseconds;
              });
    return entries;
}

VpResult<void> VpDocumentRecoveryManager::restoreRecovery(const VpRecoveryEntry& entry)
{
    if (!m_document)
    {
        return VpResult<void>::failure(toCoreText(tr("没有可恢复的活动文档。")));
    }
    return m_document->recover(entry.recovery_file_path, entry.original_file_path);
}

bool VpDocumentRecoveryManager::discardRecovery(const VpRecoveryEntry& entry)
{
    const bool removed_recovery =
        !QFileInfo::exists(entry.recovery_file_path) || QFile::remove(entry.recovery_file_path);
    const bool removed_metadata =
        !QFileInfo::exists(entry.metadata_file_path) || QFile::remove(entry.metadata_file_path);
    return removed_recovery && removed_metadata;
}

int VpDocumentRecoveryManager::discardRecoveriesForSource(const QString& original_file_path)
{
    const QString normalized_path = normalizedSourcePath(original_file_path);
    int discarded_count = 0;
    for (const VpRecoveryEntry& entry : availableRecoveries())
    {
        if (entry.original_file_path == normalized_path && discardRecovery(entry))
        {
            ++discarded_count;
        }
    }
    return discarded_count;
}

void VpDocumentRecoveryManager::pruneRecoveries(const QString& original_file_path,
                                                int maximum_count)
{
    int matching_index = 0;
    for (const VpRecoveryEntry& entry : availableRecoveries())
    {
        if (entry.original_file_path != original_file_path)
        {
            continue;
        }
        ++matching_index;
        if (matching_index > maximum_count)
        {
            discardRecovery(entry);
        }
    }
}

} // namespace Vp
