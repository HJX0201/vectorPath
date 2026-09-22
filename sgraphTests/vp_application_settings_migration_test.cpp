#include "vp_application_settings_migration.h"
#include "vp_test_runner.h"

#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

namespace Vp
{

class VpApplicationSettingsMigrationTest final : public QObject
{
    Q_OBJECT

  private slots:
    void copiesOnlyMissingSettings();
    void isIdempotent();
    void preservesNewerLegacyPrecedence();
    void acceptsEmptyLegacySettings();
    void reportsDestinationWriteFailure();
};

void VpApplicationSettingsMigrationTest::copiesOnlyMissingSettings()
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

void VpApplicationSettingsMigrationTest::isIdempotent()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QSettings legacy(directory.filePath(QStringLiteral("legacy.ini")), QSettings::IniFormat);
    QSettings current(directory.filePath(QStringLiteral("current.ini")), QSettings::IniFormat);
    legacy.setValue(QStringLiteral("shortcuts/draw.line"), QStringLiteral("L"));

    QCOMPARE(migrateMissingApplicationSettings(legacy, current), 1);
    QCOMPARE(migrateMissingApplicationSettings(legacy, current), 0);
}

void VpApplicationSettingsMigrationTest::preservesNewerLegacyPrecedence()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QSettings smart_cad(directory.filePath(QStringLiteral("smartcad.ini")), QSettings::IniFormat);
    QSettings smart_cam(directory.filePath(QStringLiteral("smartcam.ini")), QSettings::IniFormat);
    QSettings vector_path(directory.filePath(QStringLiteral("vectorpath.ini")),
                          QSettings::IniFormat);
    smart_cad.setValue(QStringLiteral("ui/theme"), QStringLiteral("classic"));
    smart_cad.setValue(QStringLiteral("ui/scalePercent"), 100);
    smart_cam.setValue(QStringLiteral("ui/theme"), QStringLiteral("dark"));
    vector_path.setValue(QStringLiteral("ui/scalePercent"), 150);

    QCOMPARE(migrateMissingApplicationSettings(smart_cam, vector_path), 1);
    QCOMPARE(migrateMissingApplicationSettings(smart_cad, vector_path), 0);
    QCOMPARE(vector_path.value(QStringLiteral("ui/theme")).toString(), QStringLiteral("dark"));
    QCOMPARE(vector_path.value(QStringLiteral("ui/scalePercent")).toInt(), 150);
}

void VpApplicationSettingsMigrationTest::acceptsEmptyLegacySettings()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QSettings legacy(directory.filePath(QStringLiteral("legacy.ini")), QSettings::IniFormat);
    QSettings current(directory.filePath(QStringLiteral("current.ini")), QSettings::IniFormat);

    QCOMPARE(migrateMissingApplicationSettings(legacy, current), 0);
    QVERIFY(current.allKeys().isEmpty());
}

void VpApplicationSettingsMigrationTest::reportsDestinationWriteFailure()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QSettings legacy(directory.filePath(QStringLiteral("legacy.ini")), QSettings::IniFormat);
    QSettings current(directory.path(), QSettings::IniFormat);
    legacy.setValue(QStringLiteral("ui/theme"), QStringLiteral("dark"));

    QCOMPARE(migrateMissingApplicationSettings(legacy, current), -1);
}

} // namespace Vp

VECTORPATH_TEST_ENTRY(Vp::VpApplicationSettingsMigrationTest,
                      vpRunVpApplicationSettingsMigrationTest)

#include "vp_application_settings_migration_test.moc"
