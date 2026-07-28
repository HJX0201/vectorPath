#include "s_cad_document.h"
#include "s_document_transaction.h"
#include "s_dxf_codec.h"

#include <QDataStream>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtTest>

namespace smartGraphics
{
namespace
{

bool writeLegacyV6File(const QString& file_path)
{
    QFile file(file_path);
    if (!file.open(QIODevice::WriteOnly))
    {
        return false;
    }
    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.setVersion(QDataStream::Qt_5_12);
    stream.writeRawData("SMCAD001", 8);
    QJsonObject manifest;
    manifest.insert(QStringLiteral("formatVersion"), 6);
    const QByteArray manifest_data = QJsonDocument(manifest).toJson(QJsonDocument::Compact);
    stream << static_cast<quint32>(manifest_data.size());
    stream.writeRawData(manifest_data.constData(), manifest_data.size());
    stream << static_cast<quint32>(2);
    stream << QStringLiteral("0") << static_cast<quint32>(QColor(20, 40, 60).rgba()) << 0.25 << true
           << false << true;
    stream << QStringLiteral("DETAIL") << static_cast<quint32>(QColor(80, 100, 120).rgba()) << 0.35
           << true << false << true;
    stream << QStringLiteral("DETAIL");
    stream << static_cast<quint64>(1);
    stream << static_cast<quint64>(7) << static_cast<quint8>(SEntityType::Line)
           << static_cast<quint32>(QColor(255, 0, 0).rgba()) << QStringLiteral("DETAIL") << -1.0;
    stream << 1.0 << 2.0 << 3.0 << 4.0;
    return stream.status() == QDataStream::Ok;
}

} // namespace

class SCadDocumentTest final : public QObject
{
    Q_OBJECT

  private slots:
    void transactionUndoRedo();
    void entityCopyUndoRedo();
    void nativeFormatRoundTrip();
    void dxfRoundTrip();
    void modificationUndoRedo();
    void layerStateAndRemovalRules();
    void entityLayerTransactionUndoRedo();
    void layerColorBindingAndStableIds();
    void layerColorPreviewDoesNotModify();
    void legacyV6ColorMigration();
    void layerManagementUndoRedo();
    void drawingSettingsUndoRedoAndFormatting();
};

void SCadDocumentTest::transactionUndoRedo()
{
    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("line"));
    transaction->addLine({1.0, 2.0}, {30.0, 40.0});
    transaction->commit();

    QCOMPARE(document.entities().size(), std::size_t(1));
    QVERIFY(document.canUndo());
    document.undo();
    QVERIFY(document.entities().empty());
    QVERIFY(document.canRedo());
    document.redo();
    QCOMPARE(document.entities().size(), std::size_t(1));
}

void SCadDocumentTest::entityCopyUndoRedo()
{
    SCadDocument document;
    auto create_transaction = document.beginTransaction(QStringLiteral("source"));
    create_transaction->addCircle({2.0, 3.0}, 4.0);
    create_transaction->commit();
    const SEntityId source_id = document.entities().front().id;

    auto copy_transaction = document.beginTransaction(QStringLiteral("copy"));
    const SEntityId copy_id = copy_transaction->addEntityCopy(document.entities().front());
    copy_transaction->commit();

    QCOMPARE(document.entities().size(), std::size_t(2));
    QVERIFY(copy_id != source_id);
    QCOMPARE(document.entities().at(1).layer_name, document.entities().front().layer_name);
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(1));
    QCOMPARE(document.entities().front().id, source_id);
}

void SCadDocumentTest::nativeFormatRoundTrip()
{
    QTemporaryDir temporary_directory;
    QVERIFY(temporary_directory.isValid());
    const QString file_path = temporary_directory.filePath(QStringLiteral("round_trip.smartcad"));

    SCadDocument source_document;
    QVERIFY(source_document.addLayer(QStringLiteral("ANNOTATION")));
    QVERIFY(source_document.addLayer(QStringLiteral("FROZEN")));
    QVERIFY(source_document.setLayerFrozen(QStringLiteral("FROZEN"), true));
    QVERIFY(source_document.setLayerColor(QStringLiteral("ANNOTATION"), QColor(12, 34, 56)));
    QVERIFY(source_document.setLayerLineWidth(QStringLiteral("ANNOTATION"), 0.70));
    QVERIFY(
        source_document.setLayerLineType(QStringLiteral("ANNOTATION"), QStringLiteral("Dashed")));
    QVERIFY(source_document.setLayerTransparency(QStringLiteral("ANNOTATION"), 35));
    QVERIFY(source_document.setLayerVisible(QStringLiteral("ANNOTATION"), false));
    QVERIFY(source_document.setLayerLocked(QStringLiteral("ANNOTATION"), true));
    QVERIFY(source_document.setLayerPlottable(QStringLiteral("ANNOTATION"), false));
    QVERIFY(source_document.setCurrentLayer(QStringLiteral("ANNOTATION")));
    SDrawingSettings source_settings;
    source_settings.insertion_unit = SInsertionUnit::Inches;
    source_settings.angle_format = SAngleFormat::DegreesMinutesSeconds;
    source_settings.linear_precision = 4;
    source_settings.angular_precision = 1;
    auto settings_transaction = source_document.beginTransaction(QStringLiteral("settings"));
    QVERIFY(settings_transaction->setDrawingSettings(source_settings));
    settings_transaction->commit();
    auto transaction = source_document.beginTransaction(QStringLiteral("geometry"));
    const SEntityId line_id = transaction->addLine({-10.0, 5.0}, {20.0, 25.0});
    QVERIFY(transaction->setEntityLineWidth(line_id, 0.50));
    transaction->addCircle({3.0, 7.0}, 12.5);
    transaction->addArc({5.0, 6.0}, 8.0, 15.0, 145.0);
    transaction->addPolyline({{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}}, true);
    transaction->addText({2.0, 3.0}, QStringLiteral("smartCad"));
    transaction->addLinearDimension({0.0, 0.0}, {10.0, 0.0}, {0.0, 3.0});
    transaction->addHatch({{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}});
    transaction->commit();
    QVERIFY(source_document.saveLayerState(QStringLiteral("PLOT_READY")));
    const SResult<void> save_result = source_document.save(file_path);
    QVERIFY2(save_result.isSuccess(), qPrintable(save_result.errorMessage()));

    SCadDocument loaded_document;
    const SResult<void> load_result = loaded_document.load(file_path);
    QVERIFY2(load_result.isSuccess(), qPrintable(load_result.errorMessage()));
    QCOMPARE(loaded_document.entities().size(), std::size_t(7));
    QCOMPARE(loaded_document.entities().at(0).type, SEntityType::Line);
    QCOMPARE(loaded_document.entities().at(1).type, SEntityType::Circle);
    QCOMPARE(loaded_document.entities().at(2).type, SEntityType::Arc);
    QCOMPARE(loaded_document.entities().at(3).type, SEntityType::Polyline);
    QCOMPARE(loaded_document.entities().at(4).type, SEntityType::Text);
    QCOMPARE(loaded_document.entities().at(5).type, SEntityType::LinearDimension);
    QCOMPARE(loaded_document.entities().at(6).type, SEntityType::Hatch);
    QCOMPARE(loaded_document.currentLayerName(), QStringLiteral("ANNOTATION"));
    QCOMPARE(loaded_document.entities().at(0).layer_name, QStringLiteral("ANNOTATION"));
    QCOMPARE(loaded_document.layerColor(QStringLiteral("ANNOTATION")), QColor(12, 34, 56));
    QCOMPARE(loaded_document.layerLineWidth(QStringLiteral("ANNOTATION")), 0.70);
    QCOMPARE(loaded_document.layerLineType(QStringLiteral("ANNOTATION")), QStringLiteral("Dashed"));
    QCOMPARE(loaded_document.layerTransparency(QStringLiteral("ANNOTATION")), 35);
    QCOMPARE(loaded_document.entities().at(0).line_width_mm, 0.50);
    QVERIFY(!loaded_document.isLayerVisible(QStringLiteral("ANNOTATION")));
    QVERIFY(loaded_document.isLayerLocked(QStringLiteral("ANNOTATION")));
    QVERIFY(!loaded_document.layer(QStringLiteral("ANNOTATION"))->is_plottable);
    QVERIFY(loaded_document.isLayerFrozen(QStringLiteral("FROZEN")));
    QVERIFY(loaded_document.layerStateNames().contains(QStringLiteral("PLOT_READY")));
    QVERIFY(loaded_document.drawingSettings() == source_settings);
}

void SCadDocumentTest::dxfRoundTrip()
{
    QTemporaryDir temporary_directory;
    QVERIFY(temporary_directory.isValid());
    const QString file_path = temporary_directory.filePath(QStringLiteral("round_trip.dxf"));

    SCadDocument source_document;
    QVERIFY(source_document.addLayer(QStringLiteral("GEOMETRY")));
    QVERIFY(source_document.setCurrentLayer(QStringLiteral("GEOMETRY")));
    auto transaction = source_document.beginTransaction(QStringLiteral("dxf geometry"));
    const SEntityId dxf_line_id = transaction->addLine({-12.0, 8.0}, {33.0, 41.0});
    QVERIFY(transaction->setEntityLineWidth(dxf_line_id, 0.50));
    transaction->addCircle({4.0, 9.0}, 17.5);
    transaction->addArc({5.0, 7.0}, 9.0, 0.0, 90.0);
    transaction->addPolyline({{0.0, 0.0}, {12.0, 0.0}, {12.0, 6.0}}, false);
    transaction->addText({3.0, 4.0}, QStringLiteral("DXF text"));
    transaction->commit();

    const SDxfCodec codec;
    const SResult<SFileCompatibilityReport> write_result = codec.write(file_path, source_document);
    QVERIFY2(write_result.isSuccess(), qPrintable(write_result.errorMessage()));
    QCOMPARE(write_result.value().exported_entity_count, std::size_t(5));

    SCadDocument loaded_document;
    const SResult<SFileCompatibilityReport> read_result = codec.read(file_path, loaded_document);
    QVERIFY2(read_result.isSuccess(), qPrintable(read_result.errorMessage()));
    QCOMPARE(read_result.value().imported_entity_count, std::size_t(5));
    QCOMPARE(loaded_document.entities().size(), std::size_t(5));
    QCOMPARE(loaded_document.entities().front().layer_name, QStringLiteral("GEOMETRY"));
    QCOMPARE(loaded_document.entities().front().line_width_mm, 0.50);
}

void SCadDocumentTest::modificationUndoRedo()
{
    SCadDocument document;
    auto create_transaction = document.beginTransaction(QStringLiteral("create"));
    const SEntityId entity_id = create_transaction->addLine({0.0, 0.0}, {10.0, 0.0});
    create_transaction->commit();

    SEntityRecord replacement = document.entities().front();
    replacement.geometry = SLineEntity{{5.0, 5.0}, {15.0, 5.0}};
    auto modify_transaction = document.beginTransaction(QStringLiteral("move"));
    QVERIFY(modify_transaction->replaceEntity(entity_id, replacement));
    modify_transaction->commit();
    QCOMPARE(std::get<SLineEntity>(document.entities().front().geometry).start_point.x, 5.0);

    document.undo();
    QCOMPARE(std::get<SLineEntity>(document.entities().front().geometry).start_point.x, 0.0);
    document.redo();
    QCOMPARE(std::get<SLineEntity>(document.entities().front().geometry).start_point.x, 5.0);

    auto remove_transaction = document.beginTransaction(QStringLiteral("erase"));
    QVERIFY(remove_transaction->removeEntity(entity_id));
    remove_transaction->commit();
    QVERIFY(document.entities().empty());
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(1));
}

void SCadDocumentTest::layerStateAndRemovalRules()
{
    SCadDocument document;
    QCOMPARE(document.layerNames(), QStringList{QStringLiteral("0")});
    QVERIFY(document.layer(QStringLiteral("DEFPOINTS")) == nullptr);
    QVERIFY(document.addLayer(QStringLiteral("WALLS")));
    QVERIFY(document.setLayerColor(QStringLiteral("WALLS"), QColor(10, 200, 30)));
    QVERIFY(document.setLayerLineWidth(QStringLiteral("WALLS"), 0.53));
    QVERIFY(document.setLayerVisible(QStringLiteral("WALLS"), false));
    QVERIFY(document.setLayerLocked(QStringLiteral("WALLS"), true));
    QVERIFY(document.setLayerPlottable(QStringLiteral("WALLS"), false));
    QCOMPARE(document.layerColor(QStringLiteral("WALLS")), QColor(10, 200, 30));
    QCOMPARE(document.layerLineWidth(QStringLiteral("WALLS")), 0.53);
    QVERIFY(!document.isLayerVisible(QStringLiteral("WALLS")));
    QVERIFY(document.isLayerLocked(QStringLiteral("WALLS")));
    QVERIFY(!document.layer(QStringLiteral("WALLS"))->is_plottable);

    QVERIFY(!document.removeLayer(QStringLiteral("0")));
    auto dimension_transaction = document.beginTransaction(QStringLiteral("dimension"));
    dimension_transaction->addLinearDimension({0.0, 0.0}, {10.0, 0.0}, {0.0, 5.0});
    dimension_transaction->commit();
    QVERIFY(document.layer(QStringLiteral("DEFPOINTS")) != nullptr);
    QVERIFY(!document.layer(QStringLiteral("DEFPOINTS"))->is_plottable);
    QVERIFY(!document.removeLayer(QStringLiteral("DEFPOINTS")));
    QVERIFY(document.setLayerLocked(QStringLiteral("WALLS"), false));
    QVERIFY(document.removeLayer(QStringLiteral("WALLS")));
    QVERIFY(document.layer(QStringLiteral("WALLS")) == nullptr);
}

void SCadDocumentTest::entityLayerTransactionUndoRedo()
{
    SCadDocument document;
    QVERIFY(document.addLayer(QStringLiteral("TARGET")));
    auto create_transaction = document.beginTransaction(QStringLiteral("create"));
    const SEntityId first_id = create_transaction->addLine({0.0, 0.0}, {10.0, 0.0});
    const SEntityId second_id = create_transaction->addCircle({5.0, 5.0}, 2.0);
    create_transaction->commit();

    auto layer_transaction = document.beginTransaction(QStringLiteral("change layer"));
    QVERIFY(layer_transaction->setEntityLayer(first_id, QStringLiteral("TARGET")));
    QVERIFY(layer_transaction->setEntityLayer(second_id, QStringLiteral("TARGET")));
    QVERIFY(layer_transaction->setEntityLineWidth(first_id, 0.80));
    QVERIFY(layer_transaction->setEntityLineWidth(second_id, 0.80));
    layer_transaction->commit();
    QCOMPARE(document.entities().at(0).layer_name, QStringLiteral("TARGET"));
    QCOMPARE(document.entities().at(1).layer_name, QStringLiteral("TARGET"));
    QCOMPARE(document.entities().at(0).line_width_mm, 0.80);
    QCOMPARE(document.entities().at(1).line_width_mm, 0.80);
    QVERIFY(!document.removeLayer(QStringLiteral("TARGET")));

    document.undo();
    QCOMPARE(document.entities().at(0).layer_name, QStringLiteral("0"));
    QCOMPARE(document.entities().at(1).layer_name, QStringLiteral("0"));
    QCOMPARE(document.entities().at(0).line_width_mm, -1.0);
    QCOMPARE(document.entities().at(1).line_width_mm, -1.0);
    document.redo();
    QCOMPARE(document.entities().at(0).layer_name, QStringLiteral("TARGET"));
    QCOMPARE(document.entities().at(1).layer_name, QStringLiteral("TARGET"));
    QCOMPARE(document.entities().at(0).line_width_mm, 0.80);
    QCOMPARE(document.entities().at(1).line_width_mm, 0.80);
}

void SCadDocumentTest::layerColorBindingAndStableIds()
{
    QTemporaryDir temporary_directory;
    QVERIFY(temporary_directory.isValid());
    const QString file_path = temporary_directory.filePath(QStringLiteral("layers.smartcad"));
    const QColor shared_color(35, 145, 220);

    SCadDocument source_document;
    QCOMPARE(source_document.layer(QStringLiteral("0"))->id, SLayerId(0));
    QVERIFY(source_document.layer(QStringLiteral("DEFPOINTS")) == nullptr);
    QVERIFY(source_document.addLayer(QStringLiteral("WALLS")));
    QVERIFY(source_document.addLayer(QStringLiteral("DOORS")));
    QVERIFY(source_document.setLayerColor(QStringLiteral("WALLS"), shared_color));
    QVERIFY(source_document.setLayerColor(QStringLiteral("DOORS"), shared_color));
    QCOMPARE(source_document.layer(QStringLiteral("WALLS"))->id, SLayerId(1));
    QCOMPARE(source_document.layer(QStringLiteral("DOORS"))->id, SLayerId(2));
    QVERIFY(source_document.layer(QStringLiteral("WALLS"))->id !=
            source_document.layer(QStringLiteral("DOORS"))->id);
    QCOMPARE(source_document.layerColor(QStringLiteral("WALLS")), shared_color);
    QCOMPARE(source_document.layerColor(QStringLiteral("DOORS")), shared_color);
    QVERIFY(source_document.save(file_path).isSuccess());

    SCadDocument loaded_document;
    QVERIFY(loaded_document.load(file_path).isSuccess());
    QCOMPARE(loaded_document.layer(QStringLiteral("WALLS"))->id, SLayerId(1));
    QCOMPARE(loaded_document.layer(QStringLiteral("DOORS"))->id, SLayerId(2));
    QCOMPARE(loaded_document.layerColor(QStringLiteral("WALLS")), shared_color);
    QCOMPARE(loaded_document.layerColor(QStringLiteral("DOORS")), shared_color);
    QVERIFY(loaded_document.addLayer(QStringLiteral("WINDOWS")));
    QCOMPARE(loaded_document.layer(QStringLiteral("WINDOWS"))->id, SLayerId(3));
}

void SCadDocumentTest::layerColorPreviewDoesNotModify()
{
    SCadDocument document;
    const QColor original_color = document.layerColor(QStringLiteral("0"));
    const QColor preview_color(180, 70, 220);

    QVERIFY(document.previewLayerColor(QStringLiteral("0"), preview_color));
    QCOMPARE(document.layerColor(QStringLiteral("0")), preview_color);
    QVERIFY(!document.isModified());
    QVERIFY(document.previewLayerColor(QStringLiteral("0"), original_color));
    QVERIFY(!document.isModified());
    QVERIFY(document.setLayerColor(QStringLiteral("0"), preview_color));
    QVERIFY(document.isModified());
}

void SCadDocumentTest::legacyV6ColorMigration()
{
    QTemporaryDir temporary_directory;
    QVERIFY(temporary_directory.isValid());
    const QString file_path = temporary_directory.filePath(QStringLiteral("legacy_v6.smartcad"));
    QVERIFY(writeLegacyV6File(file_path));

    SCadDocument document;
    const SResult<void> load_result = document.load(file_path);
    QVERIFY2(load_result.isSuccess(), qPrintable(load_result.errorMessage()));
    QCOMPARE(document.entities().size(), std::size_t(1));
    QCOMPARE(document.entities().front().layer_name, QStringLiteral("DETAIL"));
    QCOMPARE(document.layer(QStringLiteral("0"))->id, SLayerId(0));
    QCOMPARE(document.layer(QStringLiteral("DETAIL"))->id, SLayerId(1));
    QCOMPARE(document.layerColor(QStringLiteral("DETAIL")), QColor(80, 100, 120));
    QVERIFY(document.addLayer(QStringLiteral("NEW")));
    QCOMPARE(document.layer(QStringLiteral("NEW"))->id, SLayerId(2));
}

void SCadDocumentTest::layerManagementUndoRedo()
{
    SCadDocument document;
    QVERIFY(document.addLayer(QStringLiteral("SOURCE")));
    QVERIFY(document.addLayer(QStringLiteral("TARGET")));
    QVERIFY(document.setCurrentLayer(QStringLiteral("SOURCE")));
    auto transaction = document.beginTransaction(QStringLiteral("layer entity"));
    transaction->addLine({0.0, 0.0}, {10.0, 0.0});
    transaction->commit();

    QVERIFY(document.renameLayer(QStringLiteral("SOURCE"), QStringLiteral("RENAMED")));
    QVERIFY(!document.layer(QStringLiteral("SOURCE")));
    QVERIFY(document.layer(QStringLiteral("RENAMED")));
    QCOMPARE(document.entities().front().layer_name, QStringLiteral("RENAMED"));
    QCOMPARE(document.currentLayerName(), QStringLiteral("RENAMED"));
    document.undo();
    QVERIFY(document.layer(QStringLiteral("SOURCE")));
    QCOMPARE(document.entities().front().layer_name, QStringLiteral("SOURCE"));
    QCOMPARE(document.currentLayerName(), QStringLiteral("SOURCE"));
    document.redo();

    QVERIFY(document.mergeLayer(QStringLiteral("RENAMED"), QStringLiteral("TARGET")));
    QVERIFY(!document.layer(QStringLiteral("RENAMED")));
    QCOMPARE(document.entities().front().layer_name, QStringLiteral("TARGET"));
    QCOMPARE(document.currentLayerName(), QStringLiteral("TARGET"));
    document.undo();
    QVERIFY(document.layer(QStringLiteral("RENAMED")));
    QCOMPARE(document.entities().front().layer_name, QStringLiteral("RENAMED"));

    QVERIFY(document.removeLayerWithContents(QStringLiteral("RENAMED")));
    QVERIFY(!document.layer(QStringLiteral("RENAMED")));
    QVERIFY(document.entities().empty());
    QCOMPARE(document.currentLayerName(), QStringLiteral("0"));
    document.undo();
    QVERIFY(document.layer(QStringLiteral("RENAMED")));
    QCOMPARE(document.entities().size(), std::size_t(1));
    QCOMPARE(document.currentLayerName(), QStringLiteral("RENAMED"));

    QVERIFY(document.setLayerFrozen(QStringLiteral("TARGET"), true));
    QVERIFY(document.isLayerFrozen(QStringLiteral("TARGET")));
    QVERIFY(!document.isLayerVisible(QStringLiteral("TARGET")));
    document.undo();
    QVERIFY(!document.isLayerFrozen(QStringLiteral("TARGET")));
    QVERIFY(document.isolateLayer(QStringLiteral("TARGET")));
    QVERIFY(document.layer(QStringLiteral("TARGET"))->is_visible);
    QVERIFY(!document.layer(QStringLiteral("RENAMED"))->is_visible);
    document.undo();
    QVERIFY(document.layer(QStringLiteral("RENAMED"))->is_visible);

    QVERIFY(document.setLayerLineType(QStringLiteral("TARGET"), QStringLiteral("Center")));
    QVERIFY(document.setLayerTransparency(QStringLiteral("TARGET"), 40));
    QVERIFY(document.saveLayerState(QStringLiteral("BASELINE")));
    QVERIFY(document.setLayerVisible(QStringLiteral("TARGET"), false));
    QVERIFY(document.setLayerLineType(QStringLiteral("TARGET"), QStringLiteral("Hidden")));
    QVERIFY(document.setLayerTransparency(QStringLiteral("TARGET"), 70));
    QVERIFY(!document.layer(QStringLiteral("TARGET"))->is_visible);
    QVERIFY(document.restoreLayerState(QStringLiteral("BASELINE")));
    QVERIFY(document.layer(QStringLiteral("TARGET"))->is_visible);
    QCOMPARE(document.layerLineType(QStringLiteral("TARGET")), QStringLiteral("Center"));
    QCOMPARE(document.layerTransparency(QStringLiteral("TARGET")), 40);
    document.undo();
    QVERIFY(!document.layer(QStringLiteral("TARGET"))->is_visible);
    QVERIFY(document.removeLayerState(QStringLiteral("BASELINE")));
    QVERIFY(!document.layerStateNames().contains(QStringLiteral("BASELINE")));
}

void SCadDocumentTest::drawingSettingsUndoRedoAndFormatting()
{
    SCadDocument document;
    SDrawingSettings settings = document.drawingSettings();
    settings.insertion_unit = SInsertionUnit::Meters;
    settings.angle_format = SAngleFormat::Radians;
    settings.linear_precision = 2;
    settings.angular_precision = 4;
    auto transaction = document.beginTransaction(QStringLiteral("drawing settings"));
    QVERIFY(transaction->setDrawingSettings(settings));
    transaction->commit();
    QVERIFY(document.drawingSettings() == settings);
    QCOMPARE(formatLinearValue(12.345, settings), QStringLiteral("12.35 m"));
    QCOMPARE(formatAngleValue(180.0, settings), QStringLiteral("3.1416 rad"));

    document.undo();
    QCOMPARE(document.drawingSettings().insertion_unit, SInsertionUnit::Millimeters);
    QCOMPARE(document.drawingSettings().angle_format, SAngleFormat::DecimalDegrees);
    document.redo();
    QVERIFY(document.drawingSettings() == settings);

    settings.angle_format = SAngleFormat::DegreesMinutesSeconds;
    settings.angular_precision = 1;
    QCOMPARE(formatAngleValue(37.5125, settings), QStringLiteral("37°30′45.0″"));
}

} // namespace smartGraphics

QTEST_APPLESS_MAIN(smartGraphics::SCadDocumentTest)
#include "s_cad_document_test.moc"
