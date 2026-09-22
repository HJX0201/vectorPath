#include "vp_cad_document.h"
#include "vp_cad_viewport.h"
#include "vp_document_transaction.h"
#include "vp_toolpath_simulation_controller.h"

#include <QtTest>

using namespace Vp;

class VpToolpathSimulationTest final : public QObject
{
    Q_OBJECT

  private slots:
    void playPauseStepAndSpeeds();
    void documentChangeInvalidatesSimulation();
};

void VpToolpathSimulationTest::playPauseStepAndSpeeds()
{
    VpCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("simulation seed"));
    transaction->addLine({0.0, 0.0}, {10.0, 0.0});
    transaction->addLine({20.0, 0.0}, {30.0, 0.0});
    transaction->commit();
    VpCadViewport viewport;
    viewport.resize(640, 480);
    viewport.setDocument(&document);
    VpToolpathSimulationController controller(&document, &viewport);
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
    QCOMPARE(controller.state(), VpSimulationState::Paused);
    controller.play();
    QCOMPARE(controller.state(), VpSimulationState::Running);
    controller.pause();
    QCOMPARE(controller.state(), VpSimulationState::Paused);
    controller.setTraceVisible(false);
    QVERIFY(!controller.isTraceVisible());
    controller.stop();
    QCOMPARE(controller.state(), VpSimulationState::Stopped);
    QCOMPARE(controller.motionCount(), std::size_t(0));
}

void VpToolpathSimulationTest::documentChangeInvalidatesSimulation()
{
    VpCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("simulation seed"));
    transaction->addLine({0.0, 0.0}, {10.0, 0.0});
    transaction->commit();
    VpCadViewport viewport;
    viewport.setDocument(&document);
    VpToolpathSimulationController controller(&document, &viewport);
    controller.step();
    QVERIFY(controller.motionCount() > 0);
    auto change = document.beginTransaction(QStringLiteral("invalidate"));
    change->addLine({20.0, 0.0}, {30.0, 0.0});
    change->commit();
    QCOMPARE(controller.state(), VpSimulationState::Stopped);
    QCOMPARE(controller.motionCount(), std::size_t(0));
}

QTEST_MAIN(VpToolpathSimulationTest)
#include "vp_toolpath_simulation_test.moc"
