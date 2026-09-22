#include "vp_cad_document.h"
#include "vp_cad_viewport.h"
#include "vp_document_transaction.h"
#include "vp_spline_edit.h"
#include "vp_test_runner.h"

#include <QtTest>

namespace Vp
{

class VpSplineEditTest final : public QObject
{
    Q_OBJECT

  private slots:
    void geometryOperations();
    void viewportCommandsAndUndo();
    void viewportPointPreviewWorkflow();
};

VpEntityRecord splineSource()
{
    VpEntityRecord source;
    source.id = 42;
    source.type = VpEntityType::Spline;
    source.layer_name = QStringLiteral("curve");
    source.line_width_mm = 0.35;
    source.geometry = VpSplineEntity{
        {VpPoint2d{0.0, 0.0}, VpPoint2d{3.0, 8.0}, VpPoint2d{7.0, -8.0}, VpPoint2d{10.0, 0.0}}};
    return source;
}

void VpSplineEditTest::geometryOperations()
{
    const VpEntityRecord source = splineSource();
    VpEntityRecord moved;
    QVERIFY(splineControlPointEntity(source, 1, {4.0, 9.0}, moved));
    QCOMPARE(std::get<VpSplineEntity>(moved.geometry).control_points[1].x, 4.0);
    QCOMPARE(std::get<VpSplineEntity>(moved.geometry).control_points[1].y, 9.0);
    QCOMPARE(moved.layer_name, source.layer_name);
    QCOMPARE(moved.line_width_mm, source.line_width_mm);
    QVERIFY(!splineControlPointEntity(source, 4, {0.0, 0.0}, moved));

    VpEntityRecord reversed;
    QVERIFY(reversedSplineEntity(source, reversed));
    const auto& reversed_points = std::get<VpSplineEntity>(reversed.geometry).control_points;
    QCOMPARE(reversed_points.front().x, 10.0);
    QCOMPARE(reversed_points.back().x, 0.0);

    VpEntityRecord polyline;
    QVERIFY(splinePolylineEntity(source, 32, polyline));
    QCOMPARE(polyline.type, VpEntityType::Polyline);
    QCOMPARE(std::get<VpPolylineEntity>(polyline.geometry).vertices.size(), std::size_t(33));
    QCOMPARE(polyline.layer_name, source.layer_name);
    QCOMPARE(polyline.line_width_mm, source.line_width_mm);
}

void VpSplineEditTest::viewportCommandsAndUndo()
{
    VpCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("spline source"));
    transaction->addSpline(std::get<VpSplineEntity>(splineSource().geometry).control_points);
    transaction->commit();

    VpCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setToolMode(VpToolMode::SplineEdit);
    viewport.submitWorldPoint({0.0, 0.0});
    QVERIFY(viewport.selectedEntityId().has_value());
    QVERIFY(viewport.submitCommandKeyword(QStringLiteral("MOVE 2 4 9")));
    QCOMPARE(std::get<VpSplineEntity>(document.entities().front().geometry).control_points[1].x,
             4.0);
    document.undo();
    QCOMPARE(std::get<VpSplineEntity>(document.entities().front().geometry).control_points[1].x,
             3.0);

    QVERIFY(viewport.submitCommandKeyword(QStringLiteral("REVERSE")));
    QCOMPARE(
        std::get<VpSplineEntity>(document.entities().front().geometry).control_points.front().x,
        10.0);
    QVERIFY(viewport.submitCommandKeyword(QStringLiteral("POLYLINE 24")));
    QCOMPARE(document.entities().front().type, VpEntityType::Polyline);
    QCOMPARE(std::get<VpPolylineEntity>(document.entities().front().geometry).vertices.size(),
             std::size_t(25));
    document.undo();
    QCOMPARE(document.entities().front().type, VpEntityType::Spline);
}

void VpSplineEditTest::viewportPointPreviewWorkflow()
{
    VpCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("spline point edit"));
    transaction->addSpline(std::get<VpSplineEntity>(splineSource().geometry).control_points);
    transaction->commit();

    VpCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setToolMode(VpToolMode::SplineEdit);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({3.0, 8.0});
    viewport.submitWorldPoint({5.0, 9.0});
    const auto& control_points =
        std::get<VpSplineEntity>(document.entities().front().geometry).control_points;
    QCOMPARE(control_points[1].x, 5.0);
    QCOMPARE(control_points[1].y, 9.0);
    document.undo();
    QCOMPARE(std::get<VpSplineEntity>(document.entities().front().geometry).control_points[1].x,
             3.0);
}

} // namespace Vp

VECTORPATH_TEST_ENTRY(Vp::VpSplineEditTest, vpRunVpSplineEditTest)
#include "vp_spline_edit_test.moc"
