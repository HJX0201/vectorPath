#pragma once

#include "vp_result.h"

#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVector>

namespace Vp
{

class VpCadDocument;

// 崩溃恢复快照的文件与元数据描述。
struct VpRecoveryEntry
{
    QString recovery_file_path;
    QString metadata_file_path;
    QString original_file_path;
    QString display_name;
    qint64 created_at_milliseconds = 0;
};

// 定时写出恢复快照，供异常退出后恢复文档。
class VpDocumentRecoveryManager final : public QObject
{
    Q_OBJECT

  public:
    explicit VpDocumentRecoveryManager(VpCadDocument* document, QObject* parent = nullptr);

    void setRecoveryDirectory(QString directory_path);
    QString recoveryDirectory() const;
    void setAutosaveIntervalMilliseconds(int interval_milliseconds);
    int autosaveIntervalMilliseconds() const noexcept;
    void start();
    void stop();
    VpResult<QString> autosaveNow();
    QVector<VpRecoveryEntry> availableRecoveries() const;
    VpResult<void> restoreRecovery(const VpRecoveryEntry& entry);
    bool discardRecovery(const VpRecoveryEntry& entry);
    int discardRecoveriesForSource(const QString& original_file_path);

  signals:
    void recoveryCreated(const QString& recovery_file_path);
    void recoveryFailed(const QString& error_message);

  private:
    QString recoveryKey() const;
    VpResult<void> writeMetadata(const VpRecoveryEntry& entry) const;
    void pruneRecoveries(const QString& original_file_path, int maximum_count);

    QPointer<VpCadDocument> m_document;
    QTimer m_timer;
    QString m_recovery_directory;
    QString m_untitled_session_key;
};

} // namespace Vp
