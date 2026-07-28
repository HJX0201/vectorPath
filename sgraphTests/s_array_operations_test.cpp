#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_cad_viewport_geometry.h"
#include "s_document_transaction.h"
#include "s_dxf_codec.h"

#include <QTemporaryDir>
#include <QtTest>
#include <algorithm>
#include <cmath>

namespace smartGraphics
{

class SArrayOperationsTest final : public QObject
{
    Q_OBJECT

  private slots:
    void rectangularArrayGeometryAndUndo();
    void polarArrayGeometryAndUndo();
    void pathArrayGeometryAndUndo();
    void associativeArrayPersistence();
};

void SArrayOperationsTest::rectangularArrayGeometryAndUndo()
{
    SEntityRecord source;
    source.id = 60;
    source.type = SEntityType::Line;
    source.layer_name = QStringLiteral("array");
    source.line_width_mm = 0.25;
    source.geometry = SLineEntity{{0.0, 0.0}, {2.0, 0.0}};
    const std::vector<SEntityRecord> copies = rectangularArrayEntities({source}, 10.0, 5.0, 3, 2);
    QCOMPARE(copies.size(), std::size_t(5));
    QCOMPARE(std::get<SLineEntity>(copies[0].geometry).start_point.x, 10.0);
    QCOMPARE(std::get<SLineEntity>(copies[1].geometry).start_point.x, 20.0);
    QCOMPARE(std::get<SLineEntity>(copies[2].geometry).start_point.y, 5.0);
    QCOMPARE(copies.back().layer_name, source.layer_name);
    QCOMPARE(copies.back().line_width_mm, source.line_width_mm);

    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("array source"));
    transaction->addLine({0.0, 0.0}, {2.0, 0.0});
    transaction->commit();
    SCadViewport viewport;
    viewport.setDocument(&document);
    viewport.selectAll();
    viewport.setRectangularArrayCounts(3, 2);
    viewport.setToolMode(SToolMode::ArrayRect);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({10.0, 5.0});
    QCOMPARE(document.entities().size(), std::size_t(6));
    const SArrayId array_id = document.entities().front().associative_array->array_id;
    QVERIFY(std::all_of(document.entities().begin(), document.entities().end(),
                        [array_id](const SEntityRecord& entity)
                        {
                            return entity.associative_array &&
                                   entity.associative_array->array_id == array_id;
                        }));
    viewport.setToolMode(SToolMode::ArrayEdit);
    QVERIFY(viewport.submitCommandKeyword(QStringLiteral("RECT 2 2 20 10")));
    QCOMPARE(document.entities().size(), std::size_t(4));
    QCOMPARE(document.entities().front().associative_array->column_count, 2);
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(6));
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(1));
}

void SArrayOperationsTest::polarArrayGeometryAndUndo()
{
    SEntityRecord source;
    source.id = 70;
    source.type = SEntityType::Line;
    source.layer_name = QStringLiteral("polar");
    source.line_width_mm = 0.4;
    source.geometry = SLineEntity{{10.0, 0.0}, {12.0, 0.0}};
    const std::vector<SEntityRecord> copies = polarArrayEntities({source}, {0.0, 0.0}, 4, 360.0);
    QCOMPARE(copies.size(), std::size_t(3));
    const auto& quarter_turn = std::get<SLineEntity>(copies.front().geometry);
    QVERIFY(std::abs(quarter_turn.start_point.x) < 1.0e-9);
    QCOMPARE(quarter_turn.start_point.y, 10.0);
    QCOMPARE(copies.back().layer_name, source.layer_name);
    QCOMPARE(copies.back().line_width_mm, source.line_width_mm);

    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("polar source"));
    transaction->addLine({10.0, 0.0}, {12.0, 0.0});
    transaction->commit();
    SCadViewport viewport;
    viewport.setDocument(&document);
    viewport.selectAll();
    viewport.setPolarArrayParameters(4, 360.0);
    viewport.setToolMode(SToolMode::ArrayPolar);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({1.0, 0.0});
    QCOMPARE(document.entities().size(), std::size_t(4));
    QVERIFY(viewport.editSelectedPolarArray(6, 180.0));
    QCOMPARE(document.entities().size(), std::size_t(6));
    QCOMPARE(document.entities().front().associative_array->item_count, 6);
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(4));
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(1));
}

void SArrayOperationsTest::pathArrayGeometryAndUndo()
{
    SEntityRecord source;
    source.id = 80;
    source.type = SEntityType::Line;
    source.layer_name = QStringLiteral("path_items");
    source.line_width_mm = 0.3;
    source.geometry = SLineEntity{{0.0, 0.0}, {2.0, 0.0}};
    SEntityRecord path;
    path.id = 81;
    path.type = SEntityType::Polyline;
    path.geometry = SPolylineEntity{{{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}}, false};
    const std::vector<SEntityRecord> items = pathArrayEntities({source}, path, {0.0, 0.0}, 3, true);
    QCOMPARE(items.size(), std::size_t(3));
    const auto& last_item = std::get<SLineEntity>(items.back().geometry);
    QCOMPARE(last_item.start_point.x, 10.0);
    QCOMPARE(last_item.start_point.y, 10.0);
    QVERIFY(std::abs(last_item.end_point.x - 10.0) < 1.0e-9);
    QCOMPARE(last_item.end_point.y, 12.0);
    QCOMPARE(items.back().layer_name, source.layer_name);

    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("path array"));
    const SEntityId source_id = transaction->addLine({0.0, 5.0}, {2.0, 5.0});
    transaction->addLine({0.0, 0.0}, {30.0, 0.0});
    transaction->commit();
    SCadViewport viewport;
    viewport.setDocument(&document);
    viewport.selectEntitiesInWindow({-1.0, 4.0}, {3.0, 6.0}, false);
    QCOMPARE(viewport.selectedEntityIds().size(), 1);
    viewport.setPathArrayParameters(4, false);
    viewport.setToolMode(SToolMode::ArrayPath);
    viewport.submitWorldPoint({0.0, 5.0});
    viewport.submitWorldPoint({15.0, 0.0});
    QCOMPARE(document.entities().size(), std::size_t(5));
    QVERIFY(viewport.editSelectedPathArray(6, true));
    QCOMPARE(document.entities().size(), std::size_t(7));
    QVERIFY(std::any_of(document.entities().begin(), document.entities().end(),
                        [](const SEntityRecord& entity)
                        {
                            return entity.associative_array.has_value();
                        }));
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(5));
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(2));
    const auto source_iterator =
        std::find_if(document.entities().begin(), document.entities().end(),
                     [source_id](const SEntityRecord& entity)
                     {
                         return entity.id == source_id;
                     });
    QVERIFY(source_iterator != document.entities().end());
    QCOMPARE(std::get<SLineEntity>(source_iterator->geometry).start_point.y, 5.0);
}

void SArrayOperationsTest::associativeArrayPersistence()
{
    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("persistent array source"));
    transaction->addCircle({2.0, 3.0}, 1.5);
    transaction->commit();
    SCadViewport viewport;
    viewport.setDocument(&document);
    viewport.selectAll();
    viewport.setRectangularArrayCounts(2, 2);
    viewport.setToolMode(SToolMode::ArrayRect);
    viewport.submitWorldPoint({2.0, 3.0});
    viewport.submitWorldPoint({12.0, 8.0});

    QTemporaryDir temporary_directory;
    QVERIFY(temporary_directory.isValid());
    const QString file_path = temporary_directory.filePath(QStringLiteral("array.smartcad"));
    QVERIFY(document.save(file_path).isSuccess());
    SCadDocument loaded_document;
    QVERIFY(loaded_document.load(file_path).isSuccess());
    QCOMPARE(loaded_document.entities().size(), std::size_t(4));
    QVERIFY(loaded_document.entities().front().associative_array.has_value());
    const SAssociativeArrayData& data = *loaded_document.entities().front().associative_array;
    QCOMPARE(data.array_type, SArrayType::Rectangular);
    QCOMPARE(data.column_count, 2);
    QCOMPARE(data.row_count, 2);
    QCOMPARE(data.source_type, SEntityType::Circle);
    QCOMPARE(std::get<SCircleEntity>(data.source_geometry).radius, 1.5);

    SDxfCodec codec;
    const QString dxf_path = temporary_directory.filePath(QStringLiteral("array.dxf"));
    const auto export_result = codec.write(dxf_path, loaded_document);
    QVERIFY(export_result.isSuccess());
    QVERIFY(export_result.value()
                .warnings.join(QLatin1Char('\n'))
                .contains(QStringLiteral("关联阵列")));
}

} // namespace smartGraphics

QTEST_MAIN(smartGraphics::SArrayOperationsTest)
#include "s_array_operations_test.moc"
