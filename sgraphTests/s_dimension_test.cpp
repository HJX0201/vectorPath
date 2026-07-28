#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_cad_viewport_geometry.h"
#include "s_dimension_geometry.h"
#include "s_document_transaction.h"
#include "s_dxf_codec.h"

#include <QTemporaryDir>
#include <QtTest>
#include <algorithm>
#include <cmath>

namespace smartGraphics
{

class SDimensionTest final : public QObject
{
    Q_OBJECT

  private slots:
    void measurementsCoverAllTypes();
    void viewportCreatesAllTypesWithUndo();
    void smartcadAndDxfRoundTrip();
    void dimensionStyleUndoAndPersistence();
    void explodeAllDimensionTypes();
};

SLinearDimensionEntity makeDimension(SDimensionType dimension_type)
{
    SLinearDimensionEntity dimension;
    dimension.dimension_type = dimension_type;
    dimension.first_point = {10.0, 0.0};
    dimension.second_point = {0.0, 10.0};
    dimension.dimension_line_point = {7.0, 7.0};
    dimension.center_point = {0.0, 0.0};
    dimension.style_name = QStringLiteral("Standard");
    return dimension;
}

void SDimensionTest::measurementsCoverAllTypes()
{
    SLinearDimensionEntity linear = makeDimension(SDimensionType::Linear);
    linear.first_point = {0.0, 0.0};
    linear.second_point = {12.0, 5.0};
    linear.dimension_line_point = {6.0, 8.0};
    QCOMPARE(dimensionMeasurement(linear), 12.0);

    SLinearDimensionEntity aligned = linear;
    aligned.dimension_type = SDimensionType::Aligned;
    QCOMPARE(dimensionMeasurement(aligned), 13.0);

    const SLinearDimensionEntity angular = makeDimension(SDimensionType::Angular);
    QCOMPARE(dimensionMeasurement(angular), 90.0);
    QVERIFY(dimensionDefaultText(angular).endsWith(QChar(0x00B0)));

    const SLinearDimensionEntity radius = makeDimension(SDimensionType::Radius);
    QCOMPARE(dimensionMeasurement(radius), 10.0);
    QVERIFY(dimensionDefaultText(radius).startsWith(QLatin1Char('R')));

    const SLinearDimensionEntity diameter = makeDimension(SDimensionType::Diameter);
    QCOMPARE(dimensionMeasurement(diameter), 20.0);

    const SLinearDimensionEntity arc_length = makeDimension(SDimensionType::ArcLength);
    QVERIFY(std::abs(dimensionMeasurement(arc_length) - 15.7079632679) < 1.0e-8);

    SLinearDimensionEntity ordinate = makeDimension(SDimensionType::Ordinate);
    ordinate.first_point = {4.0, 5.0};
    ordinate.dimension_line_point = {12.0, 5.0};
    QCOMPARE(dimensionMeasurement(ordinate), 5.0);
}

void SDimensionTest::viewportCreatesAllTypesWithUndo()
{
    SCadDocument document;
    SCadViewport viewport;
    viewport.setDocument(&document);

    viewport.setToolMode(SToolMode::LinearDimension);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({10.0, 0.0});
    viewport.submitWorldPoint({0.0, 3.0});

    viewport.setToolMode(SToolMode::AlignedDimension);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({6.0, 8.0});
    viewport.submitWorldPoint({3.0, 7.0});

    viewport.setToolMode(SToolMode::AngularDimension);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({10.0, 0.0});
    viewport.submitWorldPoint({0.0, 10.0});
    viewport.submitWorldPoint({7.0, 7.0});

    viewport.setToolMode(SToolMode::RadiusDimension);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({5.0, 0.0});
    viewport.submitWorldPoint({8.0, 0.0});

    viewport.setToolMode(SToolMode::DiameterDimension);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({5.0, 0.0});
    viewport.submitWorldPoint({8.0, 0.0});

    viewport.setToolMode(SToolMode::ArcLengthDimension);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({10.0, 0.0});
    viewport.submitWorldPoint({0.0, 10.0});
    viewport.submitWorldPoint({7.0, 7.0});

    viewport.setToolMode(SToolMode::OrdinateDimension);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({4.0, 5.0});
    viewport.submitWorldPoint({12.0, 5.0});

    QCOMPARE(document.entities().size(), std::size_t(7));
    for (std::size_t index = 0; index < document.entities().size(); ++index)
    {
        const auto& dimension =
            std::get<SLinearDimensionEntity>(document.entities()[index].geometry);
        QCOMPARE(static_cast<int>(dimension.dimension_type), static_cast<int>(index));
    }
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(6));
    document.redo();
    QCOMPARE(document.entities().size(), std::size_t(7));
}

void SDimensionTest::smartcadAndDxfRoundTrip()
{
    QTemporaryDir temporary_directory;
    QVERIFY(temporary_directory.isValid());
    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("all dimensions"));
    for (int type_value = static_cast<int>(SDimensionType::Linear);
         type_value <= static_cast<int>(SDimensionType::Ordinate); ++type_value)
    {
        transaction->addDimension(makeDimension(static_cast<SDimensionType>(type_value)));
    }
    transaction->commit();

    const QString smartcad_path =
        temporary_directory.filePath(QStringLiteral("dimensions.smartcad"));
    QVERIFY(document.save(smartcad_path).isSuccess());
    SCadDocument smartcad_document;
    QVERIFY(smartcad_document.load(smartcad_path).isSuccess());
    QCOMPARE(smartcad_document.entities().size(), std::size_t(7));
    QCOMPARE(
        std::get<SLinearDimensionEntity>(smartcad_document.entities()[5].geometry).dimension_type,
        SDimensionType::ArcLength);

    const QString dxf_path = temporary_directory.filePath(QStringLiteral("dimensions.dxf"));
    SDxfCodec codec;
    const auto write_result = codec.write(dxf_path, document);
    QVERIFY(write_result.isSuccess());
    QCOMPARE(write_result.value().exported_entity_count, 7);
    QCOMPARE(write_result.value().skipped_entity_count, 0);

    SCadDocument dxf_document;
    const auto read_result = codec.read(dxf_path, dxf_document);
    QVERIFY(read_result.isSuccess());
    QCOMPARE(dxf_document.entities().size(), std::size_t(7));
    QCOMPARE(std::get<SLinearDimensionEntity>(dxf_document.entities()[6].geometry).dimension_type,
             SDimensionType::Ordinate);
}

void SDimensionTest::dimensionStyleUndoAndPersistence()
{
    QTemporaryDir temporary_directory;
    QVERIFY(temporary_directory.isValid());
    SCadDocument document;
    SDimensionStyleRecord iso_style;
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
    SLinearDimensionEntity dimension = makeDimension(SDimensionType::Aligned);
    dimension.style_name.clear();
    transaction->addDimension(std::move(dimension));
    transaction->commit();
    QCOMPARE(std::get<SLinearDimensionEntity>(document.entities().front().geometry).style_name,
             QStringLiteral("ISO-25"));
    QVERIFY(!document.removeDimensionStyle(QStringLiteral("ISO-25")));

    const QString file_path = temporary_directory.filePath(QStringLiteral("styles.smartcad"));
    QVERIFY(document.save(file_path).isSuccess());
    SCadDocument loaded_document;
    QVERIFY(loaded_document.load(file_path).isSuccess());
    QCOMPARE(loaded_document.currentDimensionStyleName(), QStringLiteral("ISO-25"));
    const SDimensionStyleRecord* loaded_style =
        loaded_document.dimensionStyle(QStringLiteral("iso-25"));
    QVERIFY(loaded_style);
    QCOMPARE(loaded_style->text_height, 3.5);
    QCOMPARE(loaded_style->linear_precision, 4);
    QVERIFY(loaded_style->suppress_trailing_zeros);
}

void SDimensionTest::explodeAllDimensionTypes()
{
    for (int type_value = static_cast<int>(SDimensionType::Linear);
         type_value <= static_cast<int>(SDimensionType::Ordinate); ++type_value)
    {
        SEntityRecord source;
        source.id = 42;
        source.type = SEntityType::LinearDimension;
        source.geometry = makeDimension(static_cast<SDimensionType>(type_value));
        const std::vector<SEntityRecord> parts = explodedEntityParts(source);
        QVERIFY(parts.size() >= 3);
        QVERIFY(std::none_of(parts.begin(), parts.end(),
                             [](const SEntityRecord& part)
                             {
                                 return part.type == SEntityType::LinearDimension;
                             }));
        QVERIFY(std::any_of(parts.begin(), parts.end(),
                            [](const SEntityRecord& part)
                            {
                                return part.type == SEntityType::Text;
                            }));
    }

    SCadDocument document;
    auto create_transaction = document.beginTransaction(QStringLiteral("dimension"));
    create_transaction->addDimension(makeDimension(SDimensionType::Aligned));
    create_transaction->commit();
    const SEntityRecord original = document.entities().front();
    std::vector<SEntityRecord> parts = explodedEntityParts(original);
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
    QCOMPARE(document.entities().front().type, SEntityType::LinearDimension);
}

} // namespace smartGraphics

QTEST_MAIN(smartGraphics::SDimensionTest)
#include "s_dimension_test.moc"
