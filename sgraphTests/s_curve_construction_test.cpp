#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_cad_viewport_geometry.h"
#include "s_document_transaction.h"

#include <QtTest>
#include <cmath>

namespace smartGraphics
{

class SCurveConstructionTest final : public QObject
{
    Q_OBJECT

  private slots:
    void circleConstructionModesAndUndo();
    void arcCenterConstructionModesAndUndo();
    void arcParameterizedGeometry();
    void arcParameterizedModesAndUndo();
    void tangentCircleGeometry();
    void tangentCircleMixedGeometry();
    void tangentCircleInteractionAndUndo();
    void tangentCircleMixedInteractionAndUndo();
};

void SCurveConstructionTest::circleConstructionModesAndUndo()
{
    SCadDocument document;
    SCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setObjectSnapEnabled(false);

    viewport.setCircleConstruction(SCircleConstruction::TwoPoint);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({10.0, 0.0});
    QCOMPARE(document.entities().size(), std::size_t(1));
    const auto& two_point = std::get<SCircleEntity>(document.entities().back().geometry);
    QCOMPARE(two_point.center.x, 5.0);
    QCOMPARE(two_point.radius, 5.0);

    viewport.setCircleConstruction(SCircleConstruction::ThreePoint);
    viewport.submitWorldPoint({5.0, 0.0});
    viewport.submitWorldPoint({0.0, 5.0});
    viewport.submitWorldPoint({-5.0, 0.0});
    const auto& three_point = std::get<SCircleEntity>(document.entities().back().geometry);
    QVERIFY(std::abs(three_point.center.x) < 1.0e-9);
    QVERIFY(std::abs(three_point.center.y) < 1.0e-9);
    QVERIFY(std::abs(three_point.radius - 5.0) < 1.0e-9);

    viewport.setCircleConstruction(SCircleConstruction::CenterDiameter);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({10.0, 0.0});
    QCOMPARE(std::get<SCircleEntity>(document.entities().back().geometry).radius, 5.0);
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(2));
}

void SCurveConstructionTest::arcCenterConstructionModesAndUndo()
{
    SCadDocument document;
    SCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setObjectSnapEnabled(false);

    viewport.setArcConstruction(SArcConstruction::CenterStartEnd);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({5.0, 0.0});
    viewport.submitWorldPoint({0.0, 5.0});
    const auto& center_start_end = std::get<SArcEntity>(document.entities().back().geometry);
    QCOMPARE(center_start_end.radius, 5.0);
    QVERIFY(std::abs(center_start_end.start_angle) < 1.0e-9);
    QVERIFY(std::abs(center_start_end.end_angle - 90.0) < 1.0e-9);

    viewport.setArcConstruction(SArcConstruction::StartCenterEnd);
    viewport.submitWorldPoint({0.0, 5.0});
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({-5.0, 0.0});
    const auto& start_center_end = std::get<SArcEntity>(document.entities().back().geometry);
    QVERIFY(std::abs(start_center_end.start_angle - 90.0) < 1.0e-9);
    QVERIFY(std::abs(start_center_end.end_angle - 180.0) < 1.0e-9);
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(1));

    viewport.setArcConstruction(SArcConstruction::ThreePoint);
    viewport.submitWorldPoint({1.0, 0.0});
    QVERIFY(viewport.submitCommandKeyword(QStringLiteral("UNDO")));
    viewport.submitWorldPoint({1.0, 0.0});
    viewport.submitWorldPoint({0.0, 1.0});
    viewport.submitWorldPoint({-1.0, 0.0});
    QCOMPARE(document.entities().size(), std::size_t(2));
}

void SCurveConstructionTest::arcParameterizedGeometry()
{
    SArcEntity result;
    QVERIFY(arcFromStartEndRadius({0.0, 0.0}, {10.0, 0.0}, 10.0, 1.0, result));
    QVERIFY(std::abs(result.center.x - 5.0) < 1.0e-9);
    QVERIFY(std::abs(result.center.y - (5.0 * std::sqrt(3.0))) < 1.0e-9);
    QVERIFY(std::abs(result.radius - 10.0) < 1.0e-9);

    QVERIFY(arcFromStartEndDirection({0.0, 0.0}, {10.0, 0.0}, 45.0, result));
    QVERIFY(std::abs(result.center.x - 5.0) < 1.0e-9);
    QVERIFY(std::abs(result.center.y + 5.0) < 1.0e-9);
    QVERIFY(std::abs(result.radius - (5.0 * std::sqrt(2.0))) < 1.0e-9);

    QVERIFY(arcFromStartEndAngle({0.0, 0.0}, {10.0, 0.0}, 90.0, result));
    QVERIFY(std::abs(result.center.x - 5.0) < 1.0e-9);
    QVERIFY(std::abs(result.center.y - 5.0) < 1.0e-9);
    QVERIFY(std::abs(result.radius - (5.0 * std::sqrt(2.0))) < 1.0e-9);

    QVERIFY(arcFromCenterStartAngle({0.0, 0.0}, {5.0, 0.0}, 90.0, result));
    QVERIFY(std::abs(result.radius - 5.0) < 1.0e-9);
    QVERIFY(std::abs(result.start_angle) < 1.0e-9);
    QVERIFY(std::abs(result.end_angle - 90.0) < 1.0e-9);
    QVERIFY(!arcFromStartEndRadius({0.0, 0.0}, {10.0, 0.0}, 4.0, 1.0, result));
}

void SCurveConstructionTest::arcParameterizedModesAndUndo()
{
    SCadDocument document;
    SCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setObjectSnapEnabled(false);

    viewport.setArcConstructionParameter(SArcConstruction::StartEndRadius, 5.0);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({10.0, 0.0});
    viewport.setArcConstructionParameter(SArcConstruction::StartEndDirection, 45.0);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({10.0, 0.0});
    viewport.setArcConstructionParameter(SArcConstruction::StartEndAngle, 90.0);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({10.0, 0.0});
    viewport.setArcConstructionParameter(SArcConstruction::StartCenterAngle, 90.0);
    viewport.submitWorldPoint({5.0, 0.0});
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.setArcConstructionParameter(SArcConstruction::CenterStartAngle, 90.0);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({5.0, 0.0});
    QCOMPARE(document.entities().size(), std::size_t(5));

    viewport.setArcConstruction(SArcConstruction::StartEndRadius);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({10.0, 0.0});
    QVERIFY(viewport.submitCommandKeyword(QStringLiteral("5")));
    QCOMPARE(document.entities().size(), std::size_t(6));

    viewport.setArcConstruction(SArcConstruction::StartEndDirection);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({10.0, 0.0});
    viewport.submitWorldPoint({0.0, 10.0});
    QCOMPARE(document.entities().size(), std::size_t(7));
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(6));
}

void SCurveConstructionTest::tangentCircleGeometry()
{
    SEntityRecord horizontal;
    horizontal.type = SEntityType::Line;
    horizontal.geometry = SLineEntity{{-30.0, 0.0}, {30.0, 0.0}};
    SEntityRecord vertical;
    vertical.type = SEntityType::Line;
    vertical.geometry = SLineEntity{{0.0, -30.0}, {0.0, 30.0}};
    SCircleEntity result;
    QVERIFY(
        tangentCircleToTwoEntities(horizontal, vertical, {20.0, 0.0}, {0.0, 20.0}, 2.0, result));
    QVERIFY(std::abs(result.center.x - 2.0) < 1.0e-9);
    QVERIFY(std::abs(result.center.y - 2.0) < 1.0e-9);

    SEntityRecord source_circle;
    source_circle.type = SEntityType::Circle;
    source_circle.geometry = SCircleEntity{{5.0, 5.0}, 1.0};
    QVERIFY(
        tangentCircleToTwoEntities(horizontal, source_circle, {5.0, 0.0}, {5.0, 4.0}, 2.0, result));
    QVERIFY(std::abs(result.center.x - 5.0) < 1.0e-9);
    QVERIFY(std::abs(result.center.y - 2.0) < 1.0e-9);

    SEntityRecord diagonal;
    diagonal.type = SEntityType::Line;
    diagonal.geometry = SLineEntity{{0.0, 10.0}, {10.0, 0.0}};
    QVERIFY(tangentCircleToThreeLines({horizontal, vertical, diagonal},
                                      {SPoint2d{3.0, 0.0}, SPoint2d{0.0, 3.0}, SPoint2d{5.0, 5.0}},
                                      result));
    const double expected_radius = 10.0 - (5.0 * std::sqrt(2.0));
    QVERIFY(std::abs(result.radius - expected_radius) < 1.0e-6);
    QVERIFY(std::abs(result.center.x - expected_radius) < 1.0e-6);
    QVERIFY(std::abs(result.center.y - expected_radius) < 1.0e-6);
}

void SCurveConstructionTest::tangentCircleMixedGeometry()
{
    SEntityRecord vertical;
    vertical.type = SEntityType::Line;
    vertical.geometry = SLineEntity{{0.0, -20.0}, {0.0, 20.0}};
    SEntityRecord horizontal;
    horizontal.type = SEntityType::Line;
    horizontal.geometry = SLineEntity{{-20.0, 0.0}, {20.0, 0.0}};
    SEntityRecord source_circle;
    source_circle.type = SEntityType::Circle;
    source_circle.geometry = SCircleEntity{{10.0, 3.0}, 4.0};

    SCircleEntity result;
    QVERIFY(tangentCircleToThreeEntities(
        {vertical, horizontal, source_circle},
        {SPoint2d{0.0, 3.0}, SPoint2d{3.0, 0.0}, SPoint2d{6.0, 3.0}}, result));
    QVERIFY(std::abs(result.center.x - 3.0) < 1.0e-6);
    QVERIFY(std::abs(result.center.y - 3.0) < 1.0e-6);
    QVERIFY(std::abs(result.radius - 3.0) < 1.0e-6);

    SEntityRecord left_circle;
    left_circle.type = SEntityType::Circle;
    left_circle.geometry = SCircleEntity{{-5.0, 3.0}, 2.0};
    SEntityRecord right_circle;
    right_circle.type = SEntityType::Circle;
    right_circle.geometry = SCircleEntity{{6.0, 3.0}, 3.0};
    QVERIFY(tangentCircleToThreeEntities(
        {horizontal, left_circle, right_circle},
        {SPoint2d{0.0, 0.0}, SPoint2d{-3.0, 3.0}, SPoint2d{3.0, 3.0}}, result));
    QVERIFY(std::abs(result.center.x) < 1.0e-6);
    QVERIFY(std::abs(result.center.y - 3.0) < 1.0e-6);
    QVERIFY(std::abs(result.radius - 3.0) < 1.0e-6);

    SEntityRecord three_left_circle;
    three_left_circle.type = SEntityType::Circle;
    three_left_circle.geometry = SCircleEntity{{-5.0, 0.0}, 3.0};
    SEntityRecord three_right_circle;
    three_right_circle.type = SEntityType::Circle;
    three_right_circle.geometry = SCircleEntity{{6.0, 0.0}, 4.0};
    SEntityRecord three_top_circle;
    three_top_circle.type = SEntityType::Circle;
    three_top_circle.geometry = SCircleEntity{{0.0, 7.0}, 5.0};
    QVERIFY(tangentCircleToThreeEntities(
        {three_left_circle, three_right_circle, three_top_circle},
        {SPoint2d{-2.0, 0.0}, SPoint2d{2.0, 0.0}, SPoint2d{0.0, 2.0}}, result));
    QVERIFY(std::abs(result.center.x) < 1.0e-6);
    QVERIFY(std::abs(result.center.y) < 1.0e-6);
    QVERIFY(std::abs(result.radius - 2.0) < 1.0e-6);

    SEntityRecord tangent_arc = left_circle;
    tangent_arc.type = SEntityType::Arc;
    tangent_arc.geometry = SArcEntity{{-5.0, 3.0}, 2.0, 330.0, 30.0};
    QVERIFY(tangentCircleToThreeEntities(
        {horizontal, tangent_arc, right_circle},
        {SPoint2d{0.0, 0.0}, SPoint2d{-3.0, 3.0}, SPoint2d{3.0, 3.0}}, result));
    QVERIFY(std::abs(result.center.y - 3.0) < 1.0e-6);
}

void SCurveConstructionTest::tangentCircleInteractionAndUndo()
{
    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("tangent supports"));
    transaction->addLine({-30.0, 0.0}, {30.0, 0.0});
    transaction->addLine({0.0, -30.0}, {0.0, 30.0});
    transaction->commit();
    SCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setObjectSnapEnabled(false);
    viewport.setCircleTangentRadius(2.0);
    viewport.submitWorldPoint({20.0, 0.0});
    viewport.submitWorldPoint({0.0, 20.0});
    QCOMPARE(document.entities().size(), std::size_t(3));
    QCOMPARE(document.entities().back().type, SEntityType::Circle);
    QCOMPARE(std::get<SCircleEntity>(document.entities().back().geometry).radius, 2.0);
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(2));

    auto diagonal_transaction = document.beginTransaction(QStringLiteral("third tangent"));
    diagonal_transaction->addLine({0.0, 10.0}, {10.0, 0.0});
    diagonal_transaction->commit();
    viewport.setCircleConstruction(SCircleConstruction::TangentTangentTangent);
    viewport.submitWorldPoint({20.0, 0.0});
    viewport.submitWorldPoint({0.0, 20.0});
    viewport.submitWorldPoint({5.0, 5.0});
    QCOMPARE(document.entities().size(), std::size_t(4));
    QCOMPARE(document.entities().back().type, SEntityType::Circle);
}

void SCurveConstructionTest::tangentCircleMixedInteractionAndUndo()
{
    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("mixed tangent supports"));
    transaction->addLine({0.0, -20.0}, {0.0, 20.0});
    transaction->addLine({-20.0, 0.0}, {20.0, 0.0});
    transaction->addCircle({10.0, 3.0}, 4.0);
    transaction->commit();

    SCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setObjectSnapEnabled(false);
    viewport.setCircleConstruction(SCircleConstruction::TangentTangentTangent);
    viewport.submitWorldPoint({0.0, 3.0});
    viewport.submitWorldPoint({3.0, 0.0});
    viewport.submitWorldPoint({6.0, 3.0});

    QCOMPARE(document.entities().size(), std::size_t(4));
    const auto& result = std::get<SCircleEntity>(document.entities().back().geometry);
    QVERIFY(std::abs(result.center.x - 3.0) < 1.0e-6);
    QVERIFY(std::abs(result.center.y - 3.0) < 1.0e-6);
    QVERIFY(std::abs(result.radius - 3.0) < 1.0e-6);
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(3));
}

} // namespace smartGraphics

QTEST_MAIN(smartGraphics::SCurveConstructionTest)
#include "s_curve_construction_test.moc"
