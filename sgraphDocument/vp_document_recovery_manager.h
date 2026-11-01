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
    // 构造恢复管理器，绑定要监视的文档。
    explicit VpDocumentRecoveryManager(VpCadDocument* document, QObject* parent = nullptr);

    // 设置恢复快照的存放目录。
    void setRecoveryDirectory(QString directory_path);
    // 返回恢复快照目录。
    QString recoveryDirectory() const;
    // 设置自动保存间隔（毫秒）。
    void setAutosaveIntervalMilliseconds(int interval_milliseconds);
    // 返回自动保存间隔。
    int autosaveIntervalMilliseconds() const noexcept;
    // 开始自动保存。
    void start();
    // 停止自动保存。
    void stop();
    // 立即写出一份恢复快照。
    VpResult<QString> autosaveNow();
    // 列出当前可用的恢复快照。
    QVector<VpRecoveryEntry> availableRecoveries() const;
    // 用恢复快照还原文档。
    VpResult<void> restoreRecovery(const VpRecoveryEntry& entry);
    // 丢弃一份恢复快照。
    bool discardRecovery(const VpRecoveryEntry& entry);
    // 丢弃某个源文件的全部恢复快照，返回数量。
    int discardRecoveriesForSource(const QString& original_file_path);

  signals:
    // 写出恢复快照后发出。
    void recoveryCreated(const QString& recovery_file_path);
    // 写恢复快照失败时发出。
    void recoveryFailed(const QString& error_message);

  private:
    // 返回恢复快照的文件名键。
    QString recoveryKey() const;
    // 写出恢复快照的元数据。
    VpResult<void> writeMetadata(const VpRecoveryEntry& entry) const;
    // 按数量上限清理旧的恢复快照。
    void pruneRecoveries(const QString& original_file_path, int maximum_count);

    QPointer<VpCadDocument> m_document;
    QTimer m_timer;
    QString m_recovery_directory;
    QString m_untitled_session_key;
};

} // namespace Vp
