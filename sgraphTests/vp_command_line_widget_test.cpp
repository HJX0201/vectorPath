#include "vp_command_catalog.h"
#include "vp_command_line_widget.h"
#include "vp_test_runner.h"

#include <QCompleter>
#include <QLineEdit>
#include <QSignalSpy>
#include <QtTest>

namespace Vp
{

class VpCommandLineWidgetTest final : public QObject
{
    Q_OBJECT

  private slots:
    void tabCompletionAndHistoryNavigation();
    void removedBlockCommandsAreNotAdvertised();
};

void VpCommandLineWidgetTest::tabCompletionAndHistoryNavigation()
{
    VpCommandLineWidget widget;
    widget.show();
    auto* input = widget.findChild<QLineEdit*>(QStringLiteral("smartCommandInput"));
    QVERIFY(input != nullptr);
    QVERIFY(input->completer() != nullptr);

    input->setFocus();
    input->setText(QStringLiteral("pli"));
    QTest::keyClick(input, Qt::Key_Tab);
    QCOMPARE(input->text(), QStringLiteral("PLINE"));

    QSignalSpy submitted_spy(&widget, &VpCommandLineWidget::commandSubmitted);
    input->setText(QStringLiteral("LINE"));
    QTest::keyClick(input, Qt::Key_Return);
    QCOMPARE(submitted_spy.count(), 1);
    QCOMPARE(submitted_spy.takeFirst().front().toString(), QStringLiteral("LINE"));
    QVERIFY(input->text().isEmpty());

    input->setText(QStringLiteral("CIRCLE"));
    QTest::keyClick(input, Qt::Key_Return);
    QTest::keyClick(input, Qt::Key_Up);
    QCOMPARE(input->text(), QStringLiteral("CIRCLE"));
    QTest::keyClick(input, Qt::Key_Up);
    QCOMPARE(input->text(), QStringLiteral("LINE"));
    QTest::keyClick(input, Qt::Key_Down);
    QCOMPARE(input->text(), QStringLiteral("CIRCLE"));
}

void VpCommandLineWidgetTest::removedBlockCommandsAreNotAdvertised()
{
    const QStringList entries = commandCompletionEntries();
    QVERIFY(!entries.contains(QStringLiteral("BLOCK")));
    QVERIFY(!entries.contains(QStringLiteral("INSERT")));
    QVERIFY(!entries.contains(QStringLiteral("WBLOCK")));
}

} // namespace Vp

VECTORPATH_TEST_ENTRY(Vp::VpCommandLineWidgetTest, vpRunVpCommandLineWidgetTest)
#include "vp_command_line_widget_test.moc"
