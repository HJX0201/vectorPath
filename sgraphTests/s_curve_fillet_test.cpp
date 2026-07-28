#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_cad_viewport_geometry.h"
#include "s_document_transaction.h"

#include <QtTest>

namespace smartGraphics
{

class SCurveFilletTest final : public QObject
{
    Q_OBJECT

  private slots:
    void lineCircleFilletGeometryAndUndo();
    void curveCurveFilletGeometry();
    void lineArcChamferGeometryAndUndo();
    void arcArcChamferGeometry();
};

void SCurveFilletTest::lineCircleFilletGeometryAndUndo()
{
    SEntityRecord line;
    line.id = 200;
    line.type = SEntityType::Line;
    line.layer_name = QStringLiteral("curve_fillet");
    line.line_width_mm = 0.5;
    line.geometry = SLineEntity{{0.0, 0.0}, {30.0, 0.0}};
    SEntityRecord circle;
    circle.id = 201;
    circle.type = SEntityType::Circle;
    circle.geometry = SCircleEntity{{15.0, 9.0}, 5.0};
    SEntityRecord line_result;
    SEntityRecord circle_result;
    SEntityRecord fillet_result;
    QVERIFY(filletedEntities(line, circle, {5.0, 0.0}, {15.0, 4.0}, 2.0, line_result, circle_result,
                             fillet_result));
    QCOMPARE(line_result.type, SEntityType::Line);
    QCOMPARE(circle_result.type, SEntityType::Circle);
    QCOMPARE(fillet_result.type, SEntityType::Arc);
    QCOMPARE(std::get<SArcEntity>(fillet_result.geometry).radius, 2.0);
    QCOMPARE(fillet_result.layer_name, line.layer_name);

    SEntityRecord arc = circle;
    arc.id = 202;
    arc.type = SEntityType::Arc;
    arc.geometry = SArcEntity{{15.0, 9.0}, 5.0, 180.0, 360.0};
    SEntityRecord arc_result;
    QVERIFY(filletedEntities(arc, line, {15.0, 4.0}, {5.0, 0.0}, 2.0, arc_result, line_result,
                             fillet_result));
    QCOMPARE(arc_result.type, SEntityType::Arc);

    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("curve fillet"));
    transaction->addLine({0.0, 0.0}, {30.0, 0.0});
    transaction->addCircle({15.0, 9.0}, 5.0);
    transaction->commit();
    SCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setFilletRadius(2.0);
    viewport.setToolMode(SToolMode::Fillet);
    viewport.submitWorldPoint({5.0, 0.0});
    viewport.submitWorldPoint({15.0, 4.0});
    QCOMPARE(document.entities().size(), std::size_t(3));
    QCOMPARE(document.entities().back().type, SEntityType::Arc);
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(2));
    QCOMPARE(document.entities()[0].type, SEntityType::Line);
    QCOMPARE(document.entities()[1].type, SEntityType::Circle);
}

void SCurveFilletTest::curveCurveFilletGeometry()
{
    SEntityRecord first_circle;
    first_circle.id = 210;
    first_circle.type = SEntityType::Circle;
    first_circle.layer_name = QStringLiteral("curve_pair");
    first_circle.geometry = SCircleEntity{{0.0, 0.0}, 5.0};
    SEntityRecord second_circle = first_circle;
    second_circle.id = 211;
    second_circle.geometry = SCircleEntity{{12.0, 0.0}, 5.0};
    SEntityRecord first_result;
    SEntityRecord second_result;
    SEntityRecord fillet_result;
    QVERIFY(filletedEntities(first_circle, second_circle, {0.0, 5.0}, {12.0, 5.0}, 2.0,
                             first_result, second_result, fillet_result));
    QCOMPARE(first_result.type, SEntityType::Circle);
    QCOMPARE(second_result.type, SEntityType::Circle);
    QCOMPARE(fillet_result.type, SEntityType::Arc);
    QCOMPARE(std::get<SArcEntity>(fillet_result.geometry).radius, 2.0);
    QCOMPARE(fillet_result.layer_name, first_circle.layer_name);
}

void SCurveFilletTest::lineArcChamferGeometryAndUndo()
{
    SEntityRecord line;
    line.id = 212;
    line.type = SEntityType::Line;
    line.layer_name = QStringLiteral("curve_chamfer");
    line.line_width_mm = 0.5;
    line.geometry = SLineEntity{{-10.0, 0.0}, {20.0, 0.0}};
    SEntityRecord arc;
    arc.id = 213;
    arc.type = SEntityType::Arc;
    arc.geometry = SArcEntity{{0.0, 0.0}, 10.0, 0.0, 180.0};
    SEntityRecord line_result;
    SEntityRecord arc_result;
    SEntityRecord chamfer_result;
    QVERIFY(chamferedEntities(line, arc, {15.0, 0.0}, {0.0, 10.0}, 2.0, 3.0, line_result,
                              arc_result, chamfer_result));
    QCOMPARE(std::get<SLineEntity>(line_result.geometry).end_point.x, 12.0);
    QVERIFY(std::abs(std::get<SArcEntity>(arc_result.geometry).start_angle -
                     (3.0 / 10.0 * 180.0 / 3.14159265358979323846)) < 1.0e-9);
    QCOMPARE(chamfer_result.type, SEntityType::Line);
    QCOMPARE(chamfer_result.layer_name, line.layer_name);
    QCOMPARE(chamfer_result.line_width_mm, line.line_width_mm);

    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("curve chamfer"));
    transaction->addLine({-10.0, 0.0}, {20.0, 0.0});
    transaction->addArc({0.0, 0.0}, 10.0, 0.0, 180.0);
    transaction->commit();
    SCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setObjectSnapEnabled(false);
    viewport.setChamferDistances(2.0, 3.0);
    viewport.setToolMode(SToolMode::Chamfer);
    viewport.submitWorldPoint({15.0, 0.0});
    viewport.submitWorldPoint({0.0, 10.0});
    QCOMPARE(document.entities().size(), std::size_t(3));
    QCOMPARE(document.entities().back().type, SEntityType::Line);
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(2));
    QCOMPARE(std::get<SArcEntity>(document.entities()[1].geometry).start_angle, 0.0);
}

void SCurveFilletTest::arcArcChamferGeometry()
{
    SEntityRecord first_arc;
    first_arc.id = 214;
    first_arc.type = SEntityType::Arc;
    first_arc.geometry = SArcEntity{{0.0, 0.0}, 10.0, 0.0, 180.0};
    SEntityRecord second_arc;
    second_arc.id = 215;
    second_arc.type = SEntityType::Arc;
    second_arc.geometry = SArcEntity{{12.0, 0.0}, 10.0, 0.0, 180.0};
    SEntityRecord first_result;
    SEntityRecord second_result;
    SEntityRecord chamfer_result;
    QVERIFY(chamferedEntities(first_arc, second_arc, {0.0, 10.0}, {12.0, 10.0}, 2.0, 2.0,
                              first_result, second_result, chamfer_result));
    QVERIFY(std::get<SArcEntity>(first_result.geometry).start_angle > 53.0);
    QVERIFY(std::get<SArcEntity>(second_result.geometry).end_angle < 127.0);
    QCOMPARE(chamfer_result.type, SEntityType::Line);
}

} // namespace smartGraphics

QTEST_MAIN(smartGraphics::SCurveFilletTest)
#include "s_curve_fillet_test.moc"
