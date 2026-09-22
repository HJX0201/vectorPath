#include "vp_cad_document.h"
#include "vp_cad_viewport.h"
#include "vp_cad_viewport_geometry.h"
#include "vp_document_transaction.h"
#include "vp_test_runner.h"

#include <QtTest>
#include <cmath>

namespace Vp
{

class VpCurveOffsetTest final : public QObject
{
    Q_OBJECT

  private slots:
    void openPolylineOffset();
    void closedPolylineOffset();
    void bulgedPolylineOffset();
    void splineOffsetUsesPolylineApproximation();
    void viewportOffsetAndUndo();
};

void VpCurveOffsetTest::openPolylineOffset()
{
    VpEntityRecord source;
    source.id = 10;
    source.type = VpEntityType::Polyline;
    source.layer_name = QStringLiteral("construction");
    source.line_width_mm = 0.35;
    source.geometry = VpPolylineEntity{{{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}}, false};

    VpEntityRecord result;
    QVERIFY(offsetEntity(source, {5.0, 2.0}, result));
    QCOMPARE(result.type, VpEntityType::Polyline);
    QCOMPARE(result.layer_name, source.layer_name);
    QCOMPARE(result.line_width_mm, source.line_width_mm);
    const auto& offset = std::get<VpPolylineEntity>(result.geometry);
    QVERIFY(!offset.is_closed);
    QCOMPARE(offset.vertices.size(), std::size_t(3));
    QVERIFY(distance(offset.vertices[0], {0.0, 2.0}) < 1.0e-8);
    QVERIFY(distance(offset.vertices[1], {8.0, 2.0}) < 1.0e-8);
    QVERIFY(distance(offset.vertices[2], {8.0, 10.0}) < 1.0e-8);
}

void VpCurveOffsetTest::closedPolylineOffset()
{
    VpEntityRecord source;
    source.type = VpEntityType::Polyline;
    source.geometry = VpPolylineEntity{{{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {0.0, 10.0}}, true};

    VpEntityRecord result;
    QVERIFY(offsetEntity(source, {5.0, -2.0}, result));
    const auto& offset = std::get<VpPolylineEntity>(result.geometry);
    QVERIFY(offset.is_closed);
    QCOMPARE(offset.vertices.size(), std::size_t(4));
    QVERIFY(distance(offset.vertices[0], {-2.0, -2.0}) < 1.0e-8);
    QVERIFY(distance(offset.vertices[2], {12.0, 12.0}) < 1.0e-8);

    VpEntityRecord inside_result;
    QVERIFY(offsetEntity(source, {5.0, 1.0}, inside_result));
    const auto& inside = std::get<VpPolylineEntity>(inside_result.geometry);
    QVERIFY(distance(inside.vertices[0], {1.0, 1.0}) < 1.0e-8);
    QVERIFY(distance(inside.vertices[2], {9.0, 9.0}) < 1.0e-8);
}

void VpCurveOffsetTest::bulgedPolylineOffset()
{
    VpEntityRecord source;
    source.type = VpEntityType::Polyline;
    source.geometry = VpPolylineEntity{{{0.0, 0.0}, {10.0, 0.0}}, false, {1.0, 0.0}};

    VpEntityRecord result;
    QVERIFY(offsetEntity(source, {5.0, -7.0}, result));
    const auto& offset = std::get<VpPolylineEntity>(result.geometry);
    QVERIFY(offset.vertices.size() >= std::size_t(30));
    for (const VpPoint2d& point : offset.vertices)
    {
        QVERIFY(std::isfinite(point.x));
        QVERIFY(std::isfinite(point.y));
    }
}

void VpCurveOffsetTest::splineOffsetUsesPolylineApproximation()
{
    VpEntityRecord source;
    source.type = VpEntityType::Spline;
    source.layer_name = QStringLiteral("curves");
    source.line_width_mm = 0.5;
    source.geometry = VpSplineEntity{
        {VpPoint2d{0.0, 0.0}, VpPoint2d{3.0, 8.0}, VpPoint2d{7.0, -8.0}, VpPoint2d{10.0, 0.0}}};

    VpEntityRecord result;
    QVERIFY(offsetEntity(source, {1.0, 3.0}, result));
    QCOMPARE(result.type, VpEntityType::Polyline);
    QCOMPARE(result.layer_name, source.layer_name);
    QCOMPARE(result.line_width_mm, source.line_width_mm);
    QVERIFY(std::get<VpPolylineEntity>(result.geometry).vertices.size() > std::size_t(20));
}

void VpCurveOffsetTest::viewportOffsetAndUndo()
{
    VpCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("offset source"));
    transaction->addPolyline({{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}}, false);
    transaction->commit();

    VpCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setToolMode(VpToolMode::Offset);
    viewport.submitWorldPoint({5.0, 0.0});
    QVERIFY(viewport.selectedEntityId().has_value());
    viewport.submitWorldPoint({5.0, 2.0});
    QCOMPARE(document.entities().size(), std::size_t(2));
    QCOMPARE(document.entities().back().type, VpEntityType::Polyline);
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(1));
}

} // namespace Vp

VECTORPATH_TEST_ENTRY(Vp::VpCurveOffsetTest, vpRunVpCurveOffsetTest)
#include "vp_curve_offset_test.moc"
