#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_document_transaction.h"
#include "s_dxf_codec.h"
#include "s_hatch_geometry.h"

#include <QTemporaryDir>
#include <QtTest>

namespace smartGraphics
{

class SHatchTest final : public QObject
{
    Q_OBJECT

  private slots:
    void geometryAndIslandValidation();
    void viewportCreationEditAndUndo();
    void nativeAndDxfRoundTrip();
};

SHatchEntity patternedHatch()
{
    SHatchEntity hatch;
    hatch.boundary = {{0.0, 0.0}, {20.0, 0.0}, {20.0, 20.0}, {0.0, 20.0}};
    hatch.island_boundaries = {{{5.0, 5.0}, {15.0, 5.0}, {15.0, 15.0}, {5.0, 15.0}}};
    hatch.fill_type = SHatchFillType::Pattern;
    hatch.pattern_name = QStringLiteral("CROSS");
    hatch.pattern_scale = 2.0;
    hatch.pattern_angle = 30.0;
    hatch.gradient_start = QColor(QStringLiteral("#488eff"));
    hatch.gradient_end = QColor(QStringLiteral("#132d4d"));
    hatch.associative_boundary_id = 42;
    return hatch;
}

void SHatchTest::geometryAndIslandValidation()
{
    const SHatchEntity hatch = patternedHatch();
    QVERIFY(isHatchValid(hatch));
    QCOMPARE(hatchLoopArea(hatch.boundary), 400.0);
    QCOMPARE(hatchLoopArea(hatch.island_boundaries.front()), 100.0);
    QVERIFY(pointInsidePolygon({10.0, 10.0}, hatch.boundary));
    QVERIFY(!pointInsidePolygon({25.0, 10.0}, hatch.boundary));
}

void SHatchTest::viewportCreationEditAndUndo()
{
    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("boundaries"));
    const SEntityId outer_id =
        transaction->addPolyline({{0.0, 0.0}, {20.0, 0.0}, {20.0, 20.0}, {0.0, 20.0}}, true);
    transaction->addPolyline({{5.0, 5.0}, {15.0, 5.0}, {15.0, 15.0}, {5.0, 15.0}}, true);
    transaction->commit();

    SCadViewport viewport;
    viewport.setDocument(&document);
    SHatchEntity settings = patternedHatch();
    settings.boundary.clear();
    settings.island_boundaries.clear();
    settings.associative_boundary_id = 0;
    viewport.setHatchSettings(settings);
    viewport.setToolMode(SToolMode::Hatch);
    viewport.submitWorldPoint({0.0, 0.0});
    QCOMPARE(document.entities().size(), std::size_t(3));
    const auto& created_hatch = std::get<SHatchEntity>(document.entities().back().geometry);
    QCOMPARE(created_hatch.associative_boundary_id, outer_id);
    QCOMPARE(created_hatch.island_boundaries.size(), std::size_t(1));
    QCOMPARE(created_hatch.fill_type, SHatchFillType::Pattern);

    viewport.selectAll();
    SHatchEntity gradient_settings = settings;
    gradient_settings.fill_type = SHatchFillType::Gradient;
    gradient_settings.pattern_angle = 90.0;
    QVERIFY(viewport.editSelectedHatches(gradient_settings));
    QCOMPARE(std::get<SHatchEntity>(document.entities().back().geometry).fill_type,
             SHatchFillType::Gradient);
    document.undo();
    QCOMPARE(std::get<SHatchEntity>(document.entities().back().geometry).fill_type,
             SHatchFillType::Pattern);
}

void SHatchTest::nativeAndDxfRoundTrip()
{
    QTemporaryDir temporary_directory;
    QVERIFY(temporary_directory.isValid());
    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("hatch"));
    transaction->addHatch(patternedHatch());
    SHatchEntity gradient = patternedHatch();
    gradient.fill_type = SHatchFillType::Gradient;
    gradient.associative_boundary_id = 0;
    transaction->addHatch(std::move(gradient));
    transaction->commit();

    const QString native_path = temporary_directory.filePath(QStringLiteral("hatch.smartcad"));
    QVERIFY(document.save(native_path).isSuccess());
    SCadDocument native_document;
    QVERIFY(native_document.load(native_path).isSuccess());
    QCOMPARE(native_document.entities().size(), std::size_t(2));
    const auto& native_hatch = std::get<SHatchEntity>(native_document.entities().front().geometry);
    QCOMPARE(native_hatch.pattern_name, QStringLiteral("CROSS"));
    QCOMPARE(native_hatch.island_boundaries.size(), std::size_t(1));
    QCOMPARE(native_hatch.associative_boundary_id, SEntityId(42));

    const QString dxf_path = temporary_directory.filePath(QStringLiteral("hatch.dxf"));
    SDxfCodec codec;
    const auto write_result = codec.write(dxf_path, document);
    QVERIFY(write_result.isSuccess());
    QCOMPARE(write_result.value().exported_entity_count, 2);
    QCOMPARE(write_result.value().skipped_entity_count, 0);
    SCadDocument dxf_document;
    const auto read_result = codec.read(dxf_path, dxf_document);
    QVERIFY(read_result.isSuccess());
    QCOMPARE(dxf_document.entities().size(), std::size_t(2));
    QCOMPARE(std::get<SHatchEntity>(dxf_document.entities()[0].geometry).fill_type,
             SHatchFillType::Pattern);
    QCOMPARE(std::get<SHatchEntity>(dxf_document.entities()[1].geometry).fill_type,
             SHatchFillType::Gradient);
}

} // namespace smartGraphics

QTEST_MAIN(smartGraphics::SHatchTest)
#include "s_hatch_test.moc"
