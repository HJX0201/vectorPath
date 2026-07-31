#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_document_transaction.h"
#include "s_toolpath_simulation_controller.h"

#include <QtTest>

using namespace smartCam;

class SToolpathSimulationTest final : public QObject
{
    Q_OBJECT

  private slots:
    void playPauseStepAndSpeeds();
    void documentChangeInvalidatesSimulation();
};

void SToolpathSimulationTest::playPauseStepAndSpeeds()
{
    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("simulation seed"));
    transaction->addLine({0.0, 0.0}, {10.0, 0.0});
    transaction->addLine({20.0, 0.0}, {30.0, 0.0});
    transaction->commit();
    SCadViewport viewport;
    viewport.resize(640, 480);
    viewport.setDocument(&document);
    SToolpathSimulationController controller(&document, &viewport);
    QCOMPARE(controller.speedMultiplier(), 1.0);
    controller.speedDown();
    QCOMPARE(controller.speedMultiplier(), 0.5);
    controller.speedDown();
    QCOMPARE(controller.speedMultiplier(), 0.25);
    controller.speedDown();
    QCOMPARE(controller.speedMultiplier(), 0.25);
    controller.speedUp();
    controller.speedUp();
    controller.speedUp();
    controller.speedUp();
    controller.speedUp();
    QCOMPARE(controller.speedMultiplier(), 4.0);
    controller.step();
    QVERIFY(controller.motionCount() >= 4);
    QCOMPARE(controller.completedMotionCount(), std::size_t(1));
    QCOMPARE(controller.state(), SSimulationState::Paused);
    controller.play();
    QCOMPARE(controller.state(), SSimulationState::Running);
    controller.pause();
    QCOMPARE(controller.state(), SSimulationState::Paused);
    controller.setTraceVisible(false);
    QVERIFY(!controller.isTraceVisible());
    controller.stop();
    QCOMPARE(controller.state(), SSimulationState::Stopped);
    QCOMPARE(controller.motionCount(), std::size_t(0));
}

void SToolpathSimulationTest::documentChangeInvalidatesSimulation()
{
    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("simulation seed"));
    transaction->addLine({0.0, 0.0}, {10.0, 0.0});
    transaction->commit();
    SCadViewport viewport;
    viewport.setDocument(&document);
    SToolpathSimulationController controller(&document, &viewport);
    controller.step();
    QVERIFY(controller.motionCount() > 0);
    auto change = document.beginTransaction(QStringLiteral("invalidate"));
    change->addLine({20.0, 0.0}, {30.0, 0.0});
    change->commit();
    QCOMPARE(controller.state(), SSimulationState::Stopped);
    QCOMPARE(controller.motionCount(), std::size_t(0));
}

QTEST_MAIN(SToolpathSimulationTest)
#include "s_toolpath_simulation_test.moc"
