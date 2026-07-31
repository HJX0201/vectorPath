#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_cad_viewport_geometry.h"
#include "s_document_transaction.h"
#include "s_dxf_codec.h"
#include "s_object_snap.h"

#include <QTemporaryDir>
#include <QtTest>
#include <algorithm>

namespace vectorPath
{

class SPolylineCornerTest final : public QObject
{
    Q_OBJECT

  private slots:
    void polylineChamferGeometryAndUndo();
    void polylineFilletGeometryPersistenceAndUndo();
    void bulgedPolylineTrimPreservesCurves();
    void interactiveArcSegmentAndUndo();
    void variableWidthPersistenceAndDxf();
    void polylineEditOperationsAndJoin();
};

void SPolylineCornerTest::polylineChamferGeometryAndUndo()
{
    SEntityRecord open_polyline;
    open_polyline.id = 220;
    open_polyline.type = SEntityType::Polyline;
    open_polyline.layer_name = QStringLiteral("polyline_corner");
    open_polyline.line_width_mm = 0.5;
    open_polyline.geometry =
        SPolylineEntity{{{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {20.0, 10.0}}, false};
    SEntityRecord result;
    QVERIFY(chamferedPolylineEntity(open_polyline, 2.0, 3.0, result));
    const auto& open_result = std::get<SPolylineEntity>(result.geometry);
    QCOMPARE(open_result.vertices.size(), std::size_t(6));
    QVERIFY(!open_result.is_closed);
    QCOMPARE(open_result.vertices[1].x, 8.0);
    QCOMPARE(open_result.vertices[2].y, 3.0);
    QCOMPARE(result.layer_name, open_polyline.layer_name);

    SEntityRecord closed_polyline = open_polyline;
    closed_polyline.id = 221;
    closed_polyline.geometry =
        SPolylineEntity{{{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {0.0, 10.0}}, true};
    QVERIFY(chamferedPolylineEntity(closed_polyline, 2.0, 2.0, result));
    const auto& closed_result = std::get<SPolylineEntity>(result.geometry);
    QCOMPARE(closed_result.vertices.size(), std::size_t(8));
    QVERIFY(closed_result.is_closed);

    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("polyline chamfer"));
    transaction->addPolyline({{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {20.0, 10.0}}, false);
    transaction->commit();
    SCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setChamferDistances(2.0, 3.0);
    viewport.setToolMode(SToolMode::Chamfer);
    viewport.submitWorldPoint({5.0, 0.0});
    QCOMPARE(document.entities().size(), std::size_t(1));
    QCOMPARE(std::get<SPolylineEntity>(document.entities().front().geometry).vertices.size(),
             std::size_t(6));
    document.undo();
    QCOMPARE(std::get<SPolylineEntity>(document.entities().front().geometry).vertices.size(),
             std::size_t(4));
}

void SPolylineCornerTest::polylineFilletGeometryPersistenceAndUndo()
{
    SEntityRecord source;
    source.id = 230;
    source.type = SEntityType::Polyline;
    source.layer_name = QStringLiteral("polyline_fillet");
    source.line_width_mm = 0.5;
    source.geometry = SPolylineEntity{{{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {20.0, 10.0}}, false};
    SEntityRecord result;
    QVERIFY(filletedPolylineEntity(source, 2.0, result));
    const auto& filleted = std::get<SPolylineEntity>(result.geometry);
    QCOMPARE(filleted.vertices.size(), std::size_t(6));
    QCOMPARE(filleted.bulges.size(), filleted.vertices.size());
    QCOMPARE(std::count_if(filleted.bulges.begin(), filleted.bulges.end(),
                           [](double bulge)
                           {
                               return std::abs(bulge) > 1.0e-9;
                           }),
             2);
    SArcEntity first_arc;
    QVERIFY(bulgeArc(filleted.vertices[1], filleted.vertices[2], filleted.bulges[1], first_arc));
    QVERIFY(std::abs(first_arc.radius - 2.0) < 1.0e-9);
    QCOMPARE(result.layer_name, source.layer_name);
    const SPoint2d arc_midpoint{
        first_arc.center.x + (first_arc.radius * std::cos(315.0 * 3.14159265358979323846 / 180.0)),
        first_arc.center.y + (first_arc.radius * std::sin(315.0 * 3.14159265358979323846 / 180.0))};
    const auto midpoint_snap = findObjectSnap({&result}, arc_midpoint, std::nullopt, 0.1);
    QVERIFY(midpoint_snap.has_value());
    QCOMPARE(midpoint_snap->type, SObjectSnapType::Midpoint);

    SEntityRecord appended_line;
    appended_line.id = 231;
    appended_line.type = SEntityType::Line;
    appended_line.geometry = SLineEntity{{20.0, 10.0}, {30.0, 10.0}};
    SEntityRecord joined_result;
    QVERIFY(joinedEntity({appended_line, result}, 1.0e-6, joined_result));
    const auto& joined_polyline = std::get<SPolylineEntity>(joined_result.geometry);
    QCOMPARE(joined_polyline.vertices.back().x, 30.0);
    QCOMPARE(std::count_if(joined_polyline.bulges.begin(), joined_polyline.bulges.end(),
                           [](double bulge)
                           {
                               return std::abs(bulge) > 1.0e-9;
                           }),
             2);

    SEntityRecord bulged_path;
    bulged_path.id = 232;
    bulged_path.type = SEntityType::Polyline;
    bulged_path.geometry = SPolylineEntity{{{0.0, 0.0}, {5.0, 5.0}, {10.0, 5.0}},
                                           false,
                                           {std::tan(3.14159265358979323846 / 8.0), 0.0, 0.0}};
    SEntityRecord extension_boundary;
    extension_boundary.id = 233;
    extension_boundary.type = SEntityType::Line;
    extension_boundary.geometry = SLineEntity{{-5.0, 0.0}, {-5.0, 10.0}};
    SEntityRecord extended_path;
    QVERIFY(extendedEntity(extension_boundary, bulged_path, {0.0, 0.0}, extended_path));
    const auto& extended_polyline = std::get<SPolylineEntity>(extended_path.geometry);
    QVERIFY(std::abs(extended_polyline.vertices.front().x + 5.0) < 1.0e-9);
    QVERIFY(std::abs(extended_polyline.vertices.front().y - 5.0) < 1.0e-6);
    QVERIFY(std::abs(extended_polyline.bulges.front() - 1.0) < 1.0e-6);

    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("polyline fillet"));
    transaction->addPolyline({{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {20.0, 10.0}}, false);
    transaction->commit();
    SCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setFilletRadius(2.0);
    viewport.setToolMode(SToolMode::Fillet);
    viewport.submitWorldPoint({5.0, 0.0});
    const auto& stored = std::get<SPolylineEntity>(document.entities().front().geometry);
    QCOMPARE(stored.vertices.size(), std::size_t(6));
    QVERIFY(!stored.bulges.empty());

    QTemporaryDir temporary_directory;
    QVERIFY(temporary_directory.isValid());
    const QString file_path = temporary_directory.filePath(QStringLiteral("bulge.smartcad"));
    QVERIFY(document.save(file_path).isSuccess());
    SCadDocument loaded_document;
    QVERIFY(loaded_document.load(file_path).isSuccess());
    const auto& loaded = std::get<SPolylineEntity>(loaded_document.entities().front().geometry);
    QCOMPARE(loaded.bulges.size(), stored.bulges.size());
    QCOMPARE(loaded.bulges[1], stored.bulges[1]);

    const QString dxf_path = temporary_directory.filePath(QStringLiteral("bulge.dxf"));
    SDxfCodec codec;
    QVERIFY(codec.write(dxf_path, document).isSuccess());
    SCadDocument dxf_document;
    QVERIFY(codec.read(dxf_path, dxf_document).isSuccess());
    const auto& dxf_polyline = std::get<SPolylineEntity>(dxf_document.entities().front().geometry);
    QCOMPARE(dxf_polyline.bulges.size(), stored.bulges.size());
    QVERIFY(std::abs(dxf_polyline.bulges[1] - stored.bulges[1]) < 1.0e-9);

    document.undo();
    const auto& restored = std::get<SPolylineEntity>(document.entities().front().geometry);
    QCOMPARE(restored.vertices.size(), std::size_t(4));
    QVERIFY(std::all_of(restored.bulges.begin(), restored.bulges.end(),
                        [](double bulge)
                        {
                            return std::abs(bulge) <= 1.0e-12;
                        }));
}

void SPolylineCornerTest::bulgedPolylineTrimPreservesCurves()
{
    SEntityRecord cutting_line;
    cutting_line.id = 240;
    cutting_line.type = SEntityType::Line;
    cutting_line.geometry = SLineEntity{{0.0, -10.0}, {0.0, 10.0}};

    SEntityRecord target;
    target.id = 241;
    target.type = SEntityType::Polyline;
    target.layer_name = QStringLiteral("bulged_trim");
    target.line_width_mm = 0.7;
    target.geometry = SPolylineEntity{
        {{-10.0, 0.0}, {-5.0, 0.0}, {5.0, 0.0}, {10.0, 0.0}}, false, {0.0, 1.0, 0.0, 0.0}};
    const std::vector<SEntityRecord> positive_parts =
        trimmedEntityParts(cutting_line, target, {-3.5, -3.5});
    QCOMPARE(positive_parts.size(), std::size_t(2));
    const auto& positive_right = std::get<SPolylineEntity>(positive_parts.back().geometry);
    QCOMPARE(positive_right.vertices.size(), std::size_t(3));
    QVERIFY(std::abs(positive_right.vertices.front().x) < 1.0e-7);
    QVERIFY(std::abs(positive_right.vertices.front().y + 5.0) < 1.0e-7);
    QVERIFY(std::abs(positive_right.bulges.front() - std::tan(3.14159265358979323846 / 8.0)) <
            1.0e-7);
    QCOMPARE(positive_parts.back().layer_name, target.layer_name);
    QCOMPARE(positive_parts.back().line_width_mm, target.line_width_mm);

    target.id = 242;
    target.geometry = SPolylineEntity{
        {{-5.0, 0.0}, {5.0, 0.0}, {5.0, 5.0}, {-5.0, 5.0}}, true, {-1.0, 0.0, 0.0, 0.0}};
    const std::vector<SEntityRecord> closed_parts =
        trimmedEntityParts(cutting_line, target, {-3.5, 3.5});
    QCOMPARE(closed_parts.size(), std::size_t(1));
    const auto& opened = std::get<SPolylineEntity>(closed_parts.front().geometry);
    QVERIFY(!opened.is_closed);
    QVERIFY(opened.vertices.size() >= std::size_t(5));
    QVERIFY(std::any_of(opened.bulges.begin(), opened.bulges.end(),
                        [](double bulge)
                        {
                            return std::abs(bulge + std::tan(3.14159265358979323846 / 8.0)) <
                                   1.0e-7;
                        }));
}

void SPolylineCornerTest::interactiveArcSegmentAndUndo()
{
    double bulge = 0.0;
    QVERIFY(threePointBulge({0.0, 0.0}, {5.0, 5.0}, {10.0, 0.0}, bulge));
    QVERIFY(std::abs(bulge + 1.0) < 1.0e-9);

    SCadDocument document;
    SCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setObjectSnapEnabled(false);
    viewport.setToolMode(SToolMode::Polyline);
    viewport.submitWorldPoint({0.0, 0.0});
    QVERIFY(viewport.submitCommandKeyword(QStringLiteral("ARC")));
    viewport.submitWorldPoint({5.0, 5.0});
    viewport.submitWorldPoint({10.0, 0.0});
    QVERIFY(viewport.submitCommandKeyword(QStringLiteral("LINE")));
    viewport.submitWorldPoint({20.0, 0.0});
    QVERIFY(viewport.submitCommandKeyword(QStringLiteral("DONE")));
    QCOMPARE(document.entities().size(), std::size_t(1));
    const auto& polyline = std::get<SPolylineEntity>(document.entities().front().geometry);
    QCOMPARE(polyline.vertices.size(), std::size_t(3));
    QCOMPARE(polyline.bulges.size(), polyline.vertices.size());
    QVERIFY(std::abs(polyline.bulges.front() + 1.0) < 1.0e-9);
    QCOMPARE(polyline.bulges[1], 0.0);
    document.undo();
    QVERIFY(document.entities().empty());
}

void SPolylineCornerTest::variableWidthPersistenceAndDxf()
{
    SPolylineEntity geometry{{{0.0, 0.0}, {10.0, 0.0}}, false, {0.0, 0.0}, {2.0, 0.0}, {4.0, 0.0}};
    const std::vector<SPoint2d> outline = polylineSegmentOutline(geometry, 0);
    QCOMPARE(outline.size(), std::size_t(4));
    QVERIFY(std::any_of(outline.begin(), outline.end(),
                        [](const SPoint2d& point)
                        {
                            return std::abs(std::abs(point.y) - 2.0) < 1.0e-9;
                        }));

    SCadDocument document;
    SCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setObjectSnapEnabled(false);
    viewport.setToolMode(SToolMode::Polyline);
    viewport.submitWorldPoint({0.0, 0.0});
    QVERIFY(viewport.submitCommandKeyword(QStringLiteral("WIDTH 2 4")));
    viewport.submitWorldPoint({10.0, 0.0});
    QVERIFY(viewport.submitCommandKeyword(QStringLiteral("DONE")));
    const auto& stored = std::get<SPolylineEntity>(document.entities().front().geometry);
    QCOMPARE(stored.start_widths.front(), 2.0);
    QCOMPARE(stored.end_widths.front(), 4.0);

    QTemporaryDir temporary_directory;
    QVERIFY(temporary_directory.isValid());
    const QString native_path = temporary_directory.filePath(QStringLiteral("width.smartcad"));
    QVERIFY(document.save(native_path).isSuccess());
    SCadDocument loaded_document;
    QVERIFY(loaded_document.load(native_path).isSuccess());
    const auto& loaded = std::get<SPolylineEntity>(loaded_document.entities().front().geometry);
    QCOMPARE(loaded.start_widths.front(), 2.0);
    QCOMPARE(loaded.end_widths.front(), 4.0);

    SDxfCodec codec;
    const QString dxf_path = temporary_directory.filePath(QStringLiteral("width.dxf"));
    QVERIFY(codec.write(dxf_path, document).isSuccess());
    SCadDocument dxf_document;
    QVERIFY(codec.read(dxf_path, dxf_document).isSuccess());
    const auto& dxf_polyline = std::get<SPolylineEntity>(dxf_document.entities().front().geometry);
    QCOMPARE(dxf_polyline.start_widths.front(), 2.0);
    QCOMPARE(dxf_polyline.end_widths.front(), 4.0);
}

void SPolylineCornerTest::polylineEditOperationsAndJoin()
{
    SEntityRecord source;
    source.id = 401;
    source.type = SEntityType::Polyline;
    source.geometry = SPolylineEntity{{{0.0, 0.0}, {10.0, 0.0}, {10.0, 8.0}},
                                      false,
                                      {0.5, -0.25, 0.0},
                                      {1.0, 2.0, 0.0},
                                      {2.0, 3.0, 0.0}};
    SEntityRecord edited;
    QVERIFY(reversedPolylineEntity(source, edited));
    const auto& reversed = std::get<SPolylineEntity>(edited.geometry);
    QCOMPARE(reversed.vertices.front().y, 8.0);
    QCOMPARE(reversed.bulges[0], 0.25);
    QCOMPARE(reversed.bulges[1], -0.5);
    QCOMPARE(reversed.start_widths[0], 3.0);
    QCOMPARE(reversed.end_widths[0], 2.0);

    QVERIFY(polylineClosedStateEntity(source, true, edited));
    QVERIFY(std::get<SPolylineEntity>(edited.geometry).is_closed);
    QVERIFY(decurvedPolylineEntity(source, edited));
    const auto& decurved = std::get<SPolylineEntity>(edited.geometry);
    QVERIFY(std::all_of(decurved.bulges.begin(), decurved.bulges.end(),
                        [](double bulge)
                        {
                            return bulge == 0.0;
                        }));

    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("PEDIT setup"));
    transaction->addPolyline({{0.0, 0.0}, {10.0, 0.0}}, false, {0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0});
    transaction->addPolyline({{10.0, 0.0}, {20.0, 0.0}}, false, {0.25, 0.0}, {3.0, 0.0},
                             {4.0, 0.0});
    transaction->commit();

    SCadViewport viewport;
    viewport.setDocument(&document);
    viewport.selectAll();
    viewport.setToolMode(SToolMode::PolylineEdit);
    QVERIFY(viewport.submitCommandKeyword(QStringLiteral("JOIN")));
    QCOMPARE(document.entities().size(), std::size_t(1));
    const auto& joined = std::get<SPolylineEntity>(document.entities().front().geometry);
    QCOMPARE(joined.vertices.size(), std::size_t(3));
    QCOMPARE(joined.start_widths[0], 1.0);
    QCOMPARE(joined.end_widths[0], 2.0);
    QCOMPARE(joined.start_widths[1], 3.0);
    QCOMPARE(joined.end_widths[1], 4.0);

    QVERIFY(viewport.submitCommandKeyword(QStringLiteral("WIDTH 5")));
    const auto& widened = std::get<SPolylineEntity>(document.entities().front().geometry);
    QCOMPARE(widened.start_widths[0], 5.0);
    QCOMPARE(widened.end_widths[1], 5.0);
    QVERIFY(viewport.submitCommandKeyword(QStringLiteral("CLOSE")));
    QVERIFY(std::get<SPolylineEntity>(document.entities().front().geometry).is_closed);
    QVERIFY(viewport.submitCommandKeyword(QStringLiteral("OPEN")));
    QVERIFY(!std::get<SPolylineEntity>(document.entities().front().geometry).is_closed);
    QVERIFY(viewport.submitCommandKeyword(QStringLiteral("REVERSE")));
    QCOMPARE(std::get<SPolylineEntity>(document.entities().front().geometry).vertices.front().x,
             20.0);
    document.undo();
    QCOMPARE(std::get<SPolylineEntity>(document.entities().front().geometry).vertices.front().x,
             0.0);
}

} // namespace vectorPath

QTEST_MAIN(vectorPath::SPolylineCornerTest)
#include "s_polyline_corner_test.moc"
