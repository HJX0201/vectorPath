#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_command_catalog.h"
#include "s_coordinate_input.h"
#include "s_document_transaction.h"
#include "s_object_snap.h"

#include <QtTest>

namespace smartGraphics
{

class SPrecisionSnapTest final : public QObject
{
    Q_OBJECT

  private slots:
    void apparentIntersectionExtensionAndParallel();
    void objectTrackingCreatesAlignedPoint();
    void relativeCartesianAndPolarCoordinateInput();
    void commandCompletionAndActiveKeywords();
};

void SPrecisionSnapTest::apparentIntersectionExtensionAndParallel()
{
    SEntityRecord horizontal;
    horizontal.id = 320;
    horizontal.type = SEntityType::Line;
    horizontal.geometry = SLineEntity{{0.0, 0.0}, {5.0, 0.0}};

    const auto extension = findObjectSnap({&horizontal}, {8.0, 0.1}, std::nullopt, 0.5);
    QVERIFY(extension.has_value());
    QCOMPARE(extension->type, SObjectSnapType::Extension);
    QCOMPARE(extension->point.x, 8.0);
    QCOMPARE(extension->point.y, 0.0);

    SEntityRecord vertical;
    vertical.id = 321;
    vertical.type = SEntityType::Line;
    vertical.geometry = SLineEntity{{10.0, 5.0}, {10.0, 10.0}};
    const auto apparent = findObjectSnap({&horizontal, &vertical}, {10.1, 0.1}, std::nullopt, 0.5);
    QVERIFY(apparent.has_value());
    QCOMPARE(apparent->type, SObjectSnapType::ApparentIntersection);
    QVERIFY(std::abs(apparent->point.x - 10.0) < 1.0e-9);
    QVERIFY(std::abs(apparent->point.y) < 1.0e-9);

    const auto parallel = findObjectSnap({&horizontal}, {8.0, 4.1}, SPoint2d{2.0, 4.0}, 0.5);
    QVERIFY(parallel.has_value());
    QCOMPARE(parallel->type, SObjectSnapType::Parallel);
    QCOMPARE(parallel->point.x, 8.0);
    QCOMPARE(parallel->point.y, 4.0);
}

void SPrecisionSnapTest::objectTrackingCreatesAlignedPoint()
{
    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("tracking source"));
    transaction->addLine({0.0, 0.0}, {10.0, 0.0});
    transaction->commit();

    SCadViewport viewport;
    viewport.resize(640, 480);
    viewport.setDocument(&document);
    viewport.setTrackingEnabled(true);
    viewport.setToolMode(SToolMode::Line);
    viewport.submitWorldPoint({0.0, 0.0});
    QTest::mouseClick(&viewport, Qt::LeftButton, Qt::NoModifier, QPoint(322, 220));

    QCOMPARE(document.entities().size(), std::size_t(2));
    const auto& tracked_line = std::get<SLineEntity>(document.entities().back().geometry);
    QCOMPARE(tracked_line.start_point.x, 0.0);
    QCOMPARE(tracked_line.end_point.x, 0.0);
    QCOMPARE(tracked_line.end_point.y, 20.0);
}

void SPrecisionSnapTest::relativeCartesianAndPolarCoordinateInput()
{
    const SCoordinateInput absolute = parseCoordinateInput(QStringLiteral("5,7"));
    const SCoordinateInput relative = parseCoordinateInput(QStringLiteral("@10,5"));
    const SCoordinateInput polar = parseCoordinateInput(QStringLiteral("@10<90"));
    QCOMPARE(absolute.mode, SCoordinateInputMode::AbsoluteCartesian);
    QCOMPARE(relative.mode, SCoordinateInputMode::RelativeCartesian);
    QCOMPARE(polar.mode, SCoordinateInputMode::RelativePolar);
    QCOMPARE(parseCoordinateInput(QStringLiteral("@bad")).mode, SCoordinateInputMode::Invalid);

    SCadDocument document;
    SCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setToolMode(SToolMode::Line);
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
    const auto& first_line = std::get<SLineEntity>(document.entities()[0].geometry);
    QCOMPARE(first_line.start_point.x, 5.0);
    QCOMPARE(first_line.start_point.y, 7.0);
    QCOMPARE(first_line.end_point.x, 15.0);
    QCOMPARE(first_line.end_point.y, 12.0);
    const auto& second_line = std::get<SLineEntity>(document.entities()[1].geometry);
    QVERIFY(std::abs(second_line.end_point.x - 15.0) < 1.0e-9);
    QVERIFY(std::abs(second_line.end_point.y - 22.0) < 1.0e-9);

    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(1));
    document.redo();
    QCOMPARE(document.entities().size(), std::size_t(2));
}

void SPrecisionSnapTest::commandCompletionAndActiveKeywords()
{
    const QStringList polyline_matches = commandCompletionsForPrefix(QStringLiteral("pli"));
    QVERIFY(polyline_matches.contains(QStringLiteral("PLINE")));
    const QStringList fillet_matches = commandCompletionsForPrefix(QStringLiteral("fillet r"));
    QVERIFY(fillet_matches.contains(QStringLiteral("FILLET R 5")));

    SCadDocument document;
    SCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setToolMode(SToolMode::Polyline);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({10.0, 0.0});
    viewport.submitWorldPoint({10.0, 10.0});
    QVERIFY(viewport.submitCommandKeyword(QStringLiteral("UNDO")));
    viewport.submitWorldPoint({10.0, 10.0});
    QVERIFY(viewport.submitCommandKeyword(QStringLiteral("CLOSE")));

    QCOMPARE(document.entities().size(), std::size_t(1));
    const auto& polyline = std::get<SPolylineEntity>(document.entities().front().geometry);
    QVERIFY(polyline.is_closed);
    QCOMPARE(polyline.vertices.size(), std::size_t(3));
    document.undo();
    QVERIFY(document.entities().empty());
    document.redo();
    QCOMPARE(document.entities().size(), std::size_t(1));

    viewport.setToolMode(SToolMode::Line);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({10.0, 0.0});
    viewport.submitWorldPoint({10.0, 10.0});
    QVERIFY(viewport.submitCommandKeyword(QStringLiteral("U")));
    QCOMPARE(document.entities().size(), std::size_t(2));
    viewport.submitWorldPoint({10.0, 10.0});
    QVERIFY(viewport.submitCommandKeyword(QStringLiteral("C")));
    QCOMPARE(document.entities().size(), std::size_t(4));
    const auto& closing_line = std::get<SLineEntity>(document.entities().back().geometry);
    QCOMPARE(closing_line.start_point.x, 10.0);
    QCOMPARE(closing_line.start_point.y, 10.0);
    QCOMPARE(closing_line.end_point.x, 0.0);
    QCOMPARE(closing_line.end_point.y, 0.0);
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(3));
}

} // namespace smartGraphics

QTEST_MAIN(smartGraphics::SPrecisionSnapTest)
#include "s_precision_snap_test.moc"
