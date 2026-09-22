#include "vp_cad_document.h"
#include "vp_document_transaction.h"
#include "vp_dxf_codec.h"
#include "vp_test_runner.h"
#include "vp_toolpath.h"
#include "vp_toolpath_document.h"

#include <QTemporaryDir>
#include <QtTest>
#include <cmath>

namespace Vp
{

class VpToolpathSortTest final : public QObject
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

VpEntityRecord lineEntity(VpEntityId id, double start_x, double start_y, double end_x, double end_y)
{
    VpEntityRecord entity;
    entity.id = id;
    entity.type = VpEntityType::Line;
    entity.geometry = VpLineEntity{{start_x, start_y}, {end_x, end_y}};
    return entity;
}

std::vector<VpEntityId> ids(const VpToolpathSortResult& result)
{
    std::vector<VpEntityId> values;
    for (const VpEntityRecord& entity : result.entities)
    {
        values.push_back(entity.id);
    }
    return values;
}

VpEntityId addDocumentLine(VpCadDocument& document, const QString& layer_name, double start_x,
                           double end_x)
{
    document.setCurrentLayer(layer_name);
    Q_ASSERT(document.currentLayerName() == layer_name);
    std::unique_ptr<VpDocumentTransaction> transaction = document.beginTransaction("seed line");
    const VpEntityId entity_id = transaction->addLine({start_x, 0.0}, {end_x, 0.0});
    transaction->commit();
    return entity_id;
}

void VpToolpathSortTest::rowScanCombinations()
{
    const std::vector<VpEntityRecord> entities{
        lineEntity(1, 10.0, 10.0, 1000.0, 1000.0), lineEntity(2, 0.0, 10.0, -1000.0, -1000.0),
        lineEntity(3, 10.0, 0.0, 500.0, 500.0), lineEntity(4, 0.0, 0.0, -500.0, -500.0)};
    VpToolpathSortOptions options;
    QCOMPARE(ids(sortToolpathEntities(entities, options)), std::vector<VpEntityId>({2, 1, 4, 3}));
    options.horizontal = VpToolpathHorizontalDirection::RightToLeft;
    QCOMPARE(ids(sortToolpathEntities(entities, options)), std::vector<VpEntityId>({1, 2, 3, 4}));
    options.vertical = VpToolpathVerticalDirection::BottomToTop;
    QCOMPARE(ids(sortToolpathEntities(entities, options)), std::vector<VpEntityId>({3, 4, 1, 2}));
    options.horizontal = VpToolpathHorizontalDirection::LeftToRight;
    QCOMPARE(ids(sortToolpathEntities(entities, options)), std::vector<VpEntityId>({4, 3, 2, 1}));
}

void VpToolpathSortTest::rowToleranceUsesBaseline()
{
    const std::vector<VpEntityRecord> entities{
        lineEntity(1, 3.0, 1.01, 3.0, 2.0), lineEntity(2, 2.0, 0.9, 2.0, 1.0),
        lineEntity(3, 1.0, 0.0, 1.0, 1.0), lineEntity(4, 1000.0, 0.0, 1000.0, 1.0)};
    VpToolpathSortOptions options;
    options.vertical = VpToolpathVerticalDirection::BottomToTop;
    const std::vector<VpEntityId> sorted = ids(sortToolpathEntities(entities, options));
    QCOMPARE(sorted, std::vector<VpEntityId>({3, 2, 4, 1}));
}

void VpToolpathSortTest::sortingUsesStartPoint()
{
    const std::vector<VpEntityRecord> entities{lineEntity(1, 5.0, 0.0, -1000.0, 0.0),
                                               lineEntity(2, 1.0, 0.0, 1000.0, 0.0)};
    VpToolpathSortOptions options;
    QCOMPARE(ids(sortToolpathEntities(entities, options)), std::vector<VpEntityId>({2, 1}));
}

void VpToolpathSortTest::shortestReversesOnlySelectedCandidate()
{
    const std::vector<VpEntityRecord> entities{lineEntity(1, 10.0, 0.0, 2.0, 0.0),
                                               lineEntity(2, 20.0, 0.0, 11.0, 0.0)};
    VpToolpathSortOptions options;
    options.mode = VpToolpathSortMode::Shortest;
    options.allow_reverse = true;
    options.shortest_start = {0.0, 0.0};
    const VpToolpathSortResult result = sortToolpathEntities(entities, options);
    QCOMPARE(ids(result), std::vector<VpEntityId>({1, 2}));
    QCOMPARE(result.reversed_entity_ids, std::vector<VpEntityId>({1, 2}));
    QCOMPARE(toolpathStartPoint(result.entities[0]).x, 2.0);
    QCOMPARE(toolpathEndPoint(result.entities[0]).x, 10.0);
    QCOMPARE(toolpathStartPoint(result.entities[1]).x, 11.0);
}

void VpToolpathSortTest::sortsEachLayerAsACompleteBlock()
{
    VpCadDocument document;
    QVERIFY(document.addLayer(QStringLiteral("A")));
    QVERIFY(document.addLayer(QStringLiteral("B")));
    const VpEntityId a_right = addDocumentLine(document, QStringLiteral("A"), 10.0, 11.0);
    const VpEntityId b_left = addDocumentLine(document, QStringLiteral("B"), -100.0, -99.0);
    const VpEntityId a_left = addDocumentLine(document, QStringLiteral("A"), 0.0, 1.0);
    const VpEntityId b_right = addDocumentLine(document, QStringLiteral("B"), 100.0, 101.0);

    const VpToolpathDocumentSortResult result =
        sortDocumentToolpaths(document, {}, VpToolpathSortOptions{});

    QCOMPARE(result.sorted_entity_count, 4);
    QCOMPARE(document.entities()[0].id, a_left);
    QCOMPARE(document.entities()[1].id, a_right);
    QCOMPARE(document.entities()[2].id, b_left);
    QCOMPARE(document.entities()[3].id, b_right);
}

void VpToolpathSortTest::shortestContinuesFromPreviousLayerEnd()
{
    VpCadDocument document;
    QVERIFY(document.addLayer(QStringLiteral("A")));
    QVERIFY(document.addLayer(QStringLiteral("B")));
    const VpEntityId layer_a = addDocumentLine(document, QStringLiteral("A"), 10.0, 100.0);
    const VpEntityId b_near_origin = addDocumentLine(document, QStringLiteral("B"), 1.0, 2.0);
    const VpEntityId b_near_layer_end = addDocumentLine(document, QStringLiteral("B"), 90.0, 91.0);
    VpToolpathSortOptions options;
    options.mode = VpToolpathSortMode::Shortest;
    options.shortest_start = {0.0, 0.0};

    sortDocumentToolpaths(document, {}, options);

    QCOMPARE(document.entities()[0].id, layer_a);
    QCOMPARE(document.entities()[1].id, b_near_layer_end);
    QCOMPARE(document.entities()[2].id, b_near_origin);
}

void VpToolpathSortTest::geometryReversal()
{
    VpEntityRecord arc;
    arc.id = 1;
    arc.type = VpEntityType::Arc;
    arc.geometry = VpArcEntity{{0.0, 0.0}, 5.0, 10.0, 80.0, false};
    const auto reversed_arc = std::get<VpArcEntity>(reversedToolpathEntity(arc).geometry);
    QCOMPARE(reversed_arc.start_angle, 80.0);
    QCOMPARE(reversed_arc.end_angle, 10.0);
    QVERIFY(reversed_arc.is_clockwise);

    VpEntityRecord polyline;
    polyline.type = VpEntityType::Polyline;
    polyline.geometry = VpPolylineEntity{{{0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0}},
                                         false,
                                         {0.5, -0.25, 0.0},
                                         {1.0, 2.0, 0.0},
                                         {3.0, 4.0, 0.0}};
    const auto reversed_polyline =
        std::get<VpPolylineEntity>(reversedToolpathEntity(polyline).geometry);
    QCOMPARE(reversed_polyline.vertices.front().x, 2.0);
    QCOMPARE(reversed_polyline.bulges[0], 0.25);
    QCOMPARE(reversed_polyline.bulges[1], -0.5);
    QCOMPARE(reversed_polyline.start_widths[0], 4.0);
    QCOMPARE(reversed_polyline.end_widths[0], 2.0);
    const std::vector<VpToolpathMotion> motions = generateToolpathMotions({polyline}, {-1.0, 0.0});
    QVERIFY(motions.size() > 4);
    QCOMPARE(motions.front().type, VpToolpathMotionType::Rapid);
    QCOMPARE(motions.back().end_point.x, 2.0);
}

void VpToolpathSortTest::documentOrderUndoRedoAndPersistence()
{
    VpCadDocument document;
    std::unique_ptr<VpDocumentTransaction> transaction = document.beginTransaction("seed");
    const VpEntityId first_id = transaction->addLine({10.0, 0.0}, {20.0, 0.0});
    const VpEntityId fixed_id = transaction->addLine({5.0, 0.0}, {6.0, 0.0});
    const VpEntityId third_id = transaction->addLine({0.0, 0.0}, {1.0, 0.0});
    transaction->commit();
    VpToolpathSortOptions options;
    const VpToolpathDocumentSortResult sort_result =
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
    VpCadDocument loaded;
    QVERIFY(loaded.load(path));
    QCOMPARE(loaded.entities()[0].id, third_id);
    QCOMPARE(loaded.entities()[1].id, fixed_id);
    QCOMPARE(loaded.entities()[2].id, first_id);
}

void VpToolpathSortTest::clockwiseArcPersistenceAndDxfDegradation()
{
    VpCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("clockwise arc"));
    VpEntityRecord arc;
    arc.type = VpEntityType::Arc;
    arc.geometry = VpArcEntity{{0.0, 0.0}, 10.0, 90.0, 0.0, true};
    transaction->addEntityCopy(arc);
    transaction->commit();
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString native_path = directory.filePath(QStringLiteral("clockwise.smcad"));
    QVERIFY(document.save(native_path));
    VpCadDocument native_loaded;
    QVERIFY(native_loaded.load(native_path));
    QVERIFY(std::get<VpArcEntity>(native_loaded.entities().front().geometry).is_clockwise);

    const QString dxf_path = directory.filePath(QStringLiteral("clockwise.dxf"));
    const VpDxfCodec codec;
    const VpResult<VpFileCompatibilityReport> write_result = codec.write(dxf_path, document);
    QVERIFY(write_result);
    QVERIFY(write_result.value().hasWarnings());
    QVERIFY(write_result.value()
                .warnings.join(QLatin1Char('\n'))
                .contains(QStringLiteral("方向信息已退化")));
    VpCadDocument dxf_loaded;
    const VpResult<VpFileCompatibilityReport> read_result = codec.read(dxf_path, dxf_loaded);
    QVERIFY(read_result);
    const auto& loaded_arc = std::get<VpArcEntity>(dxf_loaded.entities().front().geometry);
    QVERIFY(!loaded_arc.is_clockwise);
    QCOMPARE(loaded_arc.start_angle, 0.0);
    QCOMPARE(loaded_arc.end_angle, 90.0);
}

} // namespace Vp

VECTORPATH_TEST_ENTRY(Vp::VpToolpathSortTest, vpRunVpToolpathSortTest)
#include "vp_toolpath_sort_test.moc"
