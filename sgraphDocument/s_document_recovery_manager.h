#pragma once

#include "s_result.h"

#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVector>

namespace smartCam
{

class SCadDocument;

struct SRecoveryEntry
{
    QString recovery_file_path;
    QString metadata_file_path;
    QString original_file_path;
    QString display_name;
    qint64 created_at_milliseconds = 0;
};

class SDocumentRecoveryManager final : public QObject
{
    Q_OBJECT

  public:
    explicit SDocumentRecoveryManager(SCadDocument* document, QObject* parent = nullptr);

    void setRecoveryDirectory(QString directory_path);
    QString recoveryDirectory() const;
    void setAutosaveIntervalMilliseconds(int interval_milliseconds);
    int autosaveIntervalMilliseconds() const noexcept;
    void start();
    void stop();
    SResult<QString> autosaveNow();
    QVector<SRecoveryEntry> availableRecoveries() const;
    SResult<void> restoreRecovery(const SRecoveryEntry& entry);
    bool discardRecovery(const SRecoveryEntry& entry);
    int discardRecoveriesForSource(const QString& original_file_path);

  signals:
    void recoveryCreated(const QString& recovery_file_path);
    void recoveryFailed(const QString& error_message);

  private:
    QString recoveryKey() const;
    SResult<void> writeMetadata(const SRecoveryEntry& entry) const;
    void pruneRecoveries(const QString& original_file_path, int maximum_count);

    QPointer<SCadDocument> m_document;
    QTimer m_timer;
    QString m_recovery_directory;
    QString m_untitled_session_key;
};

} // namespace smartCam
