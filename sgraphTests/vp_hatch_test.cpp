#include "vp_cad_document.h"
#include "vp_cad_viewport.h"
#include "vp_document_transaction.h"
#include "vp_dxf_codec.h"
#include "vp_hatch_geometry.h"

#include <QTemporaryDir>
#include <QtTest>

namespace Vp
{

class VpHatchTest final : public QObject
{
    Q_OBJECT

  private slots:
    void geometryAndIslandValidation();
    void viewportCreationEditAndUndo();
    void nativeAndDxfRoundTrip();
};

VpHatchEntity patternedHatch()
{
    VpHatchEntity hatch;
    hatch.boundary = {{0.0, 0.0}, {20.0, 0.0}, {20.0, 20.0}, {0.0, 20.0}};
    hatch.island_boundaries = {{{5.0, 5.0}, {15.0, 5.0}, {15.0, 15.0}, {5.0, 15.0}}};
    hatch.fill_type = VpHatchFillType::Pattern;
    hatch.pattern_name = QStringLiteral("CROSS");
    hatch.pattern_scale = 2.0;
    hatch.pattern_angle = 30.0;
    hatch.gradient_start = QColor(QStringLiteral("#488eff"));
    hatch.gradient_end = QColor(QStringLiteral("#132d4d"));
    hatch.associative_boundary_id = 42;
    return hatch;
}

void VpHatchTest::geometryAndIslandValidation()
{
    const VpHatchEntity hatch = patternedHatch();
    QVERIFY(isHatchValid(hatch));
    QCOMPARE(hatchLoopArea(hatch.boundary), 400.0);
    QCOMPARE(hatchLoopArea(hatch.island_boundaries.front()), 100.0);
    QVERIFY(pointInsidePolygon({10.0, 10.0}, hatch.boundary));
    QVERIFY(!pointInsidePolygon({25.0, 10.0}, hatch.boundary));
}

void VpHatchTest::viewportCreationEditAndUndo()
{
    VpCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("boundaries"));
    const VpEntityId outer_id =
        transaction->addPolyline({{0.0, 0.0}, {20.0, 0.0}, {20.0, 20.0}, {0.0, 20.0}}, true);
    transaction->addPolyline({{5.0, 5.0}, {15.0, 5.0}, {15.0, 15.0}, {5.0, 15.0}}, true);
    transaction->commit();

    VpCadViewport viewport;
    viewport.setDocument(&document);
    VpHatchEntity settings = patternedHatch();
    settings.boundary.clear();
    settings.island_boundaries.clear();
    settings.associative_boundary_id = 0;
    viewport.setHatchSettings(settings);
    viewport.setToolMode(VpToolMode::Hatch);
    viewport.submitWorldPoint({0.0, 0.0});
    QCOMPARE(document.entities().size(), std::size_t(3));
    const auto& created_hatch = std::get<VpHatchEntity>(document.entities().back().geometry);
    QCOMPARE(created_hatch.associative_boundary_id, outer_id);
    QCOMPARE(created_hatch.island_boundaries.size(), std::size_t(1));
    QCOMPARE(created_hatch.fill_type, VpHatchFillType::Pattern);

    viewport.selectAll();
    VpHatchEntity gradient_settings = settings;
    gradient_settings.fill_type = VpHatchFillType::Gradient;
    gradient_settings.pattern_angle = 90.0;
    QVERIFY(viewport.editSelectedHatches(gradient_settings));
    QCOMPARE(std::get<VpHatchEntity>(document.entities().back().geometry).fill_type,
             VpHatchFillType::Gradient);
    document.undo();
    QCOMPARE(std::get<VpHatchEntity>(document.entities().back().geometry).fill_type,
             VpHatchFillType::Pattern);
}

void VpHatchTest::nativeAndDxfRoundTrip()
{
    QTemporaryDir temporary_directory;
    QVERIFY(temporary_directory.isValid());
    VpCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("hatch"));
    transaction->addHatch(patternedHatch());
    VpHatchEntity gradient = patternedHatch();
    gradient.fill_type = VpHatchFillType::Gradient;
    gradient.associative_boundary_id = 0;
    transaction->addHatch(std::move(gradient));
    transaction->commit();

    const QString native_path = temporary_directory.filePath(QStringLiteral("hatch.smartcad"));
    QVERIFY(document.save(native_path).isSuccess());
    VpCadDocument native_document;
    QVERIFY(native_document.load(native_path).isSuccess());
    QCOMPARE(native_document.entities().size(), std::size_t(2));
    const auto& native_hatch = std::get<VpHatchEntity>(native_document.entities().front().geometry);
    QCOMPARE(native_hatch.pattern_name, QStringLiteral("CROSS"));
    QCOMPARE(native_hatch.island_boundaries.size(), std::size_t(1));
    QCOMPARE(native_hatch.associative_boundary_id, VpEntityId(42));

    const QString dxf_path = temporary_directory.filePath(QStringLiteral("hatch.dxf"));
    VpDxfCodec codec;
    const auto write_result = codec.write(dxf_path, document);
    QVERIFY(write_result.isSuccess());
    QCOMPARE(write_result.value().exported_entity_count, 2);
    QCOMPARE(write_result.value().skipped_entity_count, 0);
    VpCadDocument dxf_document;
    const auto read_result = codec.read(dxf_path, dxf_document);
    QVERIFY(read_result.isSuccess());
    QCOMPARE(dxf_document.entities().size(), std::size_t(2));
    QCOMPARE(std::get<VpHatchEntity>(dxf_document.entities()[0].geometry).fill_type,
             VpHatchFillType::Pattern);
    QCOMPARE(std::get<VpHatchEntity>(dxf_document.entities()[1].geometry).fill_type,
             VpHatchFillType::Gradient);
}

} // namespace Vp

QTEST_MAIN(Vp::VpHatchTest)
#include "vp_hatch_test.moc"
