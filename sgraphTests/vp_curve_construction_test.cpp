#include "vp_cad_document.h"
#include "vp_cad_viewport.h"
#include "vp_cad_viewport_geometry.h"
#include "vp_document_transaction.h"

#include <QtTest>
#include <cmath>

namespace Vp
{

class VpCurveConstructionTest final : public QObject
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

void VpCurveConstructionTest::circleConstructionModesAndUndo()
{
    VpCadDocument document;
    VpCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setObjectSnapEnabled(false);

    viewport.setCircleConstruction(VpCircleConstruction::TwoPoint);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({10.0, 0.0});
    QCOMPARE(document.entities().size(), std::size_t(1));
    const auto& two_point = std::get<VpCircleEntity>(document.entities().back().geometry);
    QCOMPARE(two_point.center.x, 5.0);
    QCOMPARE(two_point.radius, 5.0);

    viewport.setCircleConstruction(VpCircleConstruction::ThreePoint);
    viewport.submitWorldPoint({5.0, 0.0});
    viewport.submitWorldPoint({0.0, 5.0});
    viewport.submitWorldPoint({-5.0, 0.0});
    const auto& three_point = std::get<VpCircleEntity>(document.entities().back().geometry);
    QVERIFY(std::abs(three_point.center.x) < 1.0e-9);
    QVERIFY(std::abs(three_point.center.y) < 1.0e-9);
    QVERIFY(std::abs(three_point.radius - 5.0) < 1.0e-9);

    viewport.setCircleConstruction(VpCircleConstruction::CenterDiameter);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({10.0, 0.0});
    QCOMPARE(std::get<VpCircleEntity>(document.entities().back().geometry).radius, 5.0);
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(2));
}

void VpCurveConstructionTest::arcCenterConstructionModesAndUndo()
{
    VpCadDocument document;
    VpCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setObjectSnapEnabled(false);

    viewport.setArcConstruction(VpArcConstruction::CenterStartEnd);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({5.0, 0.0});
    viewport.submitWorldPoint({0.0, 5.0});
    const auto& center_start_end = std::get<VpArcEntity>(document.entities().back().geometry);
    QCOMPARE(center_start_end.radius, 5.0);
    QVERIFY(std::abs(center_start_end.start_angle) < 1.0e-9);
    QVERIFY(std::abs(center_start_end.end_angle - 90.0) < 1.0e-9);

    viewport.setArcConstruction(VpArcConstruction::StartCenterEnd);
    viewport.submitWorldPoint({0.0, 5.0});
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({-5.0, 0.0});
    const auto& start_center_end = std::get<VpArcEntity>(document.entities().back().geometry);
    QVERIFY(std::abs(start_center_end.start_angle - 90.0) < 1.0e-9);
    QVERIFY(std::abs(start_center_end.end_angle - 180.0) < 1.0e-9);
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(1));

    viewport.setArcConstruction(VpArcConstruction::ThreePoint);
    viewport.submitWorldPoint({1.0, 0.0});
    QVERIFY(viewport.submitCommandKeyword(QStringLiteral("UNDO")));
    viewport.submitWorldPoint({1.0, 0.0});
    viewport.submitWorldPoint({0.0, 1.0});
    viewport.submitWorldPoint({-1.0, 0.0});
    QCOMPARE(document.entities().size(), std::size_t(2));
}

void VpCurveConstructionTest::arcParameterizedGeometry()
{
    VpArcEntity result;
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

void VpCurveConstructionTest::arcParameterizedModesAndUndo()
{
    VpCadDocument document;
    VpCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setObjectSnapEnabled(false);

    viewport.setArcConstructionParameter(VpArcConstruction::StartEndRadius, 5.0);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({10.0, 0.0});
    viewport.setArcConstructionParameter(VpArcConstruction::StartEndDirection, 45.0);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({10.0, 0.0});
    viewport.setArcConstructionParameter(VpArcConstruction::StartEndAngle, 90.0);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({10.0, 0.0});
    viewport.setArcConstructionParameter(VpArcConstruction::StartCenterAngle, 90.0);
    viewport.submitWorldPoint({5.0, 0.0});
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.setArcConstructionParameter(VpArcConstruction::CenterStartAngle, 90.0);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({5.0, 0.0});
    QCOMPARE(document.entities().size(), std::size_t(5));

    viewport.setArcConstruction(VpArcConstruction::StartEndRadius);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({10.0, 0.0});
    QVERIFY(viewport.submitCommandKeyword(QStringLiteral("5")));
    QCOMPARE(document.entities().size(), std::size_t(6));

    viewport.setArcConstruction(VpArcConstruction::StartEndDirection);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({10.0, 0.0});
    viewport.submitWorldPoint({0.0, 10.0});
    QCOMPARE(document.entities().size(), std::size_t(7));
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(6));
}

void VpCurveConstructionTest::tangentCircleGeometry()
{
    VpEntityRecord horizontal;
    horizontal.type = VpEntityType::Line;
    horizontal.geometry = VpLineEntity{{-30.0, 0.0}, {30.0, 0.0}};
    VpEntityRecord vertical;
    vertical.type = VpEntityType::Line;
    vertical.geometry = VpLineEntity{{0.0, -30.0}, {0.0, 30.0}};
    VpCircleEntity result;
    QVERIFY(
        tangentCircleToTwoEntities(horizontal, vertical, {20.0, 0.0}, {0.0, 20.0}, 2.0, result));
    QVERIFY(std::abs(result.center.x - 2.0) < 1.0e-9);
    QVERIFY(std::abs(result.center.y - 2.0) < 1.0e-9);

    VpEntityRecord source_circle;
    source_circle.type = VpEntityType::Circle;
    source_circle.geometry = VpCircleEntity{{5.0, 5.0}, 1.0};
    QVERIFY(
        tangentCircleToTwoEntities(horizontal, source_circle, {5.0, 0.0}, {5.0, 4.0}, 2.0, result));
    QVERIFY(std::abs(result.center.x - 5.0) < 1.0e-9);
    QVERIFY(std::abs(result.center.y - 2.0) < 1.0e-9);

    VpEntityRecord diagonal;
    diagonal.type = VpEntityType::Line;
    diagonal.geometry = VpLineEntity{{0.0, 10.0}, {10.0, 0.0}};
    QVERIFY(tangentCircleToThreeLines(
        {horizontal, vertical, diagonal},
        {VpPoint2d{3.0, 0.0}, VpPoint2d{0.0, 3.0}, VpPoint2d{5.0, 5.0}}, result));
    const double expected_radius = 10.0 - (5.0 * std::sqrt(2.0));
    QVERIFY(std::abs(result.radius - expected_radius) < 1.0e-6);
    QVERIFY(std::abs(result.center.x - expected_radius) < 1.0e-6);
    QVERIFY(std::abs(result.center.y - expected_radius) < 1.0e-6);
}

void VpCurveConstructionTest::tangentCircleMixedGeometry()
{
    VpEntityRecord vertical;
    vertical.type = VpEntityType::Line;
    vertical.geometry = VpLineEntity{{0.0, -20.0}, {0.0, 20.0}};
    VpEntityRecord horizontal;
    horizontal.type = VpEntityType::Line;
    horizontal.geometry = VpLineEntity{{-20.0, 0.0}, {20.0, 0.0}};
    VpEntityRecord source_circle;
    source_circle.type = VpEntityType::Circle;
    source_circle.geometry = VpCircleEntity{{10.0, 3.0}, 4.0};

    VpCircleEntity result;
    QVERIFY(tangentCircleToThreeEntities(
        {vertical, horizontal, source_circle},
        {VpPoint2d{0.0, 3.0}, VpPoint2d{3.0, 0.0}, VpPoint2d{6.0, 3.0}}, result));
    QVERIFY(std::abs(result.center.x - 3.0) < 1.0e-6);
    QVERIFY(std::abs(result.center.y - 3.0) < 1.0e-6);
    QVERIFY(std::abs(result.radius - 3.0) < 1.0e-6);

    VpEntityRecord left_circle;
    left_circle.type = VpEntityType::Circle;
    left_circle.geometry = VpCircleEntity{{-5.0, 3.0}, 2.0};
    VpEntityRecord right_circle;
    right_circle.type = VpEntityType::Circle;
    right_circle.geometry = VpCircleEntity{{6.0, 3.0}, 3.0};
    QVERIFY(tangentCircleToThreeEntities(
        {horizontal, left_circle, right_circle},
        {VpPoint2d{0.0, 0.0}, VpPoint2d{-3.0, 3.0}, VpPoint2d{3.0, 3.0}}, result));
    QVERIFY(std::abs(result.center.x) < 1.0e-6);
    QVERIFY(std::abs(result.center.y - 3.0) < 1.0e-6);
    QVERIFY(std::abs(result.radius - 3.0) < 1.0e-6);

    VpEntityRecord three_left_circle;
    three_left_circle.type = VpEntityType::Circle;
    three_left_circle.geometry = VpCircleEntity{{-5.0, 0.0}, 3.0};
    VpEntityRecord three_right_circle;
    three_right_circle.type = VpEntityType::Circle;
    three_right_circle.geometry = VpCircleEntity{{6.0, 0.0}, 4.0};
    VpEntityRecord three_top_circle;
    three_top_circle.type = VpEntityType::Circle;
    three_top_circle.geometry = VpCircleEntity{{0.0, 7.0}, 5.0};
    QVERIFY(tangentCircleToThreeEntities(
        {three_left_circle, three_right_circle, three_top_circle},
        {VpPoint2d{-2.0, 0.0}, VpPoint2d{2.0, 0.0}, VpPoint2d{0.0, 2.0}}, result));
    QVERIFY(std::abs(result.center.x) < 1.0e-6);
    QVERIFY(std::abs(result.center.y) < 1.0e-6);
    QVERIFY(std::abs(result.radius - 2.0) < 1.0e-6);

    VpEntityRecord tangent_arc = left_circle;
    tangent_arc.type = VpEntityType::Arc;
    tangent_arc.geometry = VpArcEntity{{-5.0, 3.0}, 2.0, 330.0, 30.0};
    QVERIFY(tangentCircleToThreeEntities(
        {horizontal, tangent_arc, right_circle},
        {VpPoint2d{0.0, 0.0}, VpPoint2d{-3.0, 3.0}, VpPoint2d{3.0, 3.0}}, result));
    QVERIFY(std::abs(result.center.y - 3.0) < 1.0e-6);
}

void VpCurveConstructionTest::tangentCircleInteractionAndUndo()
{
    VpCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("tangent supports"));
    transaction->addLine({-30.0, 0.0}, {30.0, 0.0});
    transaction->addLine({0.0, -30.0}, {0.0, 30.0});
    transaction->commit();
    VpCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setObjectSnapEnabled(false);
    viewport.setCircleTangentRadius(2.0);
    viewport.submitWorldPoint({20.0, 0.0});
    viewport.submitWorldPoint({0.0, 20.0});
    QCOMPARE(document.entities().size(), std::size_t(3));
    QCOMPARE(document.entities().back().type, VpEntityType::Circle);
    QCOMPARE(std::get<VpCircleEntity>(document.entities().back().geometry).radius, 2.0);
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(2));

    auto diagonal_transaction = document.beginTransaction(QStringLiteral("third tangent"));
    diagonal_transaction->addLine({0.0, 10.0}, {10.0, 0.0});
    diagonal_transaction->commit();
    viewport.setCircleConstruction(VpCircleConstruction::TangentTangentTangent);
    viewport.submitWorldPoint({20.0, 0.0});
    viewport.submitWorldPoint({0.0, 20.0});
    viewport.submitWorldPoint({5.0, 5.0});
    QCOMPARE(document.entities().size(), std::size_t(4));
    QCOMPARE(document.entities().back().type, VpEntityType::Circle);
}

void VpCurveConstructionTest::tangentCircleMixedInteractionAndUndo()
{
    VpCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("mixed tangent supports"));
    transaction->addLine({0.0, -20.0}, {0.0, 20.0});
    transaction->addLine({-20.0, 0.0}, {20.0, 0.0});
    transaction->addCircle({10.0, 3.0}, 4.0);
    transaction->commit();

    VpCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setObjectSnapEnabled(false);
    viewport.setCircleConstruction(VpCircleConstruction::TangentTangentTangent);
    viewport.submitWorldPoint({0.0, 3.0});
    viewport.submitWorldPoint({3.0, 0.0});
    viewport.submitWorldPoint({6.0, 3.0});

    QCOMPARE(document.entities().size(), std::size_t(4));
    const auto& result = std::get<VpCircleEntity>(document.entities().back().geometry);
    QVERIFY(std::abs(result.center.x - 3.0) < 1.0e-6);
    QVERIFY(std::abs(result.center.y - 3.0) < 1.0e-6);
    QVERIFY(std::abs(result.radius - 3.0) < 1.0e-6);
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(3));
}

} // namespace Vp

QTEST_MAIN(Vp::VpCurveConstructionTest)
#include "vp_curve_construction_test.moc"
