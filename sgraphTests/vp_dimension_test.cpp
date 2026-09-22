#include "vp_cad_document.h"
#include "vp_cad_viewport.h"
#include "vp_cad_viewport_geometry.h"
#include "vp_dimension_geometry.h"
#include "vp_document_transaction.h"
#include "vp_dxf_codec.h"

#include <QTemporaryDir>
#include <QtTest>
#include <algorithm>
#include <cmath>

namespace Vp
{

class VpDimensionTest final : public QObject
{
    Q_OBJECT

  private slots:
    void measurementsCoverAllTypes();
    void viewportCreatesAllTypesWithUndo();
    void smartcadAndDxfRoundTrip();
    void dimensionStyleUndoAndPersistence();
    void explodeAllDimensionTypes();
};

VpLinearDimensionEntity makeDimension(VpDimensionType dimension_type)
{
    VpLinearDimensionEntity dimension;
    dimension.dimension_type = dimension_type;
    dimension.first_point = {10.0, 0.0};
    dimension.second_point = {0.0, 10.0};
    dimension.dimension_line_point = {7.0, 7.0};
    dimension.center_point = {0.0, 0.0};
    dimension.style_name = QStringLiteral("Standard");
    return dimension;
}

void VpDimensionTest::measurementsCoverAllTypes()
{
    VpLinearDimensionEntity linear = makeDimension(VpDimensionType::Linear);
    linear.first_point = {0.0, 0.0};
    linear.second_point = {12.0, 5.0};
    linear.dimension_line_point = {6.0, 8.0};
    QCOMPARE(dimensionMeasurement(linear), 12.0);

    VpLinearDimensionEntity aligned = linear;
    aligned.dimension_type = VpDimensionType::Aligned;
    QCOMPARE(dimensionMeasurement(aligned), 13.0);

    const VpLinearDimensionEntity angular = makeDimension(VpDimensionType::Angular);
    QCOMPARE(dimensionMeasurement(angular), 90.0);
    QVERIFY(dimensionDefaultText(angular).endsWith(QChar(0x00B0)));

    const VpLinearDimensionEntity radius = makeDimension(VpDimensionType::Radius);
    QCOMPARE(dimensionMeasurement(radius), 10.0);
    QVERIFY(dimensionDefaultText(radius).startsWith(QLatin1Char('R')));

    const VpLinearDimensionEntity diameter = makeDimension(VpDimensionType::Diameter);
    QCOMPARE(dimensionMeasurement(diameter), 20.0);

    const VpLinearDimensionEntity arc_length = makeDimension(VpDimensionType::ArcLength);
    QVERIFY(std::abs(dimensionMeasurement(arc_length) - 15.7079632679) < 1.0e-8);

    VpLinearDimensionEntity ordinate = makeDimension(VpDimensionType::Ordinate);
    ordinate.first_point = {4.0, 5.0};
    ordinate.dimension_line_point = {12.0, 5.0};
    QCOMPARE(dimensionMeasurement(ordinate), 5.0);
}

void VpDimensionTest::viewportCreatesAllTypesWithUndo()
{
    VpCadDocument document;
    VpCadViewport viewport;
    viewport.setDocument(&document);

    viewport.setToolMode(VpToolMode::LinearDimension);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({10.0, 0.0});
    viewport.submitWorldPoint({0.0, 3.0});

    viewport.setToolMode(VpToolMode::AlignedDimension);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({6.0, 8.0});
    viewport.submitWorldPoint({3.0, 7.0});

    viewport.setToolMode(VpToolMode::AngularDimension);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({10.0, 0.0});
    viewport.submitWorldPoint({0.0, 10.0});
    viewport.submitWorldPoint({7.0, 7.0});

    viewport.setToolMode(VpToolMode::RadiusDimension);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({5.0, 0.0});
    viewport.submitWorldPoint({8.0, 0.0});

    viewport.setToolMode(VpToolMode::DiameterDimension);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({5.0, 0.0});
    viewport.submitWorldPoint({8.0, 0.0});

    viewport.setToolMode(VpToolMode::ArcLengthDimension);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({10.0, 0.0});
    viewport.submitWorldPoint({0.0, 10.0});
    viewport.submitWorldPoint({7.0, 7.0});

    viewport.setToolMode(VpToolMode::OrdinateDimension);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({4.0, 5.0});
    viewport.submitWorldPoint({12.0, 5.0});

    QCOMPARE(document.entities().size(), std::size_t(7));
    for (std::size_t index = 0; index < document.entities().size(); ++index)
    {
        const auto& dimension =
            std::get<VpLinearDimensionEntity>(document.entities()[index].geometry);
        QCOMPARE(static_cast<int>(dimension.dimension_type), static_cast<int>(index));
    }
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(6));
    document.redo();
    QCOMPARE(document.entities().size(), std::size_t(7));
}

void VpDimensionTest::smartcadAndDxfRoundTrip()
{
    QTemporaryDir temporary_directory;
    QVERIFY(temporary_directory.isValid());
    VpCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("all dimensions"));
    for (int type_value = static_cast<int>(VpDimensionType::Linear);
         type_value <= static_cast<int>(VpDimensionType::Ordinate); ++type_value)
    {
        transaction->addDimension(makeDimension(static_cast<VpDimensionType>(type_value)));
    }
    transaction->commit();

    const QString smartcad_path =
        temporary_directory.filePath(QStringLiteral("dimensions.smartcad"));
    QVERIFY(document.save(smartcad_path).isSuccess());
    VpCadDocument smartcad_document;
    QVERIFY(smartcad_document.load(smartcad_path).isSuccess());
    QCOMPARE(smartcad_document.entities().size(), std::size_t(7));
    QCOMPARE(
        std::get<VpLinearDimensionEntity>(smartcad_document.entities()[5].geometry).dimension_type,
        VpDimensionType::ArcLength);

    const QString dxf_path = temporary_directory.filePath(QStringLiteral("dimensions.dxf"));
    VpDxfCodec codec;
    const auto write_result = codec.write(dxf_path, document);
    QVERIFY(write_result.isSuccess());
    QCOMPARE(write_result.value().exported_entity_count, 7);
    QCOMPARE(write_result.value().skipped_entity_count, 0);

    VpCadDocument dxf_document;
    const auto read_result = codec.read(dxf_path, dxf_document);
    QVERIFY(read_result.isSuccess());
    QCOMPARE(dxf_document.entities().size(), std::size_t(7));
    QCOMPARE(std::get<VpLinearDimensionEntity>(dxf_document.entities()[6].geometry).dimension_type,
             VpDimensionType::Ordinate);
}

void VpDimensionTest::dimensionStyleUndoAndPersistence()
{
    QTemporaryDir temporary_directory;
    QVERIFY(temporary_directory.isValid());
    VpCadDocument document;
    VpDimensionStyleRecord iso_style;
    iso_style.name = QStringLiteral("ISO-25");
    iso_style.text_height = 3.5;
    iso_style.arrow_size = 3.0;
    iso_style.overall_scale = 2.0;
    iso_style.linear_scale = 0.5;
    iso_style.linear_precision = 4;
    iso_style.angular_precision = 3;
    iso_style.prefix = QStringLiteral("[");
    iso_style.suffix = QStringLiteral("]");
    iso_style.suppress_trailing_zeros = true;
    QVERIFY(document.addOrUpdateDimensionStyle(iso_style));
    QVERIFY(document.setCurrentDimensionStyle(QStringLiteral("ISO-25")));
    QCOMPARE(document.currentDimensionStyleName(), QStringLiteral("ISO-25"));
    document.undo();
    QCOMPARE(document.currentDimensionStyleName(), QStringLiteral("Standard"));
    document.redo();
    QCOMPARE(document.currentDimensionStyleName(), QStringLiteral("ISO-25"));

    auto transaction = document.beginTransaction(QStringLiteral("styled dimension"));
    VpLinearDimensionEntity dimension = makeDimension(VpDimensionType::Aligned);
    dimension.style_name.clear();
    transaction->addDimension(std::move(dimension));
    transaction->commit();
    QCOMPARE(std::get<VpLinearDimensionEntity>(document.entities().front().geometry).style_name,
             QStringLiteral("ISO-25"));
    QVERIFY(!document.removeDimensionStyle(QStringLiteral("ISO-25")));

    const QString file_path = temporary_directory.filePath(QStringLiteral("styles.smartcad"));
    QVERIFY(document.save(file_path).isSuccess());
    VpCadDocument loaded_document;
    QVERIFY(loaded_document.load(file_path).isSuccess());
    QCOMPARE(loaded_document.currentDimensionStyleName(), QStringLiteral("ISO-25"));
    const VpDimensionStyleRecord* loaded_style =
        loaded_document.dimensionStyle(QStringLiteral("iso-25"));
    QVERIFY(loaded_style);
    QCOMPARE(loaded_style->text_height, 3.5);
    QCOMPARE(loaded_style->linear_precision, 4);
    QVERIFY(loaded_style->suppress_trailing_zeros);
}

void VpDimensionTest::explodeAllDimensionTypes()
{
    for (int type_value = static_cast<int>(VpDimensionType::Linear);
         type_value <= static_cast<int>(VpDimensionType::Ordinate); ++type_value)
    {
        VpEntityRecord source;
        source.id = 42;
        source.type = VpEntityType::LinearDimension;
        source.geometry = makeDimension(static_cast<VpDimensionType>(type_value));
        const std::vector<VpEntityRecord> parts = explodedEntityParts(source);
        QVERIFY(parts.size() >= 3);
        QVERIFY(std::none_of(parts.begin(), parts.end(),
                             [](const VpEntityRecord& part)
                             {
                                 return part.type == VpEntityType::LinearDimension;
                             }));
        QVERIFY(std::any_of(parts.begin(), parts.end(),
                            [](const VpEntityRecord& part)
                            {
                                return part.type == VpEntityType::Text;
                            }));
    }

    VpCadDocument document;
    auto create_transaction = document.beginTransaction(QStringLiteral("dimension"));
    create_transaction->addDimension(makeDimension(VpDimensionType::Aligned));
    create_transaction->commit();
    const VpEntityRecord original = document.entities().front();
    std::vector<VpEntityRecord> parts = explodedEntityParts(original);
    auto explode_transaction = document.beginTransaction(QStringLiteral("explode dimension"));
    explode_transaction->replaceEntity(original.id, parts.front());
    for (std::size_t index = 1; index < parts.size(); ++index)
    {
        explode_transaction->addEntityCopy(std::move(parts[index]));
    }
    explode_transaction->commit();
    QVERIFY(document.entities().size() > 1);
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(1));
    QCOMPARE(document.entities().front().type, VpEntityType::LinearDimension);
}

} // namespace Vp

QTEST_MAIN(Vp::VpDimensionTest)
#include "vp_dimension_test.moc"
