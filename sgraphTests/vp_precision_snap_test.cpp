#include "vp_cad_document.h"
#include "vp_cad_viewport.h"
#include "vp_command_catalog.h"
#include "vp_coordinate_input.h"
#include "vp_document_transaction.h"
#include "vp_object_snap.h"
#include "vp_test_runner.h"

#include <QtTest>

namespace Vp
{

class VpPrecisionSnapTest final : public QObject
{
    Q_OBJECT

  private slots:
    void apparentIntersectionExtensionAndParallel();
    void objectTrackingCreatesAlignedPoint();
    void relativeCartesianAndPolarCoordinateInput();
    void commandCompletionAndActiveKeywords();
};

void VpPrecisionSnapTest::apparentIntersectionExtensionAndParallel()
{
    VpEntityRecord horizontal;
    horizontal.id = 320;
    horizontal.type = VpEntityType::Line;
    horizontal.geometry = VpLineEntity{{0.0, 0.0}, {5.0, 0.0}};

    const auto extension = findObjectSnap({&horizontal}, {8.0, 0.1}, std::nullopt, 0.5);
    QVERIFY(extension.has_value());
    QCOMPARE(extension->type, VpObjectSnapType::Extension);
    QCOMPARE(extension->point.x, 8.0);
    QCOMPARE(extension->point.y, 0.0);

    VpEntityRecord vertical;
    vertical.id = 321;
    vertical.type = VpEntityType::Line;
    vertical.geometry = VpLineEntity{{10.0, 5.0}, {10.0, 10.0}};
    const auto apparent = findObjectSnap({&horizontal, &vertical}, {10.1, 0.1}, std::nullopt, 0.5);
    QVERIFY(apparent.has_value());
    QCOMPARE(apparent->type, VpObjectSnapType::ApparentIntersection);
    QVERIFY(std::abs(apparent->point.x - 10.0) < 1.0e-9);
    QVERIFY(std::abs(apparent->point.y) < 1.0e-9);

    const auto parallel = findObjectSnap({&horizontal}, {8.0, 4.1}, VpPoint2d{2.0, 4.0}, 0.5);
    QVERIFY(parallel.has_value());
    QCOMPARE(parallel->type, VpObjectSnapType::Parallel);
    QCOMPARE(parallel->point.x, 8.0);
    QCOMPARE(parallel->point.y, 4.0);
}

void VpPrecisionSnapTest::objectTrackingCreatesAlignedPoint()
{
    VpCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("tracking source"));
    transaction->addLine({0.0, 0.0}, {10.0, 0.0});
    transaction->commit();

    VpCadViewport viewport;
    viewport.resize(640, 480);
    viewport.setDocument(&document);
    viewport.setTrackingEnabled(true);
    viewport.setToolMode(VpToolMode::Line);
    viewport.submitWorldPoint({0.0, 0.0});
    QTest::mouseClick(&viewport, Qt::LeftButton, Qt::NoModifier, QPoint(322, 220));

    QCOMPARE(document.entities().size(), std::size_t(2));
    const auto& tracked_line = std::get<VpLineEntity>(document.entities().back().geometry);
    QCOMPARE(tracked_line.start_point.x, 0.0);
    QCOMPARE(tracked_line.end_point.x, 0.0);
    QCOMPARE(tracked_line.end_point.y, 20.0);
}

void VpPrecisionSnapTest::relativeCartesianAndPolarCoordinateInput()
{
    const VpCoordinateInput absolute = parseCoordinateInput(QStringLiteral("5,7"));
    const VpCoordinateInput relative = parseCoordinateInput(QStringLiteral("@10,5"));
    const VpCoordinateInput polar = parseCoordinateInput(QStringLiteral("@10<90"));
    QCOMPARE(absolute.mode, VpCoordinateInputMode::AbsoluteCartesian);
    QCOMPARE(relative.mode, VpCoordinateInputMode::RelativeCartesian);
    QCOMPARE(polar.mode, VpCoordinateInputMode::RelativePolar);
    QCOMPARE(parseCoordinateInput(QStringLiteral("@bad")).mode, VpCoordinateInputMode::Invalid);

    VpCadDocument document;
    VpCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setToolMode(VpToolMode::Line);
    QVERIFY(!viewport.submitCoordinateInput(relative));
    QVERIFY(viewport.previewCoordinateInput(absolute));
    QVERIFY(viewport.commandPreviewPoint().has_value());
    QCOMPARE(viewport.commandPreviewPoint()->x, 5.0);
    QCOMPARE(viewport.commandPreviewPoint()->y, 7.0);
    QVERIFY(viewport.submitCoordinateInput(absolute));
    QVERIFY(!viewport.commandPreviewPoint().has_value());
    QVERIFY(viewport.previewCoordinateInput(relative));
    QCOMPARE(viewport.commandPreviewPoint()->x, 15.0);
    QCOMPARE(viewport.commandPreviewPoint()->y, 12.0);
    QVERIFY(viewport.submitCoordinateInput(relative));
    QVERIFY(viewport.submitCoordinateInput(polar));

    QCOMPARE(document.entities().size(), std::size_t(2));
    const auto& first_line = std::get<VpLineEntity>(document.entities()[0].geometry);
    QCOMPARE(first_line.start_point.x, 5.0);
    QCOMPARE(first_line.start_point.y, 7.0);
    QCOMPARE(first_line.end_point.x, 15.0);
    QCOMPARE(first_line.end_point.y, 12.0);
    const auto& second_line = std::get<VpLineEntity>(document.entities()[1].geometry);
    QVERIFY(std::abs(second_line.end_point.x - 15.0) < 1.0e-9);
    QVERIFY(std::abs(second_line.end_point.y - 22.0) < 1.0e-9);

    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(1));
    document.redo();
    QCOMPARE(document.entities().size(), std::size_t(2));
}

void VpPrecisionSnapTest::commandCompletionAndActiveKeywords()
{
    const QStringList polyline_matches = commandCompletionsForPrefix(QStringLiteral("pli"));
    QVERIFY(polyline_matches.contains(QStringLiteral("PLINE")));
    const QStringList fillet_matches = commandCompletionsForPrefix(QStringLiteral("fillet r"));
    QVERIFY(fillet_matches.contains(QStringLiteral("FILLET R 5")));

    VpCadDocument document;
    VpCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setToolMode(VpToolMode::Polyline);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({10.0, 0.0});
    viewport.submitWorldPoint({10.0, 10.0});
    QVERIFY(viewport.submitCommandKeyword(QStringLiteral("UNDO")));
    viewport.submitWorldPoint({10.0, 10.0});
    QVERIFY(viewport.submitCommandKeyword(QStringLiteral("CLOSE")));

    QCOMPARE(document.entities().size(), std::size_t(1));
    const auto& polyline = std::get<VpPolylineEntity>(document.entities().front().geometry);
    QVERIFY(polyline.is_closed);
    QCOMPARE(polyline.vertices.size(), std::size_t(3));
    document.undo();
    QVERIFY(document.entities().empty());
    document.redo();
    QCOMPARE(document.entities().size(), std::size_t(1));

    viewport.setToolMode(VpToolMode::Line);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({10.0, 0.0});
    viewport.submitWorldPoint({10.0, 10.0});
    QVERIFY(viewport.submitCommandKeyword(QStringLiteral("U")));
    QCOMPARE(document.entities().size(), std::size_t(2));
    viewport.submitWorldPoint({10.0, 10.0});
    QVERIFY(viewport.submitCommandKeyword(QStringLiteral("C")));
    QCOMPARE(document.entities().size(), std::size_t(4));
    const auto& closing_line = std::get<VpLineEntity>(document.entities().back().geometry);
    QCOMPARE(closing_line.start_point.x, 10.0);
    QCOMPARE(closing_line.start_point.y, 10.0);
    QCOMPARE(closing_line.end_point.x, 0.0);
    QCOMPARE(closing_line.end_point.y, 0.0);
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(3));
}

} // namespace Vp

VECTORPATH_TEST_ENTRY(Vp::VpPrecisionSnapTest, vpRunVpPrecisionSnapTest)
#include "vp_precision_snap_test.moc"
