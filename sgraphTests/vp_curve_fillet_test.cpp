#include "vp_cad_document.h"
#include "vp_cad_viewport.h"
#include "vp_cad_viewport_geometry.h"
#include "vp_document_transaction.h"

#include <QtTest>

namespace Vp
{

class VpCurveFilletTest final : public QObject
{
    Q_OBJECT

  private slots:
    void lineCircleFilletGeometryAndUndo();
    void curveCurveFilletGeometry();
    void lineArcChamferGeometryAndUndo();
    void arcArcChamferGeometry();
};

void VpCurveFilletTest::lineCircleFilletGeometryAndUndo()
{
    VpEntityRecord line;
    line.id = 200;
    line.type = VpEntityType::Line;
    line.layer_name = QStringLiteral("curve_fillet");
    line.line_width_mm = 0.5;
    line.geometry = VpLineEntity{{0.0, 0.0}, {30.0, 0.0}};
    VpEntityRecord circle;
    circle.id = 201;
    circle.type = VpEntityType::Circle;
    circle.geometry = VpCircleEntity{{15.0, 9.0}, 5.0};
    VpEntityRecord line_result;
    VpEntityRecord circle_result;
    VpEntityRecord fillet_result;
    QVERIFY(filletedEntities(line, circle, {5.0, 0.0}, {15.0, 4.0}, 2.0, line_result, circle_result,
                             fillet_result));
    QCOMPARE(line_result.type, VpEntityType::Line);
    QCOMPARE(circle_result.type, VpEntityType::Circle);
    QCOMPARE(fillet_result.type, VpEntityType::Arc);
    QCOMPARE(std::get<VpArcEntity>(fillet_result.geometry).radius, 2.0);
    QCOMPARE(fillet_result.layer_name, line.layer_name);

    VpEntityRecord arc = circle;
    arc.id = 202;
    arc.type = VpEntityType::Arc;
    arc.geometry = VpArcEntity{{15.0, 9.0}, 5.0, 180.0, 360.0};
    VpEntityRecord arc_result;
    QVERIFY(filletedEntities(arc, line, {15.0, 4.0}, {5.0, 0.0}, 2.0, arc_result, line_result,
                             fillet_result));
    QCOMPARE(arc_result.type, VpEntityType::Arc);

    VpCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("curve fillet"));
    transaction->addLine({0.0, 0.0}, {30.0, 0.0});
    transaction->addCircle({15.0, 9.0}, 5.0);
    transaction->commit();
    VpCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setFilletRadius(2.0);
    viewport.setToolMode(VpToolMode::Fillet);
    viewport.submitWorldPoint({5.0, 0.0});
    viewport.submitWorldPoint({15.0, 4.0});
    QCOMPARE(document.entities().size(), std::size_t(3));
    QCOMPARE(document.entities().back().type, VpEntityType::Arc);
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(2));
    QCOMPARE(document.entities()[0].type, VpEntityType::Line);
    QCOMPARE(document.entities()[1].type, VpEntityType::Circle);
}

void VpCurveFilletTest::curveCurveFilletGeometry()
{
    VpEntityRecord first_circle;
    first_circle.id = 210;
    first_circle.type = VpEntityType::Circle;
    first_circle.layer_name = QStringLiteral("curve_pair");
    first_circle.geometry = VpCircleEntity{{0.0, 0.0}, 5.0};
    VpEntityRecord second_circle = first_circle;
    second_circle.id = 211;
    second_circle.geometry = VpCircleEntity{{12.0, 0.0}, 5.0};
    VpEntityRecord first_result;
    VpEntityRecord second_result;
    VpEntityRecord fillet_result;
    QVERIFY(filletedEntities(first_circle, second_circle, {0.0, 5.0}, {12.0, 5.0}, 2.0,
                             first_result, second_result, fillet_result));
    QCOMPARE(first_result.type, VpEntityType::Circle);
    QCOMPARE(second_result.type, VpEntityType::Circle);
    QCOMPARE(fillet_result.type, VpEntityType::Arc);
    QCOMPARE(std::get<VpArcEntity>(fillet_result.geometry).radius, 2.0);
    QCOMPARE(fillet_result.layer_name, first_circle.layer_name);
}

void VpCurveFilletTest::lineArcChamferGeometryAndUndo()
{
    VpEntityRecord line;
    line.id = 212;
    line.type = VpEntityType::Line;
    line.layer_name = QStringLiteral("curve_chamfer");
    line.line_width_mm = 0.5;
    line.geometry = VpLineEntity{{-10.0, 0.0}, {20.0, 0.0}};
    VpEntityRecord arc;
    arc.id = 213;
    arc.type = VpEntityType::Arc;
    arc.geometry = VpArcEntity{{0.0, 0.0}, 10.0, 0.0, 180.0};
    VpEntityRecord line_result;
    VpEntityRecord arc_result;
    VpEntityRecord chamfer_result;
    QVERIFY(chamferedEntities(line, arc, {15.0, 0.0}, {0.0, 10.0}, 2.0, 3.0, line_result,
                              arc_result, chamfer_result));
    QCOMPARE(std::get<VpLineEntity>(line_result.geometry).end_point.x, 12.0);
    QVERIFY(std::abs(std::get<VpArcEntity>(arc_result.geometry).start_angle -
                     (3.0 / 10.0 * 180.0 / 3.14159265358979323846)) < 1.0e-9);
    QCOMPARE(chamfer_result.type, VpEntityType::Line);
    QCOMPARE(chamfer_result.layer_name, line.layer_name);
    QCOMPARE(chamfer_result.line_width_mm, line.line_width_mm);

    VpCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("curve chamfer"));
    transaction->addLine({-10.0, 0.0}, {20.0, 0.0});
    transaction->addArc({0.0, 0.0}, 10.0, 0.0, 180.0);
    transaction->commit();
    VpCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setObjectSnapEnabled(false);
    viewport.setChamferDistances(2.0, 3.0);
    viewport.setToolMode(VpToolMode::Chamfer);
    viewport.submitWorldPoint({15.0, 0.0});
    viewport.submitWorldPoint({0.0, 10.0});
    QCOMPARE(document.entities().size(), std::size_t(3));
    QCOMPARE(document.entities().back().type, VpEntityType::Line);
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(2));
    QCOMPARE(std::get<VpArcEntity>(document.entities()[1].geometry).start_angle, 0.0);
}

void VpCurveFilletTest::arcArcChamferGeometry()
{
    VpEntityRecord first_arc;
    first_arc.id = 214;
    first_arc.type = VpEntityType::Arc;
    first_arc.geometry = VpArcEntity{{0.0, 0.0}, 10.0, 0.0, 180.0};
    VpEntityRecord second_arc;
    second_arc.id = 215;
    second_arc.type = VpEntityType::Arc;
    second_arc.geometry = VpArcEntity{{12.0, 0.0}, 10.0, 0.0, 180.0};
    VpEntityRecord first_result;
    VpEntityRecord second_result;
    VpEntityRecord chamfer_result;
    QVERIFY(chamferedEntities(first_arc, second_arc, {0.0, 10.0}, {12.0, 10.0}, 2.0, 2.0,
                              first_result, second_result, chamfer_result));
    QVERIFY(std::get<VpArcEntity>(first_result.geometry).start_angle > 53.0);
    QVERIFY(std::get<VpArcEntity>(second_result.geometry).end_angle < 127.0);
    QCOMPARE(chamfer_result.type, VpEntityType::Line);
}

} // namespace Vp

QTEST_MAIN(Vp::VpCurveFilletTest)
#include "vp_curve_fillet_test.moc"
