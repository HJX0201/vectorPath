#include "vp_cad_document.h"
#include "vp_cad_viewport.h"
#include "vp_cad_viewport_geometry.h"
#include "vp_document_transaction.h"
#include "vp_object_snap.h"
#include "vp_test_runner.h"

#include <QtTest>
#include <algorithm>
#include <cmath>

namespace Vp
{

class VpCadViewportTest final : public QObject
{
    Q_OBJECT

  private slots:
    void windowAndCrossingSelection();
    void selectsHatchByInteriorClick();
    void selectAllAndDelete();
    void threePointArcGeometry();
    void modificationPreviewGeometry();
    void curveTrimGeometryAndUndo();
    void curveExtendGeometryAndUndo();
    void curveBreakGeometryAndUndo();
    void curveJoinGeometryAndUndo();
    void explodeGeometry();
    void stretchGeometryAndUndo();
    void filletGeometryAndUndo();
    void chamferGeometryAndUndo();
    void objectSnapGeometry();
};

void VpCadViewportTest::windowAndCrossingSelection()
{
    VpCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("selection geometry"));
    transaction->addLine({0.0, 0.0}, {10.0, 0.0});
    transaction->addLine({5.0, -5.0}, {5.0, 5.0});
    transaction->commit();

    VpCadViewport viewport;
    viewport.setDocument(&document);
    viewport.selectEntitiesInWindow({-1.0, -1.0}, {11.0, 1.0}, false);
    QCOMPARE(viewport.selectedEntityIds().size(), 1);

    viewport.selectEntitiesInWindow({-1.0, -1.0}, {11.0, 1.0}, true);
    QCOMPARE(viewport.selectedEntityIds().size(), 2);
}

void VpCadViewportTest::selectsHatchByInteriorClick()
{
    VpCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("hatch click selection"));
    VpHatchEntity hatch;
    hatch.boundary = {
        {-20.0, -20.0},
        {20.0, -20.0},
        {20.0, 20.0},
        {-20.0, 20.0},
    };
    transaction->addHatch(std::move(hatch));
    transaction->commit();

    VpCadViewport viewport;
    viewport.resize(640, 480);
    viewport.setDocument(&document);
    viewport.show();
    QCoreApplication::processEvents();
    QTest::mouseClick(&viewport, Qt::LeftButton, Qt::NoModifier, viewport.rect().center());
    QCOMPARE(viewport.selectedEntityIds().size(), 1);
    QCOMPARE(viewport.selectedEntityId().value(), document.entities().front().id);
}

void VpCadViewportTest::selectAllAndDelete()
{
    VpCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("delete geometry"));
    transaction->addCircle({0.0, 0.0}, 4.0);
    transaction->addCircle({20.0, 0.0}, 3.0);
    transaction->commit();

    VpCadViewport viewport;
    viewport.setDocument(&document);
    viewport.selectAll();
    QCOMPARE(viewport.selectedEntityIds().size(), 2);
    viewport.deleteSelected();
    QVERIFY(document.entities().empty());
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(2));
}

void VpCadViewportTest::threePointArcGeometry()
{
    VpPoint2d center;
    double radius = 0.0;
    double start_angle = 0.0;
    double end_angle = 0.0;
    QVERIFY(calculateThreePointArc({1.0, 0.0}, {0.0, 1.0}, {-1.0, 0.0}, center, radius, start_angle,
                                   end_angle));
    QVERIFY(std::abs(center.x) < 1.0e-9);
    QVERIFY(std::abs(center.y) < 1.0e-9);
    QVERIFY(std::abs(radius - 1.0) < 1.0e-9);
    QVERIFY(std::abs(start_angle) < 1.0e-9);
    QVERIFY(std::abs(end_angle - 180.0) < 1.0e-9);

    QVERIFY(!calculateThreePointArc({0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0}, center, radius, start_angle,
                                    end_angle));
}

void VpCadViewportTest::modificationPreviewGeometry()
{
    VpEntityRecord source_line;
    source_line.id = 1;
    source_line.type = VpEntityType::Line;
    source_line.geometry = VpLineEntity{{0.0, 0.0}, {10.0, 0.0}};

    const VpEntityRecord moved_line = translatedEntity(source_line, 3.0, 4.0);
    QCOMPARE(std::get<VpLineEntity>(moved_line.geometry).start_point.x, 3.0);
    QCOMPARE(std::get<VpLineEntity>(moved_line.geometry).start_point.y, 4.0);

    const VpEntityRecord rotated_line = rotatedEntity(source_line, {0.0, 0.0}, 90.0);
    QVERIFY(std::abs(std::get<VpLineEntity>(rotated_line.geometry).end_point.x) < 1.0e-9);
    QVERIFY(std::abs(std::get<VpLineEntity>(rotated_line.geometry).end_point.y - 10.0) < 1.0e-9);

    const VpEntityRecord scaled_line = scaledEntity(source_line, {0.0, 0.0}, 2.5);
    QCOMPARE(std::get<VpLineEntity>(scaled_line.geometry).end_point.x, 25.0);

    const VpEntityRecord mirrored_line = mirroredEntity(source_line, {0.0, -1.0}, {0.0, 1.0});
    QCOMPARE(std::get<VpLineEntity>(mirrored_line.geometry).start_point.x, 0.0);
    QCOMPARE(std::get<VpLineEntity>(mirrored_line.geometry).end_point.x, -10.0);

    VpEntityRecord offset_line;
    QVERIFY(offsetEntity(source_line, {5.0, 4.0}, offset_line));
    QCOMPARE(std::get<VpLineEntity>(offset_line.geometry).start_point.y, 4.0);
    QCOMPARE(std::get<VpLineEntity>(offset_line.geometry).end_point.y, 4.0);

    VpEntityRecord cutting_line;
    cutting_line.id = 2;
    cutting_line.type = VpEntityType::Line;
    cutting_line.geometry = VpLineEntity{{5.0, -5.0}, {5.0, 5.0}};
    VpEntityRecord trimmed_line;
    QVERIFY(trimmedLineEntity(cutting_line, source_line, {1.0, 0.0}, trimmed_line));
    QCOMPARE(std::get<VpLineEntity>(trimmed_line.geometry).start_point.x, 5.0);
    QCOMPARE(std::get<VpLineEntity>(trimmed_line.geometry).end_point.x, 10.0);

    VpEntityRecord boundary_line;
    boundary_line.id = 3;
    boundary_line.type = VpEntityType::Line;
    boundary_line.geometry = VpLineEntity{{10.0, -5.0}, {10.0, 5.0}};
    VpEntityRecord short_line;
    short_line.id = 4;
    short_line.type = VpEntityType::Line;
    short_line.geometry = VpLineEntity{{0.0, 0.0}, {4.0, 0.0}};
    VpEntityRecord extended_line;
    QVERIFY(extendedLineEntity(boundary_line, short_line, extended_line));
    QCOMPARE(std::get<VpLineEntity>(extended_line.geometry).start_point.x, 0.0);
    QCOMPARE(std::get<VpLineEntity>(extended_line.geometry).end_point.x, 10.0);
    QVERIFY(!extendedLineEntity(boundary_line, source_line, extended_line));

    const std::vector<VpEntityRecord> broken_lines =
        brokenLineEntities(source_line, {3.0, 0.0}, {7.0, 0.0});
    QCOMPARE(broken_lines.size(), std::size_t(2));
    QCOMPARE(std::get<VpLineEntity>(broken_lines[0].geometry).end_point.x, 3.0);
    QCOMPARE(std::get<VpLineEntity>(broken_lines[1].geometry).start_point.x, 7.0);
    const std::vector<VpEntityRecord> split_lines =
        brokenLineEntities(source_line, {5.0, 0.0}, {5.0, 0.0});
    QCOMPARE(split_lines.size(), std::size_t(2));
    QCOMPARE(std::get<VpLineEntity>(split_lines[0].geometry).end_point.x, 5.0);
    QCOMPARE(std::get<VpLineEntity>(split_lines[1].geometry).start_point.x, 5.0);
    QVERIFY(brokenLineEntities(source_line, {0.0, 0.0}, {0.0, 0.0}).empty());

    VpEntityRecord join_first = source_line;
    join_first.id = 10;
    join_first.geometry = VpLineEntity{{0.0, 0.0}, {5.0, 0.0}};
    VpEntityRecord join_second = source_line;
    join_second.id = 11;
    join_second.geometry = VpLineEntity{{10.0, 0.0}, {5.0, 0.0}};
    VpEntityRecord join_third = source_line;
    join_third.id = 12;
    join_third.geometry = VpLineEntity{{10.0, 0.0}, {10.0, 5.0}};
    VpEntityRecord joined_entity;
    QVERIFY(joinedLineEntity({join_first, join_third, join_second}, 1.0e-6, joined_entity));
    QCOMPARE(joined_entity.type, VpEntityType::Polyline);
    const auto& joined_polyline = std::get<VpPolylineEntity>(joined_entity.geometry);
    QCOMPARE(joined_polyline.vertices.size(), std::size_t(4));
    QVERIFY(!joined_polyline.is_closed);

    VpEntityRecord triangle_second = source_line;
    triangle_second.id = 13;
    triangle_second.geometry = VpLineEntity{{0.0, 5.0}, {0.0, 0.0}};
    VpEntityRecord triangle_third = source_line;
    triangle_third.id = 14;
    triangle_third.geometry = VpLineEntity{{5.0, 0.0}, {0.0, 5.0}};
    QVERIFY(joinedLineEntity({join_first, triangle_second, triangle_third}, 1.0e-6, joined_entity));
    const auto& closed_polyline = std::get<VpPolylineEntity>(joined_entity.geometry);
    QCOMPARE(closed_polyline.vertices.size(), std::size_t(3));
    QVERIFY(closed_polyline.is_closed);
    QVERIFY(!joinedLineEntity({join_first, boundary_line}, 1.0e-6, joined_entity));
}

void VpCadViewportTest::curveTrimGeometryAndUndo()
{
    VpEntityRecord cutting_line;
    cutting_line.id = 90;
    cutting_line.type = VpEntityType::Line;
    cutting_line.geometry = VpLineEntity{{0.0, -10.0}, {0.0, 10.0}};
    VpEntityRecord circle;
    circle.id = 91;
    circle.type = VpEntityType::Circle;
    circle.layer_name = QStringLiteral("curves");
    circle.line_width_mm = 0.5;
    circle.geometry = VpCircleEntity{{0.0, 0.0}, 5.0};
    const std::vector<VpEntityRecord> circle_parts =
        trimmedEntityParts(cutting_line, circle, {5.0, 0.0});
    QCOMPARE(circle_parts.size(), std::size_t(1));
    QCOMPARE(circle_parts.front().type, VpEntityType::Arc);
    const auto& remaining_arc = std::get<VpArcEntity>(circle_parts.front().geometry);
    QCOMPARE(remaining_arc.start_angle, 90.0);
    QCOMPARE(remaining_arc.end_angle, 270.0);
    QCOMPARE(circle_parts.front().layer_name, circle.layer_name);

    VpEntityRecord target_line = cutting_line;
    target_line.id = 92;
    target_line.geometry = VpLineEntity{{-10.0, 0.0}, {10.0, 0.0}};
    const std::vector<VpEntityRecord> line_parts =
        trimmedEntityParts(circle, target_line, {0.0, 0.0});
    QCOMPARE(line_parts.size(), std::size_t(2));
    QCOMPARE(std::get<VpLineEntity>(line_parts[0].geometry).end_point.x, -5.0);
    QCOMPARE(std::get<VpLineEntity>(line_parts[1].geometry).start_point.x, 5.0);

    VpEntityRecord target_arc = circle;
    target_arc.id = 93;
    target_arc.type = VpEntityType::Arc;
    target_arc.geometry = VpArcEntity{{0.0, 0.0}, 5.0, 0.0, 180.0};
    const std::vector<VpEntityRecord> arc_parts =
        trimmedEntityParts(cutting_line, target_arc, {3.5, 3.5});
    QCOMPARE(arc_parts.size(), std::size_t(1));
    QCOMPARE(std::get<VpArcEntity>(arc_parts[0].geometry).start_angle, 90.0);
    QCOMPARE(std::get<VpArcEntity>(arc_parts[0].geometry).end_angle, 180.0);

    VpEntityRecord target_polyline;
    target_polyline.id = 94;
    target_polyline.type = VpEntityType::Polyline;
    target_polyline.layer_name = QStringLiteral("curves");
    target_polyline.geometry = VpPolylineEntity{{{-10.0, 0.0}, {-5.0, 0.0}, {10.0, 0.0}}, false};
    const std::vector<VpEntityRecord> polyline_parts =
        trimmedEntityParts(cutting_line, target_polyline, {-2.0, 0.0});
    QCOMPARE(polyline_parts.size(), std::size_t(2));
    QCOMPARE(std::get<VpPolylineEntity>(polyline_parts[0].geometry).vertices.back().x, -5.0);
    QCOMPARE(std::get<VpPolylineEntity>(polyline_parts[1].geometry).vertices.front().x, 0.0);
    QCOMPARE(polyline_parts[1].layer_name, target_polyline.layer_name);

    target_polyline.id = 95;
    target_polyline.geometry =
        VpPolylineEntity{{{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {0.0, 10.0}}, true};
    VpEntityRecord short_cutting_line = cutting_line;
    short_cutting_line.geometry = VpLineEntity{{5.0, -2.0}, {5.0, 2.0}};
    const std::vector<VpEntityRecord> closed_polyline_parts =
        trimmedEntityParts(short_cutting_line, target_polyline, {2.0, 0.0});
    QCOMPARE(closed_polyline_parts.size(), std::size_t(1));
    const auto& opened_polyline =
        std::get<VpPolylineEntity>(closed_polyline_parts.front().geometry);
    QVERIFY(!opened_polyline.is_closed);
    QCOMPARE(opened_polyline.vertices.front().x, 5.0);
    QCOMPARE(opened_polyline.vertices.back().x, 0.0);

    VpCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("curve trim"));
    transaction->addLine({0.0, -10.0}, {0.0, 10.0});
    transaction->addCircle({0.0, 0.0}, 5.0);
    transaction->commit();
    VpCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setToolMode(VpToolMode::Trim);
    viewport.submitWorldPoint({0.0, 8.0});
    viewport.submitWorldPoint({5.0, 0.0});
    QCOMPARE(document.entities().size(), std::size_t(2));
    QVERIFY(std::any_of(document.entities().begin(), document.entities().end(),
                        [](const VpEntityRecord& entity)
                        {
                            return entity.type == VpEntityType::Arc;
                        }));
    document.undo();
    QVERIFY(std::any_of(document.entities().begin(), document.entities().end(),
                        [](const VpEntityRecord& entity)
                        {
                            return entity.type == VpEntityType::Circle;
                        }));
}

void VpCadViewportTest::curveExtendGeometryAndUndo()
{
    VpEntityRecord circle_boundary;
    circle_boundary.id = 100;
    circle_boundary.type = VpEntityType::Circle;
    circle_boundary.geometry = VpCircleEntity{{0.0, 0.0}, 10.0};
    VpEntityRecord target_line;
    target_line.id = 101;
    target_line.type = VpEntityType::Line;
    target_line.layer_name = QStringLiteral("extend_curves");
    target_line.line_width_mm = 0.5;
    target_line.geometry = VpLineEntity{{0.0, 0.0}, {5.0, 0.0}};
    VpEntityRecord extended;
    QVERIFY(extendedEntity(circle_boundary, target_line, {5.0, 0.0}, extended));
    QCOMPARE(std::get<VpLineEntity>(extended.geometry).end_point.x, 10.0);
    QCOMPARE(extended.layer_name, target_line.layer_name);

    VpEntityRecord line_boundary;
    line_boundary.id = 102;
    line_boundary.type = VpEntityType::Line;
    line_boundary.geometry = VpLineEntity{{-10.0, 0.0}, {0.0, 0.0}};
    VpEntityRecord target_arc;
    target_arc.id = 103;
    target_arc.type = VpEntityType::Arc;
    target_arc.geometry = VpArcEntity{{0.0, 0.0}, 5.0, 0.0, 90.0};
    QVERIFY(extendedEntity(line_boundary, target_arc, {0.0, 5.0}, extended));
    QCOMPARE(std::get<VpArcEntity>(extended.geometry).start_angle, 0.0);
    QCOMPARE(std::get<VpArcEntity>(extended.geometry).end_angle, 180.0);

    VpEntityRecord target_polyline;
    target_polyline.id = 104;
    target_polyline.type = VpEntityType::Polyline;
    target_polyline.layer_name = QStringLiteral("extend_curves");
    target_polyline.geometry = VpPolylineEntity{{{0.0, 0.0}, {3.0, 0.0}, {5.0, 0.0}}, false};
    QVERIFY(extendedEntity(circle_boundary, target_polyline, {5.0, 0.0}, extended));
    QCOMPARE(std::get<VpPolylineEntity>(extended.geometry).vertices.back().x, 10.0);
    QCOMPARE(extended.layer_name, target_polyline.layer_name);
    target_polyline.geometry = VpPolylineEntity{{{0.0, 0.0}, {3.0, 0.0}, {5.0, 0.0}}, true};
    QVERIFY(!extendedEntity(circle_boundary, target_polyline, {5.0, 0.0}, extended));

    VpCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("curve extend"));
    transaction->addCircle({0.0, 0.0}, 10.0);
    const VpEntityId line_id = transaction->addLine({0.0, 0.0}, {5.0, 0.0});
    transaction->commit();
    VpCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setToolMode(VpToolMode::Extend);
    viewport.submitWorldPoint({0.0, 10.0});
    viewport.submitWorldPoint({5.0, 0.0});
    const auto extended_iterator =
        std::find_if(document.entities().begin(), document.entities().end(),
                     [line_id](const VpEntityRecord& entity)
                     {
                         return entity.id == line_id;
                     });
    QVERIFY(extended_iterator != document.entities().end());
    QCOMPARE(std::get<VpLineEntity>(extended_iterator->geometry).end_point.x, 10.0);
    document.undo();
    const auto restored_iterator =
        std::find_if(document.entities().begin(), document.entities().end(),
                     [line_id](const VpEntityRecord& entity)
                     {
                         return entity.id == line_id;
                     });
    QVERIFY(restored_iterator != document.entities().end());
    QCOMPARE(std::get<VpLineEntity>(restored_iterator->geometry).end_point.x, 5.0);
}

void VpCadViewportTest::curveBreakGeometryAndUndo()
{
    VpEntityRecord circle;
    circle.id = 110;
    circle.type = VpEntityType::Circle;
    circle.layer_name = QStringLiteral("break_curves");
    circle.line_width_mm = 0.5;
    circle.geometry = VpCircleEntity{{0.0, 0.0}, 5.0};
    const std::vector<VpEntityRecord> circle_parts =
        brokenEntityParts(circle, {5.0, 0.0}, {0.0, 5.0});
    QCOMPARE(circle_parts.size(), std::size_t(1));
    QCOMPARE(circle_parts.front().type, VpEntityType::Arc);
    const auto& circle_remainder = std::get<VpArcEntity>(circle_parts.front().geometry);
    QCOMPARE(circle_remainder.start_angle, 90.0);
    QCOMPARE(circle_remainder.end_angle, 0.0);
    QCOMPARE(circle_parts.front().layer_name, circle.layer_name);

    VpEntityRecord arc = circle;
    arc.id = 111;
    arc.type = VpEntityType::Arc;
    arc.geometry = VpArcEntity{{0.0, 0.0}, 5.0, 0.0, 180.0};
    const double diagonal = 5.0 / std::sqrt(2.0);
    const std::vector<VpEntityRecord> arc_parts =
        brokenEntityParts(arc, {diagonal, diagonal}, {-diagonal, diagonal});
    QCOMPARE(arc_parts.size(), std::size_t(2));
    QCOMPARE(std::get<VpArcEntity>(arc_parts[0].geometry).start_angle, 0.0);
    QCOMPARE(std::get<VpArcEntity>(arc_parts[0].geometry).end_angle, 45.0);
    QCOMPARE(std::get<VpArcEntity>(arc_parts[1].geometry).start_angle, 135.0);
    QCOMPARE(std::get<VpArcEntity>(arc_parts[1].geometry).end_angle, 180.0);

    VpCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("curve break"));
    transaction->addCircle({0.0, 0.0}, 5.0);
    transaction->commit();
    VpCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setToolMode(VpToolMode::Break);
    viewport.submitWorldPoint({5.0, 0.0});
    viewport.submitWorldPoint({0.0, 5.0});
    QCOMPARE(document.entities().size(), std::size_t(1));
    QCOMPARE(document.entities().front().type, VpEntityType::Arc);
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(1));
    QCOMPARE(document.entities().front().type, VpEntityType::Circle);
}

void VpCadViewportTest::curveJoinGeometryAndUndo()
{
    VpEntityRecord first_arc;
    first_arc.id = 120;
    first_arc.type = VpEntityType::Arc;
    first_arc.layer_name = QStringLiteral("join_curves");
    first_arc.line_width_mm = 0.5;
    first_arc.geometry = VpArcEntity{{0.0, 0.0}, 5.0, 0.0, 90.0};
    VpEntityRecord second_arc = first_arc;
    second_arc.id = 121;
    second_arc.geometry = VpArcEntity{{0.0, 0.0}, 5.0, 90.0, 180.0};
    VpEntityRecord joined;
    QVERIFY(joinedEntity({second_arc, first_arc}, 1.0e-6, joined));
    QCOMPARE(joined.type, VpEntityType::Arc);
    QCOMPARE(std::get<VpArcEntity>(joined.geometry).start_angle, 0.0);
    QCOMPARE(std::get<VpArcEntity>(joined.geometry).end_angle, 180.0);
    QCOMPARE(joined.layer_name, first_arc.layer_name);

    VpEntityRecord third_arc = first_arc;
    third_arc.id = 122;
    third_arc.geometry = VpArcEntity{{0.0, 0.0}, 5.0, 180.0, 270.0};
    VpEntityRecord fourth_arc = first_arc;
    fourth_arc.id = 123;
    fourth_arc.geometry = VpArcEntity{{0.0, 0.0}, 5.0, 270.0, 0.0};
    QVERIFY(joinedEntity({third_arc, first_arc, fourth_arc, second_arc}, 1.0e-6, joined));
    QCOMPARE(joined.type, VpEntityType::Circle);

    VpCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("curve join"));
    transaction->addArc({0.0, 0.0}, 5.0, 0.0, 90.0);
    transaction->addArc({0.0, 0.0}, 5.0, 90.0, 180.0);
    transaction->commit();
    VpCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setToolMode(VpToolMode::Join);
    const double diagonal = 5.0 / std::sqrt(2.0);
    viewport.submitWorldPoint({diagonal, diagonal});
    viewport.submitWorldPoint({-diagonal, diagonal});
    QTest::keyClick(&viewport, Qt::Key_Return);
    QCOMPARE(document.entities().size(), std::size_t(1));
    QCOMPARE(document.entities().front().type, VpEntityType::Arc);
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(2));

    VpEntityRecord source_polyline;
    source_polyline.id = 124;
    source_polyline.type = VpEntityType::Polyline;
    source_polyline.layer_name = QStringLiteral("join_linear");
    source_polyline.geometry = VpPolylineEntity{{{0.0, 0.0}, {5.0, 0.0}, {10.0, 0.0}}, false};
    VpEntityRecord appended_line;
    appended_line.id = 125;
    appended_line.type = VpEntityType::Line;
    appended_line.geometry = VpLineEntity{{10.0, 0.0}, {15.0, 0.0}};
    QVERIFY(joinedEntity({appended_line, source_polyline}, 1.0e-6, joined));
    QCOMPARE(joined.type, VpEntityType::Polyline);
    const auto& joined_linear = std::get<VpPolylineEntity>(joined.geometry);
    QCOMPARE(joined_linear.vertices.size(), std::size_t(4));
    QCOMPARE(joined_linear.vertices.front().x, 0.0);
    QCOMPARE(joined_linear.vertices.back().x, 15.0);

    VpCadDocument linear_document;
    auto linear_transaction = linear_document.beginTransaction(QStringLiteral("polyline join"));
    linear_transaction->addPolyline({{0.0, 0.0}, {5.0, 0.0}, {10.0, 0.0}}, false);
    linear_transaction->addLine({10.0, 0.0}, {15.0, 0.0});
    linear_transaction->commit();
    VpCadViewport linear_viewport;
    linear_viewport.setDocument(&linear_document);
    linear_viewport.setToolMode(VpToolMode::Join);
    linear_viewport.submitWorldPoint({2.5, 0.0});
    linear_viewport.submitWorldPoint({12.5, 0.0});
    QTest::keyClick(&linear_viewport, Qt::Key_Return);
    QCOMPARE(linear_document.entities().size(), std::size_t(1));
    QCOMPARE(linear_document.entities().front().type, VpEntityType::Polyline);
    linear_document.undo();
    QCOMPARE(linear_document.entities().size(), std::size_t(2));
}

void VpCadViewportTest::explodeGeometry()
{
    VpEntityRecord open_polyline;
    open_polyline.id = 20;
    open_polyline.type = VpEntityType::Polyline;
    open_polyline.layer_name = QStringLiteral("construction");
    open_polyline.line_width_mm = 0.5;
    open_polyline.geometry = VpPolylineEntity{{{0.0, 0.0}, {5.0, 0.0}, {5.0, 4.0}}, false};

    const std::vector<VpEntityRecord> open_parts = explodedEntityParts(open_polyline);
    QCOMPARE(open_parts.size(), std::size_t(2));
    QCOMPARE(open_parts[0].type, VpEntityType::Line);
    QCOMPARE(open_parts[0].layer_name, QStringLiteral("construction"));
    QCOMPARE(open_parts[0].line_width_mm, 0.5);
    QCOMPARE(std::get<VpLineEntity>(open_parts[0].geometry).start_point.x, 0.0);
    QCOMPARE(std::get<VpLineEntity>(open_parts[1].geometry).end_point.y, 4.0);

    VpEntityRecord closed_polyline = open_polyline;
    closed_polyline.id = 21;
    closed_polyline.geometry = VpPolylineEntity{{{0.0, 0.0}, {5.0, 0.0}, {0.0, 4.0}}, true};
    const std::vector<VpEntityRecord> closed_parts = explodedEntityParts(closed_polyline);
    QCOMPARE(closed_parts.size(), std::size_t(3));
    QCOMPARE(std::get<VpLineEntity>(closed_parts.back().geometry).start_point.y, 4.0);
    QCOMPARE(std::get<VpLineEntity>(closed_parts.back().geometry).end_point.x, 0.0);
    QCOMPARE(std::get<VpLineEntity>(closed_parts.back().geometry).end_point.y, 0.0);

    VpEntityRecord hatch = open_polyline;
    hatch.id = 22;
    hatch.type = VpEntityType::Hatch;
    hatch.geometry = VpHatchEntity{{{1.0, 1.0}, {6.0, 1.0}, {1.0, 5.0}}};
    const std::vector<VpEntityRecord> hatch_parts = explodedEntityParts(hatch);
    QCOMPARE(hatch_parts.size(), std::size_t(3));
    for (const VpEntityRecord& hatch_part : hatch_parts)
    {
        QCOMPARE(hatch_part.type, VpEntityType::Line);
        QCOMPARE(hatch_part.layer_name, hatch.layer_name);
        QCOMPARE(hatch_part.line_width_mm, hatch.line_width_mm);
    }

    VpEntityRecord line = open_polyline;
    line.type = VpEntityType::Line;
    line.geometry = VpLineEntity{{0.0, 0.0}, {1.0, 1.0}};
    QVERIFY(explodedEntityParts(line).empty());
}

void VpCadViewportTest::stretchGeometryAndUndo()
{
    VpEntityRecord source;
    source.id = 30;
    source.type = VpEntityType::Polyline;
    source.layer_name = QStringLiteral("detail");
    source.line_width_mm = 0.7;
    source.geometry = VpPolylineEntity{{{0.0, 0.0}, {5.0, 0.0}, {10.0, 0.0}}, false};
    VpEntityRecord stretched;
    QVERIFY(stretchedEntity(source, {4.0, -1.0}, {11.0, 1.0}, 0.0, 3.0, stretched));
    const auto& stretched_polyline = std::get<VpPolylineEntity>(stretched.geometry);
    QCOMPARE(stretched_polyline.vertices[0].y, 0.0);
    QCOMPARE(stretched_polyline.vertices[1].y, 3.0);
    QCOMPARE(stretched_polyline.vertices[2].y, 3.0);
    QCOMPARE(stretched.layer_name, source.layer_name);
    QCOMPARE(stretched.line_width_mm, source.line_width_mm);
    QVERIFY(!stretchedEntity(source, {20.0, 20.0}, {30.0, 30.0}, 1.0, 1.0, stretched));

    VpCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("stretch line"));
    transaction->addLine({0.0, 0.0}, {10.0, 0.0});
    transaction->commit();
    VpCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setToolMode(VpToolMode::Stretch);
    viewport.submitWorldPoint({8.0, -2.0});
    viewport.submitWorldPoint({12.0, 2.0});
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({0.0, 5.0});

    QCOMPARE(document.entities().size(), std::size_t(1));
    const auto& stretched_line = std::get<VpLineEntity>(document.entities().front().geometry);
    QCOMPARE(stretched_line.start_point.y, 0.0);
    QCOMPARE(stretched_line.end_point.x, 10.0);
    QCOMPARE(stretched_line.end_point.y, 5.0);
    document.undo();
    const auto& original_line = std::get<VpLineEntity>(document.entities().front().geometry);
    QCOMPARE(original_line.start_point.y, 0.0);
    QCOMPARE(original_line.end_point.y, 0.0);
}

void VpCadViewportTest::filletGeometryAndUndo()
{
    VpEntityRecord horizontal;
    horizontal.id = 40;
    horizontal.type = VpEntityType::Line;
    horizontal.layer_name = QStringLiteral("profile");
    horizontal.line_width_mm = 0.35;
    horizontal.geometry = VpLineEntity{{0.0, 0.0}, {20.0, 0.0}};
    VpEntityRecord vertical = horizontal;
    vertical.id = 41;
    vertical.geometry = VpLineEntity{{0.0, 0.0}, {0.0, 20.0}};
    VpEntityRecord first_result;
    VpEntityRecord second_result;
    VpEntityRecord arc_result;
    QVERIFY(filletedLineEntities(horizontal, vertical, {15.0, 0.0}, {0.0, 15.0}, 5.0, first_result,
                                 second_result, arc_result));
    const auto& first_line = std::get<VpLineEntity>(first_result.geometry);
    const auto& second_line = std::get<VpLineEntity>(second_result.geometry);
    const auto& fillet_arc = std::get<VpArcEntity>(arc_result.geometry);
    QCOMPARE(first_line.end_point.x, 5.0);
    QCOMPARE(first_line.end_point.y, 0.0);
    QCOMPARE(second_line.end_point.x, 0.0);
    QCOMPARE(second_line.end_point.y, 5.0);
    QCOMPARE(fillet_arc.center.x, 5.0);
    QCOMPARE(fillet_arc.center.y, 5.0);
    QCOMPARE(fillet_arc.radius, 5.0);
    QCOMPARE(arc_result.layer_name, horizontal.layer_name);
    QCOMPARE(arc_result.line_width_mm, horizontal.line_width_mm);

    VpEntityRecord parallel = horizontal;
    parallel.id = 42;
    parallel.geometry = VpLineEntity{{0.0, 10.0}, {20.0, 10.0}};
    QVERIFY(!filletedLineEntities(horizontal, parallel, {15.0, 0.0}, {15.0, 10.0}, 5.0,
                                  first_result, second_result, arc_result));

    VpCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("fillet lines"));
    transaction->addLine({0.0, 0.0}, {20.0, 0.0});
    transaction->addLine({0.0, 0.0}, {0.0, 20.0});
    transaction->commit();
    VpCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setFilletRadius(2.0);
    viewport.setToolMode(VpToolMode::Fillet);
    viewport.submitWorldPoint({10.0, 0.0});
    viewport.submitWorldPoint({0.0, 10.0});
    QCOMPARE(document.entities().size(), std::size_t(3));
    QCOMPARE(document.entities().back().type, VpEntityType::Arc);
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(2));
    QCOMPARE(std::get<VpLineEntity>(document.entities()[0].geometry).end_point.x, 20.0);
    QCOMPARE(std::get<VpLineEntity>(document.entities()[1].geometry).end_point.y, 20.0);
}

void VpCadViewportTest::chamferGeometryAndUndo()
{
    VpEntityRecord horizontal;
    horizontal.id = 50;
    horizontal.type = VpEntityType::Line;
    horizontal.layer_name = QStringLiteral("outline");
    horizontal.line_width_mm = 0.5;
    horizontal.geometry = VpLineEntity{{0.0, 0.0}, {20.0, 0.0}};
    VpEntityRecord vertical = horizontal;
    vertical.id = 51;
    vertical.geometry = VpLineEntity{{0.0, 0.0}, {0.0, 20.0}};
    VpEntityRecord first_result;
    VpEntityRecord second_result;
    VpEntityRecord chamfer_result;
    QVERIFY(chamferedLineEntities(horizontal, vertical, {15.0, 0.0}, {0.0, 15.0}, 4.0, 6.0,
                                  first_result, second_result, chamfer_result));
    const auto& first_line = std::get<VpLineEntity>(first_result.geometry);
    const auto& second_line = std::get<VpLineEntity>(second_result.geometry);
    const auto& chamfer_line = std::get<VpLineEntity>(chamfer_result.geometry);
    QCOMPARE(first_line.end_point.x, 4.0);
    QCOMPARE(second_line.end_point.y, 6.0);
    QCOMPARE(chamfer_line.start_point.x, 4.0);
    QCOMPARE(chamfer_line.start_point.y, 0.0);
    QCOMPARE(chamfer_line.end_point.x, 0.0);
    QCOMPARE(chamfer_line.end_point.y, 6.0);
    QCOMPARE(chamfer_result.layer_name, horizontal.layer_name);
    QCOMPARE(chamfer_result.line_width_mm, horizontal.line_width_mm);

    VpCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("chamfer lines"));
    transaction->addLine({0.0, 0.0}, {20.0, 0.0});
    transaction->addLine({0.0, 0.0}, {0.0, 20.0});
    transaction->commit();
    VpCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setChamferDistances(3.0, 4.0);
    viewport.setToolMode(VpToolMode::Chamfer);
    viewport.submitWorldPoint({10.0, 0.0});
    viewport.submitWorldPoint({0.0, 10.0});
    QCOMPARE(document.entities().size(), std::size_t(3));
    QCOMPARE(document.entities().back().type, VpEntityType::Line);
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(2));
    QCOMPARE(std::get<VpLineEntity>(document.entities()[0].geometry).end_point.x, 20.0);
    QCOMPARE(std::get<VpLineEntity>(document.entities()[1].geometry).end_point.y, 20.0);
}

void VpCadViewportTest::objectSnapGeometry()
{
    VpEntityRecord horizontal;
    horizontal.type = VpEntityType::Line;
    horizontal.geometry = VpLineEntity{{-10.0, 0.0}, {10.0, 0.0}};
    VpEntityRecord vertical;
    vertical.type = VpEntityType::Line;
    vertical.geometry = VpLineEntity{{0.0, -10.0}, {0.0, 10.0}};
    std::vector<const VpEntityRecord*> lines{&horizontal, &vertical};

    const auto intersection = findObjectSnap(lines, {0.2, 0.1}, std::nullopt, 1.0);
    QVERIFY(intersection.has_value());
    QCOMPARE(intersection->type, VpObjectSnapType::Intersection);
    QVERIFY(std::abs(intersection->point.x) < 1.0e-9);
    QVERIFY(std::abs(intersection->point.y) < 1.0e-9);

    const auto perpendicular = findObjectSnap({&horizontal}, {2.0, 0.1}, VpPoint2d{2.0, 4.0}, 0.5);
    QVERIFY(perpendicular.has_value());
    QCOMPARE(perpendicular->type, VpObjectSnapType::Perpendicular);
    QCOMPARE(perpendicular->point.x, 2.0);

    VpEntityRecord circle;
    circle.type = VpEntityType::Circle;
    circle.geometry = VpCircleEntity{{0.0, 0.0}, 5.0};
    const auto quadrant = findObjectSnap({&circle}, {5.1, 0.0}, std::nullopt, 0.5);
    QVERIFY(quadrant.has_value());
    QCOMPARE(quadrant->type, VpObjectSnapType::Quadrant);

    const auto tangent = findObjectSnap({&circle}, {2.5, 4.33}, VpPoint2d{10.0, 0.0}, 0.5);
    QVERIFY(tangent.has_value());
    QCOMPARE(tangent->type, VpObjectSnapType::Tangent);
}

} // namespace Vp

VECTORPATH_TEST_ENTRY(Vp::VpCadViewportTest, vpRunVpCadViewportTest)
#include "vp_cad_viewport_test.moc"
