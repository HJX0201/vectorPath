#include "s_command_line_widget.h"
#include "s_command_catalog.h"

#include <QCompleter>
#include <QLineEdit>
#include <QSignalSpy>
#include <QtTest>

namespace smartGraphics
{

class SCommandLineWidgetTest final : public QObject
{
    Q_OBJECT

  private slots:
    void tabCompletionAndHistoryNavigation();
    void removedBlockCommandsAreNotAdvertised();
};

void SCommandLineWidgetTest::tabCompletionAndHistoryNavigation()
{
    SCommandLineWidget widget;
    widget.show();
    auto* input = widget.findChild<QLineEdit*>(QStringLiteral("smartCommandInput"));
    QVERIFY(input != nullptr);
    QVERIFY(input->completer() != nullptr);

    input->setFocus();
    input->setText(QStringLiteral("pli"));
    QTest::keyClick(input, Qt::Key_Tab);
    QCOMPARE(input->text(), QStringLiteral("PLINE"));

    QSignalSpy submitted_spy(&widget, &SCommandLineWidget::commandSubmitted);
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

void SCommandLineWidgetTest::removedBlockCommandsAreNotAdvertised()
{
    const QStringList entries = commandCompletionEntries();
    QVERIFY(!entries.contains(QStringLiteral("BLOCK")));
    QVERIFY(!entries.contains(QStringLiteral("INSERT")));
    QVERIFY(!entries.contains(QStringLiteral("WBLOCK")));
}

} // namespace smartGraphics

QTEST_MAIN(smartGraphics::SCommandLineWidgetTest)
#include "s_command_line_widget_test.moc"
