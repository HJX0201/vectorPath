#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_document_transaction.h"
#include "s_spline_edit.h"

#include <QtTest>

namespace smartGraphics
{

class SSplineEditTest final : public QObject
{
    Q_OBJECT

  private slots:
    void geometryOperations();
    void viewportCommandsAndUndo();
    void viewportPointPreviewWorkflow();
};

SEntityRecord splineSource()
{
    SEntityRecord source;
    source.id = 42;
    source.type = SEntityType::Spline;
    source.layer_name = QStringLiteral("curve");
    source.line_width_mm = 0.35;
    source.geometry = SSplineEntity{
        {SPoint2d{0.0, 0.0}, SPoint2d{3.0, 8.0}, SPoint2d{7.0, -8.0}, SPoint2d{10.0, 0.0}}};
    return source;
}

void SSplineEditTest::geometryOperations()
{
    const SEntityRecord source = splineSource();
    SEntityRecord moved;
    QVERIFY(splineControlPointEntity(source, 1, {4.0, 9.0}, moved));
    QCOMPARE(std::get<SSplineEntity>(moved.geometry).control_points[1].x, 4.0);
    QCOMPARE(std::get<SSplineEntity>(moved.geometry).control_points[1].y, 9.0);
    QCOMPARE(moved.layer_name, source.layer_name);
    QCOMPARE(moved.line_width_mm, source.line_width_mm);
    QVERIFY(!splineControlPointEntity(source, 4, {0.0, 0.0}, moved));

    SEntityRecord reversed;
    QVERIFY(reversedSplineEntity(source, reversed));
    const auto& reversed_points = std::get<SSplineEntity>(reversed.geometry).control_points;
    QCOMPARE(reversed_points.front().x, 10.0);
    QCOMPARE(reversed_points.back().x, 0.0);

    SEntityRecord polyline;
    QVERIFY(splinePolylineEntity(source, 32, polyline));
    QCOMPARE(polyline.type, SEntityType::Polyline);
    QCOMPARE(std::get<SPolylineEntity>(polyline.geometry).vertices.size(), std::size_t(33));
    QCOMPARE(polyline.layer_name, source.layer_name);
    QCOMPARE(polyline.line_width_mm, source.line_width_mm);
}

void SSplineEditTest::viewportCommandsAndUndo()
{
    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("spline source"));
    transaction->addSpline(std::get<SSplineEntity>(splineSource().geometry).control_points);
    transaction->commit();

    SCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setToolMode(SToolMode::SplineEdit);
    viewport.submitWorldPoint({0.0, 0.0});
    QVERIFY(viewport.selectedEntityId().has_value());
    QVERIFY(viewport.submitCommandKeyword(QStringLiteral("MOVE 2 4 9")));
    QCOMPARE(std::get<SSplineEntity>(document.entities().front().geometry).control_points[1].x,
             4.0);
    document.undo();
    QCOMPARE(std::get<SSplineEntity>(document.entities().front().geometry).control_points[1].x,
             3.0);

    QVERIFY(viewport.submitCommandKeyword(QStringLiteral("REVERSE")));
    QCOMPARE(std::get<SSplineEntity>(document.entities().front().geometry).control_points.front().x,
             10.0);
    QVERIFY(viewport.submitCommandKeyword(QStringLiteral("POLYLINE 24")));
    QCOMPARE(document.entities().front().type, SEntityType::Polyline);
    QCOMPARE(std::get<SPolylineEntity>(document.entities().front().geometry).vertices.size(),
             std::size_t(25));
    document.undo();
    QCOMPARE(document.entities().front().type, SEntityType::Spline);
}

void SSplineEditTest::viewportPointPreviewWorkflow()
{
    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("spline point edit"));
    transaction->addSpline(std::get<SSplineEntity>(splineSource().geometry).control_points);
    transaction->commit();

    SCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setToolMode(SToolMode::SplineEdit);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({3.0, 8.0});
    viewport.submitWorldPoint({5.0, 9.0});
    const auto& control_points =
        std::get<SSplineEntity>(document.entities().front().geometry).control_points;
    QCOMPARE(control_points[1].x, 5.0);
    QCOMPARE(control_points[1].y, 9.0);
    document.undo();
    QCOMPARE(std::get<SSplineEntity>(document.entities().front().geometry).control_points[1].x,
             3.0);
}

} // namespace smartGraphics

QTEST_MAIN(smartGraphics::SSplineEditTest)
#include "s_spline_edit_test.moc"
