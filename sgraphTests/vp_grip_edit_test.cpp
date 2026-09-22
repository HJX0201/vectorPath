#include "vp_cad_document.h"
#include "vp_cad_viewport.h"
#include "vp_cad_viewport_geometry.h"
#include "vp_document_transaction.h"

#include <QtTest>
#include <cmath>

namespace Vp
{

class VpGripEditTest final : public QObject
{
    Q_OBJECT

  private slots:
    void entityGripGeometry();
    void gripTransformGeometry();
    void gripOperationCycle();
    void viewportGripDragAndUndo();
    void viewportMultiEntityGripMoveAndUndo();
    void lengthenGeometryAndUndo();
};

void VpGripEditTest::entityGripGeometry()
{
    VpEntityRecord line;
    line.id = 300;
    line.type = VpEntityType::Line;
    line.geometry = VpLineEntity{{0.0, 0.0}, {10.0, 0.0}};
    const std::vector<VpGripHandle> line_grips = entityGripHandles(line);
    QCOMPARE(line_grips.size(), std::size_t(3));
    VpEntityRecord edited;
    QVERIFY(gripEditedEntity(line, line_grips[1], {8.0, 4.0}, edited));
    const auto& moved_line = std::get<VpLineEntity>(edited.geometry);
    QCOMPARE(moved_line.start_point.x, 3.0);
    QCOMPARE(moved_line.start_point.y, 4.0);
    QCOMPARE(moved_line.end_point.x, 13.0);

    VpEntityRecord circle;
    circle.id = 301;
    circle.type = VpEntityType::Circle;
    circle.geometry = VpCircleEntity{{2.0, 3.0}, 5.0};
    const std::vector<VpGripHandle> circle_grips = entityGripHandles(circle);
    QCOMPARE(circle_grips.size(), std::size_t(5));
    QVERIFY(gripEditedEntity(circle, circle_grips[1], {12.0, 3.0}, edited));
    QCOMPARE(std::get<VpCircleEntity>(edited.geometry).radius, 10.0);

    VpEntityRecord polyline;
    polyline.id = 302;
    polyline.type = VpEntityType::Polyline;
    polyline.geometry =
        VpPolylineEntity{{{0.0, 0.0}, {5.0, 5.0}, {10.0, 0.0}}, false, {0.5, -0.5, 0.0}};
    const std::vector<VpGripHandle> polyline_grips = entityGripHandles(polyline);
    QVERIFY(gripEditedEntity(polyline, polyline_grips[1], {6.0, 7.0}, edited));
    const auto& edited_polyline = std::get<VpPolylineEntity>(edited.geometry);
    QCOMPARE(edited_polyline.vertices[1].x, 6.0);
    QCOMPARE(edited_polyline.vertices[1].y, 7.0);
    QCOMPARE(edited_polyline.bulges, std::get<VpPolylineEntity>(polyline.geometry).bulges);
}

void VpGripEditTest::gripTransformGeometry()
{
    VpEntityRecord first_line;
    first_line.id = 303;
    first_line.type = VpEntityType::Line;
    first_line.geometry = VpLineEntity{{1.0, 0.0}, {3.0, 0.0}};
    VpEntityRecord second_line;
    second_line.id = 304;
    second_line.type = VpEntityType::Line;
    second_line.geometry = VpLineEntity{{0.0, 2.0}, {0.0, 4.0}};
    const std::vector<VpEntityRecord> sources{first_line, second_line};

    const auto moved =
        gripTransformedEntities(sources, VpGripOperation::Move, {0.0, 0.0}, {1.0, 0.0}, {5.0, 3.0});
    QCOMPARE(std::get<VpLineEntity>(moved[0].geometry).start_point.x, 6.0);
    QCOMPARE(std::get<VpLineEntity>(moved[1].geometry).end_point.y, 7.0);

    const auto rotated = gripTransformedEntities(sources, VpGripOperation::Rotate, {0.0, 0.0},
                                                 {1.0, 0.0}, {0.0, 1.0});
    QVERIFY(std::abs(std::get<VpLineEntity>(rotated[0].geometry).start_point.x) < 1.0e-9);
    QCOMPARE(std::get<VpLineEntity>(rotated[0].geometry).start_point.y, 1.0);

    const auto scaled = gripTransformedEntities(sources, VpGripOperation::Scale, {0.0, 0.0},
                                                {1.0, 0.0}, {2.0, 0.0});
    QCOMPARE(std::get<VpLineEntity>(scaled[0].geometry).end_point.x, 6.0);
    QCOMPARE(std::get<VpLineEntity>(scaled[1].geometry).end_point.y, 8.0);

    const auto mirrored = gripTransformedEntities(sources, VpGripOperation::Mirror, {0.0, 0.0},
                                                  {1.0, 0.0}, {0.0, 1.0});
    QCOMPARE(std::get<VpLineEntity>(mirrored[0].geometry).start_point.x, -1.0);
    QCOMPARE(std::get<VpLineEntity>(mirrored[1].geometry).end_point.x, 0.0);
}

void VpGripEditTest::gripOperationCycle()
{
    VpCadViewport viewport;
    QCOMPARE(viewport.gripOperation(), VpGripOperation::Stretch);
    viewport.cycleGripOperation();
    QCOMPARE(viewport.gripOperation(), VpGripOperation::Move);
    viewport.cycleGripOperation();
    QCOMPARE(viewport.gripOperation(), VpGripOperation::Rotate);
    viewport.cycleGripOperation();
    QCOMPARE(viewport.gripOperation(), VpGripOperation::Scale);
    viewport.cycleGripOperation();
    QCOMPARE(viewport.gripOperation(), VpGripOperation::Mirror);
    viewport.cycleGripOperation();
    QCOMPARE(viewport.gripOperation(), VpGripOperation::Stretch);
}

void VpGripEditTest::viewportGripDragAndUndo()
{
    VpCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("grip source"));
    transaction->addLine({0.0, 0.0}, {10.0, 0.0});
    transaction->commit();

    VpCadViewport viewport;
    viewport.resize(640, 480);
    viewport.setDocument(&document);
    viewport.selectEntitiesInWindow({-1.0, -1.0}, {11.0, 1.0}, false);
    QCOMPARE(viewport.selectedEntityIds().size(), 1);

    QTest::mousePress(&viewport, Qt::LeftButton, Qt::NoModifier, QPoint(320, 240));
    QTest::mouseMove(&viewport, QPoint(300, 220), 1);
    QTest::mouseRelease(&viewport, Qt::LeftButton, Qt::NoModifier, QPoint(300, 220));
    const auto& edited_line = std::get<VpLineEntity>(document.entities().front().geometry);
    QCOMPARE(edited_line.start_point.x, -20.0);
    QCOMPARE(edited_line.start_point.y, 20.0);

    document.undo();
    const auto& restored_line = std::get<VpLineEntity>(document.entities().front().geometry);
    QCOMPARE(restored_line.start_point.x, 0.0);
    QCOMPARE(restored_line.start_point.y, 0.0);
}

void VpGripEditTest::viewportMultiEntityGripMoveAndUndo()
{
    VpCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("multi grip source"));
    transaction->addLine({-10.0, 0.0}, {0.0, 0.0});
    transaction->addLine({10.0, 0.0}, {20.0, 0.0});
    transaction->commit();

    VpCadViewport viewport;
    viewport.resize(640, 480);
    viewport.setDocument(&document);
    viewport.setObjectSnapEnabled(false);
    viewport.selectAll();
    viewport.beginGripEdit(VpGripOperation::Move);
    QCOMPARE(viewport.selectedEntityIds().size(), 2);

    QTest::mousePress(&viewport, Qt::LeftButton, Qt::NoModifier, QPoint(315, 240));
    QTest::mouseMove(&viewport, QPoint(325, 230), 1);
    QTest::mouseRelease(&viewport, Qt::LeftButton, Qt::NoModifier, QPoint(325, 230));
    const auto& first_moved = std::get<VpLineEntity>(document.entities()[0].geometry);
    const auto& second_moved = std::get<VpLineEntity>(document.entities()[1].geometry);
    QCOMPARE(first_moved.start_point.x, 0.0);
    QCOMPARE(first_moved.start_point.y, 10.0);
    QCOMPARE(second_moved.start_point.x, 20.0);
    QCOMPARE(second_moved.start_point.y, 10.0);

    document.undo();
    const auto& first_restored = std::get<VpLineEntity>(document.entities()[0].geometry);
    const auto& second_restored = std::get<VpLineEntity>(document.entities()[1].geometry);
    QCOMPARE(first_restored.start_point.x, -10.0);
    QCOMPARE(first_restored.start_point.y, 0.0);
    QCOMPARE(second_restored.start_point.x, 10.0);
    QCOMPARE(second_restored.start_point.y, 0.0);
}

void VpGripEditTest::lengthenGeometryAndUndo()
{
    VpEntityRecord line;
    line.id = 310;
    line.type = VpEntityType::Line;
    line.geometry = VpLineEntity{{0.0, 0.0}, {10.0, 0.0}};
    VpEntityRecord edited;
    QVERIFY(lengthenedEntity(line, {10.0, 0.0}, {15.0, 4.0}, edited));
    QCOMPARE(std::get<VpLineEntity>(edited.geometry).end_point.x, 15.0);
    QCOMPARE(std::get<VpLineEntity>(edited.geometry).end_point.y, 0.0);

    VpEntityRecord arc;
    arc.id = 311;
    arc.type = VpEntityType::Arc;
    arc.geometry = VpArcEntity{{0.0, 0.0}, 5.0, 0.0, 90.0};
    QVERIFY(lengthenedEntity(arc, {0.0, 5.0}, {-5.0, 0.0}, edited));
    QVERIFY(std::abs(std::get<VpArcEntity>(edited.geometry).end_angle - 180.0) < 1.0e-9);

    VpEntityRecord polyline;
    polyline.id = 312;
    polyline.type = VpEntityType::Polyline;
    polyline.geometry = VpPolylineEntity{{{-5.0, 0.0}, {5.0, 0.0}}, false, {1.0, 0.0}};
    QVERIFY(lengthenedEntity(polyline, {5.0, 0.0}, {0.0, -5.0}, edited));
    const auto& shortened = std::get<VpPolylineEntity>(edited.geometry);
    QVERIFY(std::abs(shortened.vertices.back().x) < 1.0e-9);
    QVERIFY(std::abs(shortened.vertices.back().y + 5.0) < 1.0e-9);
    QVERIFY(std::abs(shortened.bulges.front() - std::tan(3.14159265358979323846 / 8.0)) < 1.0e-9);

    VpCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("lengthen source"));
    transaction->addLine({0.0, 0.0}, {10.0, 0.0});
    transaction->commit();
    VpCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setToolMode(VpToolMode::Lengthen);
    viewport.submitWorldPoint({10.0, 0.0});
    viewport.submitWorldPoint({15.0, 2.0});
    QCOMPARE(std::get<VpLineEntity>(document.entities().front().geometry).end_point.x, 15.0);
    document.undo();
    QCOMPARE(std::get<VpLineEntity>(document.entities().front().geometry).end_point.x, 10.0);
}

} // namespace Vp

QTEST_MAIN(Vp::VpGripEditTest)
#include "vp_grip_edit_test.moc"
