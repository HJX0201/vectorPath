#include "s_cad_document.h"
#include "s_document_recovery_manager.h"
#include "s_document_transaction.h"

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest>

namespace vectorPath
{

class SDocumentRecoveryTest final : public QObject
{
    Q_OBJECT

  private slots:
    void unnamedDocumentAutosaveAndRestore();
    void namedDocumentKeepsActivePathAndModifiedState();
};

void SDocumentRecoveryTest::unnamedDocumentAutosaveAndRestore()
{
    QTemporaryDir temporary_directory;
    QVERIFY(temporary_directory.isValid());
    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("recovery line"));
    transaction->addLine({0.0, 0.0}, {10.0, 0.0});
    transaction->commit();
    QVERIFY(document.isModified());
    QVERIFY(document.filePath().isEmpty());

    SDocumentRecoveryManager manager(&document);
    manager.setRecoveryDirectory(temporary_directory.path());
    const SResult<QString> autosave_result = manager.autosaveNow();
    QVERIFY(autosave_result.isSuccess());
    QVERIFY(QFileInfo::exists(autosave_result.value()));
    QVERIFY(document.isModified());
    QVERIFY(document.filePath().isEmpty());
    const QVector<SRecoveryEntry> entries = manager.availableRecoveries();
    QCOMPARE(entries.size(), 1);
    QCOMPARE(entries.front().display_name, QStringLiteral("未命名"));

    document.clear();
    QVERIFY(manager.restoreRecovery(entries.front()).isSuccess());
    QCOMPARE(document.entities().size(), std::size_t(1));
    QVERIFY(document.isModified());
    QVERIFY(document.filePath().isEmpty());
    QVERIFY(manager.discardRecovery(entries.front()));
    QVERIFY(manager.availableRecoveries().isEmpty());
}

void SDocumentRecoveryTest::namedDocumentKeepsActivePathAndModifiedState()
{
    QTemporaryDir temporary_directory;
    QVERIFY(temporary_directory.isValid());
    const QString drawing_path = temporary_directory.filePath(QStringLiteral("named.smartcad"));
    const QString recovery_directory = temporary_directory.filePath(QStringLiteral("recovery"));
    SCadDocument document;
    auto initial_transaction = document.beginTransaction(QStringLiteral("initial circle"));
    initial_transaction->addCircle({2.0, 3.0}, 4.0);
    initial_transaction->commit();
    QVERIFY(document.save(drawing_path).isSuccess());
    auto change_transaction = document.beginTransaction(QStringLiteral("unsaved line"));
    change_transaction->addLine({0.0, 0.0}, {5.0, 5.0});
    change_transaction->commit();

    SDocumentRecoveryManager manager(&document);
    manager.setRecoveryDirectory(recovery_directory);
    QVERIFY(manager.autosaveNow().isSuccess());
    QCOMPARE(document.filePath(), drawing_path);
    QVERIFY(document.isModified());
    const QVector<SRecoveryEntry> entries = manager.availableRecoveries();
    QCOMPARE(entries.size(), 1);
    QCOMPARE(entries.front().original_file_path, QFileInfo(drawing_path).absoluteFilePath());

    document.clear();
    QVERIFY(manager.restoreRecovery(entries.front()).isSuccess());
    QCOMPARE(document.filePath(), QFileInfo(drawing_path).absoluteFilePath());
    QCOMPARE(document.entities().size(), std::size_t(2));
    QVERIFY(document.isModified());
    QCOMPARE(manager.discardRecoveriesForSource(drawing_path), 1);
}

} // namespace vectorPath

QTEST_MAIN(vectorPath::SDocumentRecoveryTest)
#include "s_document_recovery_test.moc"
