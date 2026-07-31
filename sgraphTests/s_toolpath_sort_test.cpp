#include "s_toolpath.h"
#include "s_toolpath_document.h"
#include "s_cad_document.h"
#include "s_document_transaction.h"
#include "s_dxf_codec.h"

#include <QtTest>
#include <QTemporaryDir>
#include <cmath>

namespace smartCam
{

class SToolpathSortTest final : public QObject
{
    Q_OBJECT

  private slots:
    void rowScanCombinations();
    void rowToleranceUsesBaseline();
    void sortingUsesStartPoint();
    void shortestReversesOnlySelectedCandidate();
    void sortsEachLayerAsACompleteBlock();
    void shortestContinuesFromPreviousLayerEnd();
    void geometryReversal();
    void documentOrderUndoRedoAndPersistence();
    void clockwiseArcPersistenceAndDxfDegradation();
};

SEntityRecord lineEntity(SEntityId id, double start_x, double start_y, double end_x,
                         double end_y)
{
    SEntityRecord entity;
    entity.id = id;
    entity.type = SEntityType::Line;
    entity.geometry = SLineEntity{{start_x, start_y}, {end_x, end_y}};
    return entity;
}

std::vector<SEntityId> ids(const SToolpathSortResult& result)
{
    std::vector<SEntityId> values;
    for (const SEntityRecord& entity : result.entities)
    {
        values.push_back(entity.id);
    }
    return values;
}

SEntityId addDocumentLine(SCadDocument& document, const QString& layer_name, double start_x,
                          double end_x)
{
    document.setCurrentLayer(layer_name);
    Q_ASSERT(document.currentLayerName() == layer_name);
    std::unique_ptr<SDocumentTransaction> transaction = document.beginTransaction("seed line");
    const SEntityId entity_id = transaction->addLine({start_x, 0.0}, {end_x, 0.0});
    transaction->commit();
    return entity_id;
}

void SToolpathSortTest::rowScanCombinations()
{
    const std::vector<SEntityRecord> entities{
        lineEntity(1, 10.0, 10.0, 1000.0, 1000.0), lineEntity(2, 0.0, 10.0, -1000.0, -1000.0),
        lineEntity(3, 10.0, 0.0, 500.0, 500.0), lineEntity(4, 0.0, 0.0, -500.0, -500.0)};
    SToolpathSortOptions options;
    QCOMPARE(ids(sortToolpathEntities(entities, options)), std::vector<SEntityId>({2, 1, 4, 3}));
    options.horizontal = SToolpathHorizontalDirection::RightToLeft;
    QCOMPARE(ids(sortToolpathEntities(entities, options)), std::vector<SEntityId>({1, 2, 3, 4}));
    options.vertical = SToolpathVerticalDirection::BottomToTop;
    QCOMPARE(ids(sortToolpathEntities(entities, options)), std::vector<SEntityId>({3, 4, 1, 2}));
    options.horizontal = SToolpathHorizontalDirection::LeftToRight;
    QCOMPARE(ids(sortToolpathEntities(entities, options)), std::vector<SEntityId>({4, 3, 2, 1}));
}

void SToolpathSortTest::rowToleranceUsesBaseline()
{
    const std::vector<SEntityRecord> entities{
        lineEntity(1, 3.0, 1.01, 3.0, 2.0), lineEntity(2, 2.0, 0.9, 2.0, 1.0),
        lineEntity(3, 1.0, 0.0, 1.0, 1.0), lineEntity(4, 1000.0, 0.0, 1000.0, 1.0)};
    SToolpathSortOptions options;
    options.vertical = SToolpathVerticalDirection::BottomToTop;
    const std::vector<SEntityId> sorted = ids(sortToolpathEntities(entities, options));
    QCOMPARE(sorted, std::vector<SEntityId>({3, 2, 4, 1}));
}

void SToolpathSortTest::sortingUsesStartPoint()
{
    const std::vector<SEntityRecord> entities{lineEntity(1, 5.0, 0.0, -1000.0, 0.0),
                                              lineEntity(2, 1.0, 0.0, 1000.0, 0.0)};
    SToolpathSortOptions options;
    QCOMPARE(ids(sortToolpathEntities(entities, options)), std::vector<SEntityId>({2, 1}));
}

void SToolpathSortTest::shortestReversesOnlySelectedCandidate()
{
    const std::vector<SEntityRecord> entities{lineEntity(1, 10.0, 0.0, 2.0, 0.0),
                                              lineEntity(2, 20.0, 0.0, 11.0, 0.0)};
    SToolpathSortOptions options;
    options.mode = SToolpathSortMode::Shortest;
    options.allow_reverse = true;
    options.shortest_start = {0.0, 0.0};
    const SToolpathSortResult result = sortToolpathEntities(entities, options);
    QCOMPARE(ids(result), std::vector<SEntityId>({1, 2}));
    QCOMPARE(result.reversed_entity_ids, std::vector<SEntityId>({1, 2}));
    QCOMPARE(toolpathStartPoint(result.entities[0]).x, 2.0);
    QCOMPARE(toolpathEndPoint(result.entities[0]).x, 10.0);
    QCOMPARE(toolpathStartPoint(result.entities[1]).x, 11.0);
}

void SToolpathSortTest::sortsEachLayerAsACompleteBlock()
{
    SCadDocument document;
    QVERIFY(document.addLayer(QStringLiteral("A")));
    QVERIFY(document.addLayer(QStringLiteral("B")));
    const SEntityId a_right = addDocumentLine(document, QStringLiteral("A"), 10.0, 11.0);
    const SEntityId b_left = addDocumentLine(document, QStringLiteral("B"), -100.0, -99.0);
    const SEntityId a_left = addDocumentLine(document, QStringLiteral("A"), 0.0, 1.0);
    const SEntityId b_right = addDocumentLine(document, QStringLiteral("B"), 100.0, 101.0);

    const SToolpathDocumentSortResult result =
        sortDocumentToolpaths(document, {}, SToolpathSortOptions{});

    QCOMPARE(result.sorted_entity_count, 4);
    QCOMPARE(document.entities()[0].id, a_left);
    QCOMPARE(document.entities()[1].id, a_right);
    QCOMPARE(document.entities()[2].id, b_left);
    QCOMPARE(document.entities()[3].id, b_right);
}

void SToolpathSortTest::shortestContinuesFromPreviousLayerEnd()
{
    SCadDocument document;
    QVERIFY(document.addLayer(QStringLiteral("A")));
    QVERIFY(document.addLayer(QStringLiteral("B")));
    const SEntityId layer_a = addDocumentLine(document, QStringLiteral("A"), 10.0, 100.0);
    const SEntityId b_near_origin = addDocumentLine(document, QStringLiteral("B"), 1.0, 2.0);
    const SEntityId b_near_layer_end = addDocumentLine(document, QStringLiteral("B"), 90.0, 91.0);
    SToolpathSortOptions options;
    options.mode = SToolpathSortMode::Shortest;
    options.shortest_start = {0.0, 0.0};

    sortDocumentToolpaths(document, {}, options);

    QCOMPARE(document.entities()[0].id, layer_a);
    QCOMPARE(document.entities()[1].id, b_near_layer_end);
    QCOMPARE(document.entities()[2].id, b_near_origin);
}

void SToolpathSortTest::geometryReversal()
{
    SEntityRecord arc;
    arc.id = 1;
    arc.type = SEntityType::Arc;
    arc.geometry = SArcEntity{{0.0, 0.0}, 5.0, 10.0, 80.0, false};
    const auto reversed_arc = std::get<SArcEntity>(reversedToolpathEntity(arc).geometry);
    QCOMPARE(reversed_arc.start_angle, 80.0);
    QCOMPARE(reversed_arc.end_angle, 10.0);
    QVERIFY(reversed_arc.is_clockwise);

    SEntityRecord polyline;
    polyline.type = SEntityType::Polyline;
    polyline.geometry = SPolylineEntity{{{0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0}}, false,
                                        {0.5, -0.25, 0.0}, {1.0, 2.0, 0.0}, {3.0, 4.0, 0.0}};
    const auto reversed_polyline =
        std::get<SPolylineEntity>(reversedToolpathEntity(polyline).geometry);
    QCOMPARE(reversed_polyline.vertices.front().x, 2.0);
    QCOMPARE(reversed_polyline.bulges[0], 0.25);
    QCOMPARE(reversed_polyline.bulges[1], -0.5);
    QCOMPARE(reversed_polyline.start_widths[0], 4.0);
    QCOMPARE(reversed_polyline.end_widths[0], 2.0);
    const std::vector<SToolpathMotion> motions = generateToolpathMotions({polyline}, {-1.0, 0.0});
    QVERIFY(motions.size() > 4);
    QCOMPARE(motions.front().type, SToolpathMotionType::Rapid);
    QCOMPARE(motions.back().end_point.x, 2.0);
}

void SToolpathSortTest::documentOrderUndoRedoAndPersistence()
{
    SCadDocument document;
    std::unique_ptr<SDocumentTransaction> transaction = document.beginTransaction("seed");
    const SEntityId first_id = transaction->addLine({10.0, 0.0}, {20.0, 0.0});
    const SEntityId fixed_id = transaction->addLine({5.0, 0.0}, {6.0, 0.0});
    const SEntityId third_id = transaction->addLine({0.0, 0.0}, {1.0, 0.0});
    transaction->commit();
    SToolpathSortOptions options;
    const SToolpathDocumentSortResult sort_result =
        sortDocumentToolpaths(document, {first_id, third_id}, options);
    QCOMPARE(sort_result.sorted_entity_count, 2);
    QCOMPARE(document.entities()[0].id, third_id);
    QCOMPARE(document.entities()[1].id, fixed_id);
    QCOMPARE(document.entities()[2].id, first_id);
    document.undo();
    QCOMPARE(document.entities()[0].id, first_id);
    QCOMPARE(document.entities()[1].id, fixed_id);
    QCOMPARE(document.entities()[2].id, third_id);
    document.redo();
    QCOMPARE(document.entities()[0].id, third_id);

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath("sorted.smcad");
    QVERIFY(document.save(path));
    SCadDocument loaded;
    QVERIFY(loaded.load(path));
    QCOMPARE(loaded.entities()[0].id, third_id);
    QCOMPARE(loaded.entities()[1].id, fixed_id);
    QCOMPARE(loaded.entities()[2].id, first_id);
}

void SToolpathSortTest::clockwiseArcPersistenceAndDxfDegradation()
{
    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("clockwise arc"));
    SEntityRecord arc;
    arc.type = SEntityType::Arc;
    arc.geometry = SArcEntity{{0.0, 0.0}, 10.0, 90.0, 0.0, true};
    transaction->addEntityCopy(arc);
    transaction->commit();
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString native_path = directory.filePath(QStringLiteral("clockwise.smcad"));
    QVERIFY(document.save(native_path));
    SCadDocument native_loaded;
    QVERIFY(native_loaded.load(native_path));
    QVERIFY(std::get<SArcEntity>(native_loaded.entities().front().geometry).is_clockwise);

    const QString dxf_path = directory.filePath(QStringLiteral("clockwise.dxf"));
    const SDxfCodec codec;
    const SResult<SFileCompatibilityReport> write_result = codec.write(dxf_path, document);
    QVERIFY(write_result);
    QVERIFY(write_result.value().hasWarnings());
    QVERIFY(write_result.value().warnings.join(QLatin1Char('\n')).contains(
        QStringLiteral("方向信息已退化")));
    SCadDocument dxf_loaded;
    const SResult<SFileCompatibilityReport> read_result = codec.read(dxf_path, dxf_loaded);
    QVERIFY(read_result);
    const auto& loaded_arc = std::get<SArcEntity>(dxf_loaded.entities().front().geometry);
    QVERIFY(!loaded_arc.is_clockwise);
    QCOMPARE(loaded_arc.start_angle, 0.0);
    QCOMPARE(loaded_arc.end_angle, 90.0);
}

} // namespace smartCam

QTEST_MAIN(smartCam::SToolpathSortTest)
#include "s_toolpath_sort_test.moc"
