#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_cad_viewport_geometry.h"
#include "s_document_transaction.h"

#include <QtTest>
#include <cmath>

namespace smartCam
{

class SCurveOffsetTest final : public QObject
{
    Q_OBJECT

  private slots:
    void openPolylineOffset();
    void closedPolylineOffset();
    void bulgedPolylineOffset();
    void splineOffsetUsesPolylineApproximation();
    void viewportOffsetAndUndo();
};

void SCurveOffsetTest::openPolylineOffset()
{
    SEntityRecord source;
    source.id = 10;
    source.type = SEntityType::Polyline;
    source.layer_name = QStringLiteral("construction");
    source.line_width_mm = 0.35;
    source.geometry = SPolylineEntity{{{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}}, false};

    SEntityRecord result;
    QVERIFY(offsetEntity(source, {5.0, 2.0}, result));
    QCOMPARE(result.type, SEntityType::Polyline);
    QCOMPARE(result.layer_name, source.layer_name);
    QCOMPARE(result.line_width_mm, source.line_width_mm);
    const auto& offset = std::get<SPolylineEntity>(result.geometry);
    QVERIFY(!offset.is_closed);
    QCOMPARE(offset.vertices.size(), std::size_t(3));
    QVERIFY(distance(offset.vertices[0], {0.0, 2.0}) < 1.0e-8);
    QVERIFY(distance(offset.vertices[1], {8.0, 2.0}) < 1.0e-8);
    QVERIFY(distance(offset.vertices[2], {8.0, 10.0}) < 1.0e-8);
}

void SCurveOffsetTest::closedPolylineOffset()
{
    SEntityRecord source;
    source.type = SEntityType::Polyline;
    source.geometry = SPolylineEntity{{{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {0.0, 10.0}}, true};

    SEntityRecord result;
    QVERIFY(offsetEntity(source, {5.0, -2.0}, result));
    const auto& offset = std::get<SPolylineEntity>(result.geometry);
    QVERIFY(offset.is_closed);
    QCOMPARE(offset.vertices.size(), std::size_t(4));
    QVERIFY(distance(offset.vertices[0], {-2.0, -2.0}) < 1.0e-8);
    QVERIFY(distance(offset.vertices[2], {12.0, 12.0}) < 1.0e-8);

    SEntityRecord inside_result;
    QVERIFY(offsetEntity(source, {5.0, 1.0}, inside_result));
    const auto& inside = std::get<SPolylineEntity>(inside_result.geometry);
    QVERIFY(distance(inside.vertices[0], {1.0, 1.0}) < 1.0e-8);
    QVERIFY(distance(inside.vertices[2], {9.0, 9.0}) < 1.0e-8);
}

void SCurveOffsetTest::bulgedPolylineOffset()
{
    SEntityRecord source;
    source.type = SEntityType::Polyline;
    source.geometry = SPolylineEntity{{{0.0, 0.0}, {10.0, 0.0}}, false, {1.0, 0.0}};

    SEntityRecord result;
    QVERIFY(offsetEntity(source, {5.0, -7.0}, result));
    const auto& offset = std::get<SPolylineEntity>(result.geometry);
    QVERIFY(offset.vertices.size() >= std::size_t(30));
    for (const SPoint2d& point : offset.vertices)
    {
        QVERIFY(std::isfinite(point.x));
        QVERIFY(std::isfinite(point.y));
    }
}

void SCurveOffsetTest::splineOffsetUsesPolylineApproximation()
{
    SEntityRecord source;
    source.type = SEntityType::Spline;
    source.layer_name = QStringLiteral("curves");
    source.line_width_mm = 0.5;
    source.geometry = SSplineEntity{
        {SPoint2d{0.0, 0.0}, SPoint2d{3.0, 8.0}, SPoint2d{7.0, -8.0}, SPoint2d{10.0, 0.0}}};

    SEntityRecord result;
    QVERIFY(offsetEntity(source, {1.0, 3.0}, result));
    QCOMPARE(result.type, SEntityType::Polyline);
    QCOMPARE(result.layer_name, source.layer_name);
    QCOMPARE(result.line_width_mm, source.line_width_mm);
    QVERIFY(std::get<SPolylineEntity>(result.geometry).vertices.size() > std::size_t(20));
}

void SCurveOffsetTest::viewportOffsetAndUndo()
{
    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("offset source"));
    transaction->addPolyline({{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}}, false);
    transaction->commit();

    SCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setToolMode(SToolMode::Offset);
    viewport.submitWorldPoint({5.0, 0.0});
    QVERIFY(viewport.selectedEntityId().has_value());
    viewport.submitWorldPoint({5.0, 2.0});
    QCOMPARE(document.entities().size(), std::size_t(2));
    QCOMPARE(document.entities().back().type, SEntityType::Polyline);
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(1));
}

} // namespace smartCam

QTEST_MAIN(smartCam::SCurveOffsetTest)
#include "s_curve_offset_test.moc"
