#include "vp_cad_document.h"
#include "vp_cad_viewport.h"
#include "vp_cad_viewport_geometry.h"
#include "vp_document_transaction.h"
#include "vp_dxf_codec.h"
#include "vp_object_snap.h"
#include "vp_spline_geometry.h"
#include "vp_test_runner.h"

#include <QTemporaryDir>
#include <QtTest>
#include <cmath>

namespace Vp
{

class VpSplineBlendTest final : public QObject
{
    Q_OBJECT

  private slots:
    void tangentBlendGeometry();
    void viewportBlendPersistenceAndUndo();
    void splineTransformGripAndSnap();
    void interactiveSplineAndUndo();
    void alignGeometryAndUndo();
};

void VpSplineBlendTest::tangentBlendGeometry()
{
    VpEntityRecord horizontal;
    horizontal.id = 400;
    horizontal.type = VpEntityType::Line;
    horizontal.layer_name = QStringLiteral("blend");
    horizontal.line_width_mm = 0.5;
    horizontal.geometry = VpLineEntity{{-10.0, 0.0}, {0.0, 0.0}};
    VpEntityRecord vertical;
    vertical.id = 401;
    vertical.type = VpEntityType::Line;
    vertical.geometry = VpLineEntity{{10.0, 10.0}, {10.0, 20.0}};
    VpEntityRecord result;
    QVERIFY(blendedSplineEntity(horizontal, vertical, {0.0, 0.0}, {10.0, 10.0}, result));
    QCOMPARE(result.type, VpEntityType::Spline);
    QCOMPARE(result.layer_name, horizontal.layer_name);
    QCOMPARE(result.line_width_mm, horizontal.line_width_mm);
    const auto& spline = std::get<VpSplineEntity>(result.geometry);
    QCOMPARE(spline.control_points.front().x, 0.0);
    QCOMPARE(spline.control_points.front().y, 0.0);
    QCOMPARE(spline.control_points.back().x, 10.0);
    QCOMPARE(spline.control_points.back().y, 10.0);
    const VpPoint2d start_tangent = splineTangent(spline, 0.0);
    const VpPoint2d end_tangent = splineTangent(spline, 1.0);
    QVERIFY(start_tangent.x > 0.0);
    QVERIFY(std::abs(start_tangent.y) < 1.0e-9);
    QVERIFY(std::abs(end_tangent.x) < 1.0e-9);
    QVERIFY(end_tangent.y > 0.0);
}

void VpSplineBlendTest::viewportBlendPersistenceAndUndo()
{
    VpCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("blend sources"));
    transaction->addLine({-10.0, 0.0}, {0.0, 0.0});
    transaction->addLine({10.0, 10.0}, {10.0, 20.0});
    transaction->commit();
    VpCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setObjectSnapEnabled(false);
    viewport.setToolMode(VpToolMode::Blend);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({10.0, 10.0});
    QCOMPARE(document.entities().size(), std::size_t(3));
    QCOMPARE(document.entities().back().type, VpEntityType::Spline);

    QTemporaryDir temporary_directory;
    QVERIFY(temporary_directory.isValid());
    const QString native_path = temporary_directory.filePath(QStringLiteral("blend.smartcad"));
    QVERIFY(document.save(native_path).isSuccess());
    VpCadDocument loaded_document;
    QVERIFY(loaded_document.load(native_path).isSuccess());
    QCOMPARE(loaded_document.entities().back().type, VpEntityType::Spline);
    const auto& loaded_spline =
        std::get<VpSplineEntity>(loaded_document.entities().back().geometry);
    QCOMPARE(loaded_spline.control_points.front().x, 0.0);
    QCOMPARE(loaded_spline.control_points.back().y, 10.0);

    VpDxfCodec codec;
    const QString dxf_path = temporary_directory.filePath(QStringLiteral("blend.dxf"));
    const auto write_result = codec.write(dxf_path, document);
    QVERIFY(write_result.isSuccess());
    QCOMPARE(write_result.value().skipped_entity_count, std::size_t(0));
    VpCadDocument dxf_document;
    const auto read_result = codec.read(dxf_path, dxf_document);
    QVERIFY(read_result.isSuccess());
    QCOMPARE(dxf_document.entities().size(), std::size_t(3));
    QCOMPARE(dxf_document.entities().back().type, VpEntityType::Spline);
    const auto& dxf_spline = std::get<VpSplineEntity>(dxf_document.entities().back().geometry);
    QCOMPARE(dxf_spline.control_points.front().x, 0.0);
    QCOMPARE(dxf_spline.control_points.back().y, 10.0);

    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(2));
}

void VpSplineBlendTest::splineTransformGripAndSnap()
{
    VpEntityRecord spline_entity;
    spline_entity.id = 402;
    spline_entity.type = VpEntityType::Spline;
    spline_entity.geometry = VpSplineEntity{
        {VpPoint2d{0.0, 0.0}, VpPoint2d{3.0, 0.0}, VpPoint2d{7.0, 10.0}, VpPoint2d{10.0, 10.0}}};
    const VpEntityRecord moved = translatedEntity(spline_entity, 5.0, -2.0);
    QCOMPARE(std::get<VpSplineEntity>(moved.geometry).control_points.front().x, 5.0);
    QCOMPARE(std::get<VpSplineEntity>(moved.geometry).control_points.back().y, 8.0);

    const std::vector<VpGripHandle> grips = entityGripHandles(spline_entity);
    QCOMPARE(grips.size(), std::size_t(4));
    VpEntityRecord edited;
    QVERIFY(gripEditedEntity(spline_entity, grips[1], {4.0, 2.0}, edited));
    QCOMPARE(std::get<VpSplineEntity>(edited.geometry).control_points[1].x, 4.0);

    const auto endpoint_snap = findObjectSnap({&spline_entity}, {0.0, 0.0}, std::nullopt, 0.1);
    QVERIFY(endpoint_snap.has_value());
    QCOMPARE(endpoint_snap->type, VpObjectSnapType::Endpoint);
    const VpPoint2d midpoint = splinePoint(std::get<VpSplineEntity>(spline_entity.geometry), 0.5);
    const auto midpoint_snap = findObjectSnap({&spline_entity}, midpoint, std::nullopt, 0.1);
    QVERIFY(midpoint_snap.has_value());
    QCOMPARE(midpoint_snap->type, VpObjectSnapType::Midpoint);
}

void VpSplineBlendTest::alignGeometryAndUndo()
{
    VpEntityRecord line;
    line.id = 403;
    line.type = VpEntityType::Line;
    line.geometry = VpLineEntity{{0.0, 0.0}, {10.0, 0.0}};
    const VpEntityRecord scaled =
        alignedEntity(line, {0.0, 0.0}, {5.0, 5.0}, {10.0, 0.0}, {5.0, 25.0}, true);
    const auto& scaled_line = std::get<VpLineEntity>(scaled.geometry);
    QVERIFY(std::abs(scaled_line.start_point.x - 5.0) < 1.0e-9);
    QVERIFY(std::abs(scaled_line.start_point.y - 5.0) < 1.0e-9);
    QVERIFY(std::abs(scaled_line.end_point.x - 5.0) < 1.0e-9);
    QVERIFY(std::abs(scaled_line.end_point.y - 25.0) < 1.0e-9);
    const VpEntityRecord unscaled =
        alignedEntity(line, {0.0, 0.0}, {5.0, 5.0}, {10.0, 0.0}, {5.0, 25.0}, false);
    QVERIFY(std::abs(std::get<VpLineEntity>(unscaled.geometry).end_point.y - 15.0) < 1.0e-9);

    VpCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("align source"));
    transaction->addLine({0.0, 0.0}, {10.0, 0.0});
    transaction->addCircle({2.0, 2.0}, 1.0);
    transaction->commit();
    VpCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setObjectSnapEnabled(false);
    viewport.selectAll();
    viewport.setToolMode(VpToolMode::Align);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({5.0, 5.0});
    viewport.submitWorldPoint({10.0, 0.0});
    viewport.submitWorldPoint({5.0, 25.0});
    const auto& aligned_line = std::get<VpLineEntity>(document.entities()[0].geometry);
    QVERIFY(std::abs(aligned_line.end_point.y - 25.0) < 1.0e-9);
    const auto& aligned_circle = std::get<VpCircleEntity>(document.entities()[1].geometry);
    QCOMPARE(aligned_circle.radius, 2.0);
    document.undo();
    QCOMPARE(std::get<VpLineEntity>(document.entities()[0].geometry).end_point.x, 10.0);
    QCOMPARE(std::get<VpCircleEntity>(document.entities()[1].geometry).radius, 1.0);
}

void VpSplineBlendTest::interactiveSplineAndUndo()
{
    VpCadDocument document;
    VpCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setObjectSnapEnabled(false);
    viewport.setToolMode(VpToolMode::Spline);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({5.0, 10.0});
    viewport.submitWorldPoint({10.0, 10.0});
    viewport.submitWorldPoint({15.0, 0.0});
    QCOMPARE(document.entities().size(), std::size_t(1));
    QCOMPARE(document.entities().front().type, VpEntityType::Spline);
    const auto& spline = std::get<VpSplineEntity>(document.entities().front().geometry);
    QCOMPARE(spline.control_points[1].y, 10.0);
    QCOMPARE(spline.control_points.back().x, 15.0);
    document.undo();
    QVERIFY(document.entities().empty());
}

} // namespace Vp

VECTORPATH_TEST_ENTRY(Vp::VpSplineBlendTest, vpRunVpSplineBlendTest)
#include "vp_spline_blend_test.moc"
