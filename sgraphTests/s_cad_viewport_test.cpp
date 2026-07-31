#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_cad_viewport_geometry.h"
#include "s_document_transaction.h"
#include "s_object_snap.h"

#include <QtTest>
#include <algorithm>
#include <cmath>

namespace smartCam
{

class SCadViewportTest final : public QObject
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

void SCadViewportTest::windowAndCrossingSelection()
{
    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("selection geometry"));
    transaction->addLine({0.0, 0.0}, {10.0, 0.0});
    transaction->addLine({5.0, -5.0}, {5.0, 5.0});
    transaction->commit();

    SCadViewport viewport;
    viewport.setDocument(&document);
    viewport.selectEntitiesInWindow({-1.0, -1.0}, {11.0, 1.0}, false);
    QCOMPARE(viewport.selectedEntityIds().size(), 1);

    viewport.selectEntitiesInWindow({-1.0, -1.0}, {11.0, 1.0}, true);
    QCOMPARE(viewport.selectedEntityIds().size(), 2);
}

void SCadViewportTest::selectsHatchByInteriorClick()
{
    SCadDocument document;
    auto transaction =
        document.beginTransaction(QStringLiteral("hatch click selection"));
    SHatchEntity hatch;
    hatch.boundary = {
        {-20.0, -20.0}, {20.0, -20.0}, {20.0, 20.0}, {-20.0, 20.0},
    };
    transaction->addHatch(std::move(hatch));
    transaction->commit();

    SCadViewport viewport;
    viewport.resize(640, 480);
    viewport.setDocument(&document);
    viewport.show();
    QCoreApplication::processEvents();
    QTest::mouseClick(&viewport, Qt::LeftButton, Qt::NoModifier,
                      viewport.rect().center());
    QCOMPARE(viewport.selectedEntityIds().size(), 1);
    QCOMPARE(viewport.selectedEntityId().value(),
             document.entities().front().id);
}

void SCadViewportTest::selectAllAndDelete()
{
    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("delete geometry"));
    transaction->addCircle({0.0, 0.0}, 4.0);
    transaction->addCircle({20.0, 0.0}, 3.0);
    transaction->commit();

    SCadViewport viewport;
    viewport.setDocument(&document);
    viewport.selectAll();
    QCOMPARE(viewport.selectedEntityIds().size(), 2);
    viewport.deleteSelected();
    QVERIFY(document.entities().empty());
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(2));
}

void SCadViewportTest::threePointArcGeometry()
{
    SPoint2d center;
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

void SCadViewportTest::modificationPreviewGeometry()
{
    SEntityRecord source_line;
    source_line.id = 1;
    source_line.type = SEntityType::Line;
    source_line.geometry = SLineEntity{{0.0, 0.0}, {10.0, 0.0}};

    const SEntityRecord moved_line = translatedEntity(source_line, 3.0, 4.0);
    QCOMPARE(std::get<SLineEntity>(moved_line.geometry).start_point.x, 3.0);
    QCOMPARE(std::get<SLineEntity>(moved_line.geometry).start_point.y, 4.0);

    const SEntityRecord rotated_line = rotatedEntity(source_line, {0.0, 0.0}, 90.0);
    QVERIFY(std::abs(std::get<SLineEntity>(rotated_line.geometry).end_point.x) < 1.0e-9);
    QVERIFY(std::abs(std::get<SLineEntity>(rotated_line.geometry).end_point.y - 10.0) < 1.0e-9);

    const SEntityRecord scaled_line = scaledEntity(source_line, {0.0, 0.0}, 2.5);
    QCOMPARE(std::get<SLineEntity>(scaled_line.geometry).end_point.x, 25.0);

    const SEntityRecord mirrored_line = mirroredEntity(source_line, {0.0, -1.0}, {0.0, 1.0});
    QCOMPARE(std::get<SLineEntity>(mirrored_line.geometry).start_point.x, 0.0);
    QCOMPARE(std::get<SLineEntity>(mirrored_line.geometry).end_point.x, -10.0);

    SEntityRecord offset_line;
    QVERIFY(offsetEntity(source_line, {5.0, 4.0}, offset_line));
    QCOMPARE(std::get<SLineEntity>(offset_line.geometry).start_point.y, 4.0);
    QCOMPARE(std::get<SLineEntity>(offset_line.geometry).end_point.y, 4.0);

    SEntityRecord cutting_line;
    cutting_line.id = 2;
    cutting_line.type = SEntityType::Line;
    cutting_line.geometry = SLineEntity{{5.0, -5.0}, {5.0, 5.0}};
    SEntityRecord trimmed_line;
    QVERIFY(trimmedLineEntity(cutting_line, source_line, {1.0, 0.0}, trimmed_line));
    QCOMPARE(std::get<SLineEntity>(trimmed_line.geometry).start_point.x, 5.0);
    QCOMPARE(std::get<SLineEntity>(trimmed_line.geometry).end_point.x, 10.0);

    SEntityRecord boundary_line;
    boundary_line.id = 3;
    boundary_line.type = SEntityType::Line;
    boundary_line.geometry = SLineEntity{{10.0, -5.0}, {10.0, 5.0}};
    SEntityRecord short_line;
    short_line.id = 4;
    short_line.type = SEntityType::Line;
    short_line.geometry = SLineEntity{{0.0, 0.0}, {4.0, 0.0}};
    SEntityRecord extended_line;
    QVERIFY(extendedLineEntity(boundary_line, short_line, extended_line));
    QCOMPARE(std::get<SLineEntity>(extended_line.geometry).start_point.x, 0.0);
    QCOMPARE(std::get<SLineEntity>(extended_line.geometry).end_point.x, 10.0);
    QVERIFY(!extendedLineEntity(boundary_line, source_line, extended_line));

    const std::vector<SEntityRecord> broken_lines =
        brokenLineEntities(source_line, {3.0, 0.0}, {7.0, 0.0});
    QCOMPARE(broken_lines.size(), std::size_t(2));
    QCOMPARE(std::get<SLineEntity>(broken_lines[0].geometry).end_point.x, 3.0);
    QCOMPARE(std::get<SLineEntity>(broken_lines[1].geometry).start_point.x, 7.0);
    const std::vector<SEntityRecord> split_lines =
        brokenLineEntities(source_line, {5.0, 0.0}, {5.0, 0.0});
    QCOMPARE(split_lines.size(), std::size_t(2));
    QCOMPARE(std::get<SLineEntity>(split_lines[0].geometry).end_point.x, 5.0);
    QCOMPARE(std::get<SLineEntity>(split_lines[1].geometry).start_point.x, 5.0);
    QVERIFY(brokenLineEntities(source_line, {0.0, 0.0}, {0.0, 0.0}).empty());

    SEntityRecord join_first = source_line;
    join_first.id = 10;
    join_first.geometry = SLineEntity{{0.0, 0.0}, {5.0, 0.0}};
    SEntityRecord join_second = source_line;
    join_second.id = 11;
    join_second.geometry = SLineEntity{{10.0, 0.0}, {5.0, 0.0}};
    SEntityRecord join_third = source_line;
    join_third.id = 12;
    join_third.geometry = SLineEntity{{10.0, 0.0}, {10.0, 5.0}};
    SEntityRecord joined_entity;
    QVERIFY(joinedLineEntity({join_first, join_third, join_second}, 1.0e-6, joined_entity));
    QCOMPARE(joined_entity.type, SEntityType::Polyline);
    const auto& joined_polyline = std::get<SPolylineEntity>(joined_entity.geometry);
    QCOMPARE(joined_polyline.vertices.size(), std::size_t(4));
    QVERIFY(!joined_polyline.is_closed);

    SEntityRecord triangle_second = source_line;
    triangle_second.id = 13;
    triangle_second.geometry = SLineEntity{{0.0, 5.0}, {0.0, 0.0}};
    SEntityRecord triangle_third = source_line;
    triangle_third.id = 14;
    triangle_third.geometry = SLineEntity{{5.0, 0.0}, {0.0, 5.0}};
    QVERIFY(joinedLineEntity({join_first, triangle_second, triangle_third}, 1.0e-6, joined_entity));
    const auto& closed_polyline = std::get<SPolylineEntity>(joined_entity.geometry);
    QCOMPARE(closed_polyline.vertices.size(), std::size_t(3));
    QVERIFY(closed_polyline.is_closed);
    QVERIFY(!joinedLineEntity({join_first, boundary_line}, 1.0e-6, joined_entity));
}

void SCadViewportTest::curveTrimGeometryAndUndo()
{
    SEntityRecord cutting_line;
    cutting_line.id = 90;
    cutting_line.type = SEntityType::Line;
    cutting_line.geometry = SLineEntity{{0.0, -10.0}, {0.0, 10.0}};
    SEntityRecord circle;
    circle.id = 91;
    circle.type = SEntityType::Circle;
    circle.layer_name = QStringLiteral("curves");
    circle.line_width_mm = 0.5;
    circle.geometry = SCircleEntity{{0.0, 0.0}, 5.0};
    const std::vector<SEntityRecord> circle_parts =
        trimmedEntityParts(cutting_line, circle, {5.0, 0.0});
    QCOMPARE(circle_parts.size(), std::size_t(1));
    QCOMPARE(circle_parts.front().type, SEntityType::Arc);
    const auto& remaining_arc = std::get<SArcEntity>(circle_parts.front().geometry);
    QCOMPARE(remaining_arc.start_angle, 90.0);
    QCOMPARE(remaining_arc.end_angle, 270.0);
    QCOMPARE(circle_parts.front().layer_name, circle.layer_name);

    SEntityRecord target_line = cutting_line;
    target_line.id = 92;
    target_line.geometry = SLineEntity{{-10.0, 0.0}, {10.0, 0.0}};
    const std::vector<SEntityRecord> line_parts =
        trimmedEntityParts(circle, target_line, {0.0, 0.0});
    QCOMPARE(line_parts.size(), std::size_t(2));
    QCOMPARE(std::get<SLineEntity>(line_parts[0].geometry).end_point.x, -5.0);
    QCOMPARE(std::get<SLineEntity>(line_parts[1].geometry).start_point.x, 5.0);

    SEntityRecord target_arc = circle;
    target_arc.id = 93;
    target_arc.type = SEntityType::Arc;
    target_arc.geometry = SArcEntity{{0.0, 0.0}, 5.0, 0.0, 180.0};
    const std::vector<SEntityRecord> arc_parts =
        trimmedEntityParts(cutting_line, target_arc, {3.5, 3.5});
    QCOMPARE(arc_parts.size(), std::size_t(1));
    QCOMPARE(std::get<SArcEntity>(arc_parts[0].geometry).start_angle, 90.0);
    QCOMPARE(std::get<SArcEntity>(arc_parts[0].geometry).end_angle, 180.0);

    SEntityRecord target_polyline;
    target_polyline.id = 94;
    target_polyline.type = SEntityType::Polyline;
    target_polyline.layer_name = QStringLiteral("curves");
    target_polyline.geometry = SPolylineEntity{{{-10.0, 0.0}, {-5.0, 0.0}, {10.0, 0.0}}, false};
    const std::vector<SEntityRecord> polyline_parts =
        trimmedEntityParts(cutting_line, target_polyline, {-2.0, 0.0});
    QCOMPARE(polyline_parts.size(), std::size_t(2));
    QCOMPARE(std::get<SPolylineEntity>(polyline_parts[0].geometry).vertices.back().x, -5.0);
    QCOMPARE(std::get<SPolylineEntity>(polyline_parts[1].geometry).vertices.front().x, 0.0);
    QCOMPARE(polyline_parts[1].layer_name, target_polyline.layer_name);

    target_polyline.id = 95;
    target_polyline.geometry =
        SPolylineEntity{{{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {0.0, 10.0}}, true};
    SEntityRecord short_cutting_line = cutting_line;
    short_cutting_line.geometry = SLineEntity{{5.0, -2.0}, {5.0, 2.0}};
    const std::vector<SEntityRecord> closed_polyline_parts =
        trimmedEntityParts(short_cutting_line, target_polyline, {2.0, 0.0});
    QCOMPARE(closed_polyline_parts.size(), std::size_t(1));
    const auto& opened_polyline = std::get<SPolylineEntity>(closed_polyline_parts.front().geometry);
    QVERIFY(!opened_polyline.is_closed);
    QCOMPARE(opened_polyline.vertices.front().x, 5.0);
    QCOMPARE(opened_polyline.vertices.back().x, 0.0);

    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("curve trim"));
    transaction->addLine({0.0, -10.0}, {0.0, 10.0});
    transaction->addCircle({0.0, 0.0}, 5.0);
    transaction->commit();
    SCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setToolMode(SToolMode::Trim);
    viewport.submitWorldPoint({0.0, 8.0});
    viewport.submitWorldPoint({5.0, 0.0});
    QCOMPARE(document.entities().size(), std::size_t(2));
    QVERIFY(std::any_of(document.entities().begin(), document.entities().end(),
                        [](const SEntityRecord& entity)
                        {
                            return entity.type == SEntityType::Arc;
                        }));
    document.undo();
    QVERIFY(std::any_of(document.entities().begin(), document.entities().end(),
                        [](const SEntityRecord& entity)
                        {
                            return entity.type == SEntityType::Circle;
                        }));
}

void SCadViewportTest::curveExtendGeometryAndUndo()
{
    SEntityRecord circle_boundary;
    circle_boundary.id = 100;
    circle_boundary.type = SEntityType::Circle;
    circle_boundary.geometry = SCircleEntity{{0.0, 0.0}, 10.0};
    SEntityRecord target_line;
    target_line.id = 101;
    target_line.type = SEntityType::Line;
    target_line.layer_name = QStringLiteral("extend_curves");
    target_line.line_width_mm = 0.5;
    target_line.geometry = SLineEntity{{0.0, 0.0}, {5.0, 0.0}};
    SEntityRecord extended;
    QVERIFY(extendedEntity(circle_boundary, target_line, {5.0, 0.0}, extended));
    QCOMPARE(std::get<SLineEntity>(extended.geometry).end_point.x, 10.0);
    QCOMPARE(extended.layer_name, target_line.layer_name);

    SEntityRecord line_boundary;
    line_boundary.id = 102;
    line_boundary.type = SEntityType::Line;
    line_boundary.geometry = SLineEntity{{-10.0, 0.0}, {0.0, 0.0}};
    SEntityRecord target_arc;
    target_arc.id = 103;
    target_arc.type = SEntityType::Arc;
    target_arc.geometry = SArcEntity{{0.0, 0.0}, 5.0, 0.0, 90.0};
    QVERIFY(extendedEntity(line_boundary, target_arc, {0.0, 5.0}, extended));
    QCOMPARE(std::get<SArcEntity>(extended.geometry).start_angle, 0.0);
    QCOMPARE(std::get<SArcEntity>(extended.geometry).end_angle, 180.0);

    SEntityRecord target_polyline;
    target_polyline.id = 104;
    target_polyline.type = SEntityType::Polyline;
    target_polyline.layer_name = QStringLiteral("extend_curves");
    target_polyline.geometry = SPolylineEntity{{{0.0, 0.0}, {3.0, 0.0}, {5.0, 0.0}}, false};
    QVERIFY(extendedEntity(circle_boundary, target_polyline, {5.0, 0.0}, extended));
    QCOMPARE(std::get<SPolylineEntity>(extended.geometry).vertices.back().x, 10.0);
    QCOMPARE(extended.layer_name, target_polyline.layer_name);
    target_polyline.geometry = SPolylineEntity{{{0.0, 0.0}, {3.0, 0.0}, {5.0, 0.0}}, true};
    QVERIFY(!extendedEntity(circle_boundary, target_polyline, {5.0, 0.0}, extended));

    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("curve extend"));
    transaction->addCircle({0.0, 0.0}, 10.0);
    const SEntityId line_id = transaction->addLine({0.0, 0.0}, {5.0, 0.0});
    transaction->commit();
    SCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setToolMode(SToolMode::Extend);
    viewport.submitWorldPoint({0.0, 10.0});
    viewport.submitWorldPoint({5.0, 0.0});
    const auto extended_iterator =
        std::find_if(document.entities().begin(), document.entities().end(),
                     [line_id](const SEntityRecord& entity)
                     {
                         return entity.id == line_id;
                     });
    QVERIFY(extended_iterator != document.entities().end());
    QCOMPARE(std::get<SLineEntity>(extended_iterator->geometry).end_point.x, 10.0);
    document.undo();
    const auto restored_iterator =
        std::find_if(document.entities().begin(), document.entities().end(),
                     [line_id](const SEntityRecord& entity)
                     {
                         return entity.id == line_id;
                     });
    QVERIFY(restored_iterator != document.entities().end());
    QCOMPARE(std::get<SLineEntity>(restored_iterator->geometry).end_point.x, 5.0);
}

void SCadViewportTest::curveBreakGeometryAndUndo()
{
    SEntityRecord circle;
    circle.id = 110;
    circle.type = SEntityType::Circle;
    circle.layer_name = QStringLiteral("break_curves");
    circle.line_width_mm = 0.5;
    circle.geometry = SCircleEntity{{0.0, 0.0}, 5.0};
    const std::vector<SEntityRecord> circle_parts =
        brokenEntityParts(circle, {5.0, 0.0}, {0.0, 5.0});
    QCOMPARE(circle_parts.size(), std::size_t(1));
    QCOMPARE(circle_parts.front().type, SEntityType::Arc);
    const auto& circle_remainder = std::get<SArcEntity>(circle_parts.front().geometry);
    QCOMPARE(circle_remainder.start_angle, 90.0);
    QCOMPARE(circle_remainder.end_angle, 0.0);
    QCOMPARE(circle_parts.front().layer_name, circle.layer_name);

    SEntityRecord arc = circle;
    arc.id = 111;
    arc.type = SEntityType::Arc;
    arc.geometry = SArcEntity{{0.0, 0.0}, 5.0, 0.0, 180.0};
    const double diagonal = 5.0 / std::sqrt(2.0);
    const std::vector<SEntityRecord> arc_parts =
        brokenEntityParts(arc, {diagonal, diagonal}, {-diagonal, diagonal});
    QCOMPARE(arc_parts.size(), std::size_t(2));
    QCOMPARE(std::get<SArcEntity>(arc_parts[0].geometry).start_angle, 0.0);
    QCOMPARE(std::get<SArcEntity>(arc_parts[0].geometry).end_angle, 45.0);
    QCOMPARE(std::get<SArcEntity>(arc_parts[1].geometry).start_angle, 135.0);
    QCOMPARE(std::get<SArcEntity>(arc_parts[1].geometry).end_angle, 180.0);

    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("curve break"));
    transaction->addCircle({0.0, 0.0}, 5.0);
    transaction->commit();
    SCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setToolMode(SToolMode::Break);
    viewport.submitWorldPoint({5.0, 0.0});
    viewport.submitWorldPoint({0.0, 5.0});
    QCOMPARE(document.entities().size(), std::size_t(1));
    QCOMPARE(document.entities().front().type, SEntityType::Arc);
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(1));
    QCOMPARE(document.entities().front().type, SEntityType::Circle);
}

void SCadViewportTest::curveJoinGeometryAndUndo()
{
    SEntityRecord first_arc;
    first_arc.id = 120;
    first_arc.type = SEntityType::Arc;
    first_arc.layer_name = QStringLiteral("join_curves");
    first_arc.line_width_mm = 0.5;
    first_arc.geometry = SArcEntity{{0.0, 0.0}, 5.0, 0.0, 90.0};
    SEntityRecord second_arc = first_arc;
    second_arc.id = 121;
    second_arc.geometry = SArcEntity{{0.0, 0.0}, 5.0, 90.0, 180.0};
    SEntityRecord joined;
    QVERIFY(joinedEntity({second_arc, first_arc}, 1.0e-6, joined));
    QCOMPARE(joined.type, SEntityType::Arc);
    QCOMPARE(std::get<SArcEntity>(joined.geometry).start_angle, 0.0);
    QCOMPARE(std::get<SArcEntity>(joined.geometry).end_angle, 180.0);
    QCOMPARE(joined.layer_name, first_arc.layer_name);

    SEntityRecord third_arc = first_arc;
    third_arc.id = 122;
    third_arc.geometry = SArcEntity{{0.0, 0.0}, 5.0, 180.0, 270.0};
    SEntityRecord fourth_arc = first_arc;
    fourth_arc.id = 123;
    fourth_arc.geometry = SArcEntity{{0.0, 0.0}, 5.0, 270.0, 0.0};
    QVERIFY(joinedEntity({third_arc, first_arc, fourth_arc, second_arc}, 1.0e-6, joined));
    QCOMPARE(joined.type, SEntityType::Circle);

    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("curve join"));
    transaction->addArc({0.0, 0.0}, 5.0, 0.0, 90.0);
    transaction->addArc({0.0, 0.0}, 5.0, 90.0, 180.0);
    transaction->commit();
    SCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setToolMode(SToolMode::Join);
    const double diagonal = 5.0 / std::sqrt(2.0);
    viewport.submitWorldPoint({diagonal, diagonal});
    viewport.submitWorldPoint({-diagonal, diagonal});
    QTest::keyClick(&viewport, Qt::Key_Return);
    QCOMPARE(document.entities().size(), std::size_t(1));
    QCOMPARE(document.entities().front().type, SEntityType::Arc);
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(2));

    SEntityRecord source_polyline;
    source_polyline.id = 124;
    source_polyline.type = SEntityType::Polyline;
    source_polyline.layer_name = QStringLiteral("join_linear");
    source_polyline.geometry = SPolylineEntity{{{0.0, 0.0}, {5.0, 0.0}, {10.0, 0.0}}, false};
    SEntityRecord appended_line;
    appended_line.id = 125;
    appended_line.type = SEntityType::Line;
    appended_line.geometry = SLineEntity{{10.0, 0.0}, {15.0, 0.0}};
    QVERIFY(joinedEntity({appended_line, source_polyline}, 1.0e-6, joined));
    QCOMPARE(joined.type, SEntityType::Polyline);
    const auto& joined_linear = std::get<SPolylineEntity>(joined.geometry);
    QCOMPARE(joined_linear.vertices.size(), std::size_t(4));
    QCOMPARE(joined_linear.vertices.front().x, 0.0);
    QCOMPARE(joined_linear.vertices.back().x, 15.0);

    SCadDocument linear_document;
    auto linear_transaction = linear_document.beginTransaction(QStringLiteral("polyline join"));
    linear_transaction->addPolyline({{0.0, 0.0}, {5.0, 0.0}, {10.0, 0.0}}, false);
    linear_transaction->addLine({10.0, 0.0}, {15.0, 0.0});
    linear_transaction->commit();
    SCadViewport linear_viewport;
    linear_viewport.setDocument(&linear_document);
    linear_viewport.setToolMode(SToolMode::Join);
    linear_viewport.submitWorldPoint({2.5, 0.0});
    linear_viewport.submitWorldPoint({12.5, 0.0});
    QTest::keyClick(&linear_viewport, Qt::Key_Return);
    QCOMPARE(linear_document.entities().size(), std::size_t(1));
    QCOMPARE(linear_document.entities().front().type, SEntityType::Polyline);
    linear_document.undo();
    QCOMPARE(linear_document.entities().size(), std::size_t(2));
}

void SCadViewportTest::explodeGeometry()
{
    SEntityRecord open_polyline;
    open_polyline.id = 20;
    open_polyline.type = SEntityType::Polyline;
    open_polyline.layer_name = QStringLiteral("construction");
    open_polyline.line_width_mm = 0.5;
    open_polyline.geometry = SPolylineEntity{{{0.0, 0.0}, {5.0, 0.0}, {5.0, 4.0}}, false};

    const std::vector<SEntityRecord> open_parts = explodedEntityParts(open_polyline);
    QCOMPARE(open_parts.size(), std::size_t(2));
    QCOMPARE(open_parts[0].type, SEntityType::Line);
    QCOMPARE(open_parts[0].layer_name, QStringLiteral("construction"));
    QCOMPARE(open_parts[0].line_width_mm, 0.5);
    QCOMPARE(std::get<SLineEntity>(open_parts[0].geometry).start_point.x, 0.0);
    QCOMPARE(std::get<SLineEntity>(open_parts[1].geometry).end_point.y, 4.0);

    SEntityRecord closed_polyline = open_polyline;
    closed_polyline.id = 21;
    closed_polyline.geometry = SPolylineEntity{{{0.0, 0.0}, {5.0, 0.0}, {0.0, 4.0}}, true};
    const std::vector<SEntityRecord> closed_parts = explodedEntityParts(closed_polyline);
    QCOMPARE(closed_parts.size(), std::size_t(3));
    QCOMPARE(std::get<SLineEntity>(closed_parts.back().geometry).start_point.y, 4.0);
    QCOMPARE(std::get<SLineEntity>(closed_parts.back().geometry).end_point.x, 0.0);
    QCOMPARE(std::get<SLineEntity>(closed_parts.back().geometry).end_point.y, 0.0);

    SEntityRecord hatch = open_polyline;
    hatch.id = 22;
    hatch.type = SEntityType::Hatch;
    hatch.geometry = SHatchEntity{{{1.0, 1.0}, {6.0, 1.0}, {1.0, 5.0}}};
    const std::vector<SEntityRecord> hatch_parts = explodedEntityParts(hatch);
    QCOMPARE(hatch_parts.size(), std::size_t(3));
    for (const SEntityRecord& hatch_part : hatch_parts)
    {
        QCOMPARE(hatch_part.type, SEntityType::Line);
        QCOMPARE(hatch_part.layer_name, hatch.layer_name);
        QCOMPARE(hatch_part.line_width_mm, hatch.line_width_mm);
    }

    SEntityRecord line = open_polyline;
    line.type = SEntityType::Line;
    line.geometry = SLineEntity{{0.0, 0.0}, {1.0, 1.0}};
    QVERIFY(explodedEntityParts(line).empty());
}

void SCadViewportTest::stretchGeometryAndUndo()
{
    SEntityRecord source;
    source.id = 30;
    source.type = SEntityType::Polyline;
    source.layer_name = QStringLiteral("detail");
    source.line_width_mm = 0.7;
    source.geometry = SPolylineEntity{{{0.0, 0.0}, {5.0, 0.0}, {10.0, 0.0}}, false};
    SEntityRecord stretched;
    QVERIFY(stretchedEntity(source, {4.0, -1.0}, {11.0, 1.0}, 0.0, 3.0, stretched));
    const auto& stretched_polyline = std::get<SPolylineEntity>(stretched.geometry);
    QCOMPARE(stretched_polyline.vertices[0].y, 0.0);
    QCOMPARE(stretched_polyline.vertices[1].y, 3.0);
    QCOMPARE(stretched_polyline.vertices[2].y, 3.0);
    QCOMPARE(stretched.layer_name, source.layer_name);
    QCOMPARE(stretched.line_width_mm, source.line_width_mm);
    QVERIFY(!stretchedEntity(source, {20.0, 20.0}, {30.0, 30.0}, 1.0, 1.0, stretched));

    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("stretch line"));
    transaction->addLine({0.0, 0.0}, {10.0, 0.0});
    transaction->commit();
    SCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setToolMode(SToolMode::Stretch);
    viewport.submitWorldPoint({8.0, -2.0});
    viewport.submitWorldPoint({12.0, 2.0});
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({0.0, 5.0});

    QCOMPARE(document.entities().size(), std::size_t(1));
    const auto& stretched_line = std::get<SLineEntity>(document.entities().front().geometry);
    QCOMPARE(stretched_line.start_point.y, 0.0);
    QCOMPARE(stretched_line.end_point.x, 10.0);
    QCOMPARE(stretched_line.end_point.y, 5.0);
    document.undo();
    const auto& original_line = std::get<SLineEntity>(document.entities().front().geometry);
    QCOMPARE(original_line.start_point.y, 0.0);
    QCOMPARE(original_line.end_point.y, 0.0);
}

void SCadViewportTest::filletGeometryAndUndo()
{
    SEntityRecord horizontal;
    horizontal.id = 40;
    horizontal.type = SEntityType::Line;
    horizontal.layer_name = QStringLiteral("profile");
    horizontal.line_width_mm = 0.35;
    horizontal.geometry = SLineEntity{{0.0, 0.0}, {20.0, 0.0}};
    SEntityRecord vertical = horizontal;
    vertical.id = 41;
    vertical.geometry = SLineEntity{{0.0, 0.0}, {0.0, 20.0}};
    SEntityRecord first_result;
    SEntityRecord second_result;
    SEntityRecord arc_result;
    QVERIFY(filletedLineEntities(horizontal, vertical, {15.0, 0.0}, {0.0, 15.0}, 5.0, first_result,
                                 second_result, arc_result));
    const auto& first_line = std::get<SLineEntity>(first_result.geometry);
    const auto& second_line = std::get<SLineEntity>(second_result.geometry);
    const auto& fillet_arc = std::get<SArcEntity>(arc_result.geometry);
    QCOMPARE(first_line.end_point.x, 5.0);
    QCOMPARE(first_line.end_point.y, 0.0);
    QCOMPARE(second_line.end_point.x, 0.0);
    QCOMPARE(second_line.end_point.y, 5.0);
    QCOMPARE(fillet_arc.center.x, 5.0);
    QCOMPARE(fillet_arc.center.y, 5.0);
    QCOMPARE(fillet_arc.radius, 5.0);
    QCOMPARE(arc_result.layer_name, horizontal.layer_name);
    QCOMPARE(arc_result.line_width_mm, horizontal.line_width_mm);

    SEntityRecord parallel = horizontal;
    parallel.id = 42;
    parallel.geometry = SLineEntity{{0.0, 10.0}, {20.0, 10.0}};
    QVERIFY(!filletedLineEntities(horizontal, parallel, {15.0, 0.0}, {15.0, 10.0}, 5.0,
                                  first_result, second_result, arc_result));

    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("fillet lines"));
    transaction->addLine({0.0, 0.0}, {20.0, 0.0});
    transaction->addLine({0.0, 0.0}, {0.0, 20.0});
    transaction->commit();
    SCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setFilletRadius(2.0);
    viewport.setToolMode(SToolMode::Fillet);
    viewport.submitWorldPoint({10.0, 0.0});
    viewport.submitWorldPoint({0.0, 10.0});
    QCOMPARE(document.entities().size(), std::size_t(3));
    QCOMPARE(document.entities().back().type, SEntityType::Arc);
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(2));
    QCOMPARE(std::get<SLineEntity>(document.entities()[0].geometry).end_point.x, 20.0);
    QCOMPARE(std::get<SLineEntity>(document.entities()[1].geometry).end_point.y, 20.0);
}

void SCadViewportTest::chamferGeometryAndUndo()
{
    SEntityRecord horizontal;
    horizontal.id = 50;
    horizontal.type = SEntityType::Line;
    horizontal.layer_name = QStringLiteral("outline");
    horizontal.line_width_mm = 0.5;
    horizontal.geometry = SLineEntity{{0.0, 0.0}, {20.0, 0.0}};
    SEntityRecord vertical = horizontal;
    vertical.id = 51;
    vertical.geometry = SLineEntity{{0.0, 0.0}, {0.0, 20.0}};
    SEntityRecord first_result;
    SEntityRecord second_result;
    SEntityRecord chamfer_result;
    QVERIFY(chamferedLineEntities(horizontal, vertical, {15.0, 0.0}, {0.0, 15.0}, 4.0, 6.0,
                                  first_result, second_result, chamfer_result));
    const auto& first_line = std::get<SLineEntity>(first_result.geometry);
    const auto& second_line = std::get<SLineEntity>(second_result.geometry);
    const auto& chamfer_line = std::get<SLineEntity>(chamfer_result.geometry);
    QCOMPARE(first_line.end_point.x, 4.0);
    QCOMPARE(second_line.end_point.y, 6.0);
    QCOMPARE(chamfer_line.start_point.x, 4.0);
    QCOMPARE(chamfer_line.start_point.y, 0.0);
    QCOMPARE(chamfer_line.end_point.x, 0.0);
    QCOMPARE(chamfer_line.end_point.y, 6.0);
    QCOMPARE(chamfer_result.layer_name, horizontal.layer_name);
    QCOMPARE(chamfer_result.line_width_mm, horizontal.line_width_mm);

    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("chamfer lines"));
    transaction->addLine({0.0, 0.0}, {20.0, 0.0});
    transaction->addLine({0.0, 0.0}, {0.0, 20.0});
    transaction->commit();
    SCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setChamferDistances(3.0, 4.0);
    viewport.setToolMode(SToolMode::Chamfer);
    viewport.submitWorldPoint({10.0, 0.0});
    viewport.submitWorldPoint({0.0, 10.0});
    QCOMPARE(document.entities().size(), std::size_t(3));
    QCOMPARE(document.entities().back().type, SEntityType::Line);
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(2));
    QCOMPARE(std::get<SLineEntity>(document.entities()[0].geometry).end_point.x, 20.0);
    QCOMPARE(std::get<SLineEntity>(document.entities()[1].geometry).end_point.y, 20.0);
}

void SCadViewportTest::objectSnapGeometry()
{
    SEntityRecord horizontal;
    horizontal.type = SEntityType::Line;
    horizontal.geometry = SLineEntity{{-10.0, 0.0}, {10.0, 0.0}};
    SEntityRecord vertical;
    vertical.type = SEntityType::Line;
    vertical.geometry = SLineEntity{{0.0, -10.0}, {0.0, 10.0}};
    std::vector<const SEntityRecord*> lines{&horizontal, &vertical};

    const auto intersection = findObjectSnap(lines, {0.2, 0.1}, std::nullopt, 1.0);
    QVERIFY(intersection.has_value());
    QCOMPARE(intersection->type, SObjectSnapType::Intersection);
    QVERIFY(std::abs(intersection->point.x) < 1.0e-9);
    QVERIFY(std::abs(intersection->point.y) < 1.0e-9);

    const auto perpendicular = findObjectSnap({&horizontal}, {2.0, 0.1}, SPoint2d{2.0, 4.0}, 0.5);
    QVERIFY(perpendicular.has_value());
    QCOMPARE(perpendicular->type, SObjectSnapType::Perpendicular);
    QCOMPARE(perpendicular->point.x, 2.0);

    SEntityRecord circle;
    circle.type = SEntityType::Circle;
    circle.geometry = SCircleEntity{{0.0, 0.0}, 5.0};
    const auto quadrant = findObjectSnap({&circle}, {5.1, 0.0}, std::nullopt, 0.5);
    QVERIFY(quadrant.has_value());
    QCOMPARE(quadrant->type, SObjectSnapType::Quadrant);

    const auto tangent = findObjectSnap({&circle}, {2.5, 4.33}, SPoint2d{10.0, 0.0}, 0.5);
    QVERIFY(tangent.has_value());
    QCOMPARE(tangent->type, SObjectSnapType::Tangent);
}

} // namespace smartCam

QTEST_MAIN(smartCam::SCadViewportTest)
#include "s_cad_viewport_test.moc"
