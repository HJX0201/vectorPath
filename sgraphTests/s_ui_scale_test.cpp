#include "s_theme_manager.h"

#include <QApplication>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>
#include <cmath>

namespace smartGraphics
{

class SUiScaleTest final : public QObject
{
    Q_OBJECT

  private slots:
    void persistsScaleAndUpdatesFont();
};

void SUiScaleTest::persistsScaleAndUpdatesFont()
{
    QTemporaryDir settings_directory;
    QVERIFY(settings_directory.isValid());
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings_directory.path());
    QCoreApplication::setOrganizationName(QStringLiteral("smartCadTest"));
    QCoreApplication::setApplicationName(QStringLiteral("uiScaleTest"));

    SThemeManager first_manager;
    QVERIFY(first_manager.setUiScalePercent(150));
    QCOMPARE(first_manager.uiScalePercent(), 150);
    QVERIFY(std::abs(QApplication::font().pointSizeF() - 13.5) < 0.01);

    SThemeManager restored_manager;
    QCOMPARE(restored_manager.uiScalePercent(), 150);
    QVERIFY(std::abs(QApplication::font().pointSizeF() - 13.5) < 0.01);
    QVERIFY(restored_manager.setUiScalePercent(100));
}

} // namespace smartGraphics

QTEST_MAIN(smartGraphics::SUiScaleTest)
#include "s_ui_scale_test.moc"
