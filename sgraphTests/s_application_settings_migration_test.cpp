#include "s_application_settings_migration.h"

#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

namespace smartCam
{

class SApplicationSettingsMigrationTest final : public QObject
{
    Q_OBJECT

  private slots:
    void copiesOnlyMissingSettings();
    void isIdempotent();
    void acceptsEmptyLegacySettings();
    void reportsDestinationWriteFailure();
};

void SApplicationSettingsMigrationTest::copiesOnlyMissingSettings()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QSettings legacy(directory.filePath(QStringLiteral("legacy.ini")), QSettings::IniFormat);
    QSettings current(directory.filePath(QStringLiteral("current.ini")), QSettings::IniFormat);
    legacy.setValue(QStringLiteral("ui/theme"), QStringLiteral("dark"));
    legacy.setValue(QStringLiteral("ui/scalePercent"), 125);
    current.setValue(QStringLiteral("ui/theme"), QStringLiteral("light"));

    QCOMPARE(migrateMissingApplicationSettings(legacy, current), 1);
    QCOMPARE(current.value(QStringLiteral("ui/theme")).toString(), QStringLiteral("light"));
    QCOMPARE(current.value(QStringLiteral("ui/scalePercent")).toInt(), 125);
}

void SApplicationSettingsMigrationTest::isIdempotent()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QSettings legacy(directory.filePath(QStringLiteral("legacy.ini")), QSettings::IniFormat);
    QSettings current(directory.filePath(QStringLiteral("current.ini")), QSettings::IniFormat);
    legacy.setValue(QStringLiteral("shortcuts/draw.line"), QStringLiteral("L"));

    QCOMPARE(migrateMissingApplicationSettings(legacy, current), 1);
    QCOMPARE(migrateMissingApplicationSettings(legacy, current), 0);
}

void SApplicationSettingsMigrationTest::acceptsEmptyLegacySettings()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QSettings legacy(directory.filePath(QStringLiteral("legacy.ini")), QSettings::IniFormat);
    QSettings current(directory.filePath(QStringLiteral("current.ini")), QSettings::IniFormat);

    QCOMPARE(migrateMissingApplicationSettings(legacy, current), 0);
    QVERIFY(current.allKeys().isEmpty());
}

void SApplicationSettingsMigrationTest::reportsDestinationWriteFailure()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QSettings legacy(directory.filePath(QStringLiteral("legacy.ini")), QSettings::IniFormat);
    QSettings current(directory.path(), QSettings::IniFormat);
    legacy.setValue(QStringLiteral("ui/theme"), QStringLiteral("dark"));

    QCOMPARE(migrateMissingApplicationSettings(legacy, current), -1);
}

} // namespace smartCam

QTEST_APPLESS_MAIN(smartCam::SApplicationSettingsMigrationTest)

#include "s_application_settings_migration_test.moc"
