#include "s_bitmap_vectorizer.h"
#include "s_cad_document.h"
#include "s_document_transaction.h"
#include "s_dxf_codec.h"
#include "s_svg_document_operations.h"
#include "s_svg_parser.h"
#include "s_vector_document_import.h"
#include "s_vector_fill.h"

#include <QImage>
#include <QTemporaryDir>
#include <QtTest>
#include <algorithm>
#include <variant>

namespace smartGraphics
{
namespace
{

double polygonArea(const std::vector<SPoint2d>& points)
{
    double twice_area = 0.0;
    for (std::size_t index = 0; index < points.size(); ++index)
    {
        const SPoint2d& first = points[index];
        const SPoint2d& second = points[(index + 1) % points.size()];
        twice_area += first.x * second.y - second.x * first.y;
    }
    return std::abs(twice_area) * 0.5;
}

double hatchArea(const SHatchEntity& hatch)
{
    double result = polygonArea(hatch.boundary);
    for (const std::vector<SPoint2d>& island : hatch.island_boundaries)
    {
        result -= polygonArea(island);
    }
    return result;
}

double hatchAreaForColor(const SCadDocument& document, const QColor& color)
{
    double result = 0.0;
    for (const SEntityRecord& entity : document.entities())
    {
        if (entity.type == SEntityType::Hatch &&
            document.layerColor(entity.layer_name) == color)
        {
            result += hatchArea(std::get<SHatchEntity>(entity.geometry));
        }
    }
    return result;
}

SVectorImportGeometry overlappingColorBlocks()
{
    SHatchEntity lower;
    lower.boundary = {
        {0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {0.0, 10.0},
    };
    SHatchEntity upper;
    upper.boundary = {
        {5.0, 0.0}, {15.0, 0.0}, {15.0, 10.0}, {5.0, 10.0},
    };
    SVectorImportGeometry geometry;
    geometry.entities = {
        {lower, SEntityType::Hatch, QColor(Qt::red)},
        {upper, SEntityType::Hatch, QColor(Qt::blue)},
    };
    return geometry;
}

} // namespace

class SSvgVectorImportTest final : public QObject
{
    Q_OBJECT

  private slots:
    void parsesSvgShapesPathCommandsTransformAndColors();
    void createsSingleLineFillWithHole();
    void createsPolygonOffsetAsPolylineEntities();
    void importsSvgColorBlocksWithoutLineFill();
    void fillsImportedSvgAsIndependentTransaction();
    void fillsOnlySelectedSvgColorBlock();
    void deduplicatesByRequestedLayerPriority();
    void vectorizesSolidBitmapAsOneRegion();
    void keepsDiagonalPixelsAsSeparateContours();
    void vectorizesSplitMergeRunsWithSameParallelResult();
    void vectorizesHoleAsTwoClosedContours();
    void importsColorLayersInOneUndoableTransaction();
    void persistsAndExportsImportedLineEntities();
};

void SSvgVectorImportTest::parsesSvgShapesPathCommandsTransformAndColors()
{
    const QByteArray svg =
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 100 80\">"
        "<g transform=\"translate(5 4)\" opacity=\"0.8\">"
        "<rect x=\"0\" y=\"0\" width=\"20\" height=\"10\" fill=\"#123456\"/>"
        "<path d=\"M30 10 L40 10 H45 V15 C45 20 50 20 50 15 "
        "S55 10 60 15 Q65 20 70 15 T80 15 A5 5 0 0 1 85 20 Z\" "
        "fill=\"rgb(200,20,30)\" fill-rule=\"evenodd\"/>"
        "<polyline points=\"1,20 5,25 10,20\" fill=\"none\" stroke=\"#00ff00\"/>"
        "</g><filter id=\"ignored\"/></svg>";
    const SResult<SSvgVectorData> result = parseSvgVectorData(svg);
    QVERIFY2(result, qPrintable(result.errorMessage()));
    QCOMPARE(result.value().source_width, 100.0);
    QCOMPARE(result.value().source_height, 80.0);
    QCOMPARE(result.value().regions.size(), std::size_t(2));
    QCOMPARE(result.value().outlines.size(), std::size_t(3));
    QVERIFY(!result.value().outlines.back().is_closed);
    QCOMPARE(result.value().regions.front().color.red(), 0x12);
    QCOMPARE(result.value().regions.front().color.alpha(), 204);
    QCOMPARE(result.value().regions.back().fill_rule, SVectorFillRule::EvenOdd);
    QVERIFY(!result.value().warnings.isEmpty());
}

void SSvgVectorImportTest::createsSingleLineFillWithHole()
{
    SVectorRegion region;
    region.fill_rule = SVectorFillRule::EvenOdd;
    region.contours = {
        {{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {0.0, 10.0}},
        {{3.0, 3.0}, {7.0, 3.0}, {7.0, 7.0}, {3.0, 7.0}},
    };
    const SResult<std::vector<SPolylineEntity>> result =
        createSingleLineFill(region, 2.0, 0.0);
    QVERIFY2(result, qPrintable(result.errorMessage()));
    QVERIFY(result.value().size() >= 6);
    for (const SPolylineEntity& polyline : result.value())
    {
        QCOMPARE(polyline.vertices.size(), std::size_t(2));
        QVERIFY(!polyline.is_closed);
        if (std::abs(polyline.vertices.front().y - 4.0) < 1.0e-8)
        {
            QVERIFY(polyline.vertices.back().x <= 3.0 ||
                    polyline.vertices.front().x >= 7.0);
        }
    }
}

void SSvgVectorImportTest::createsPolygonOffsetAsPolylineEntities()
{
    SVectorRegion region;
    region.contours = {
        {{0.0, 0.0}, {12.0, 0.0}, {12.0, 12.0}, {0.0, 12.0}},
    };
    const SResult<std::vector<SPolylineEntity>> result =
        createPolygonOffsetFill(region, 2.0);
    QVERIFY2(result, qPrintable(result.errorMessage()));
    QVERIFY(result.value().size() >= 2);
    QVERIFY(std::all_of(
        result.value().begin(), result.value().end(),
        [](const SPolylineEntity& polyline)
        {
            return polyline.is_closed && polyline.vertices.size() >= 3;
        }));

    SSvgVectorData data;
    data.source_width = 12.0;
    data.source_height = 12.0;
    data.regions.push_back(region);
    SVectorImportSettings settings;
    settings.fill_mode = SImportFillMode::PolygonOffset;
    settings.fill_spacing = 2.0;
    settings.preserve_outlines = false;
    settings.include_color_blocks = false;
    const auto geometry = createVectorImportGeometry(data, settings);
    QVERIFY(geometry);
    QVERIFY(std::all_of(geometry.value().entities.begin(), geometry.value().entities.end(),
                        [](const SColoredEntityGeometry& entity)
                        {
                            return entity.type == SEntityType::Polyline &&
                                   std::holds_alternative<SPolylineEntity>(
                                       entity.geometry);
                        }));
}

void SSvgVectorImportTest::importsSvgColorBlocksWithoutLineFill()
{
    const QByteArray svg =
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 20 10\">"
        "<rect width=\"20\" height=\"10\" fill=\"#336699\"/></svg>";
    const SResult<SSvgVectorData> parsed = parseSvgVectorData(svg);
    QVERIFY2(parsed, qPrintable(parsed.errorMessage()));
    const SResult<SVectorImportGeometry> geometry =
        createVectorImportGeometry(parsed.value(), SVectorImportSettings{});
    QVERIFY2(geometry, qPrintable(geometry.errorMessage()));

    const auto hatch_count = std::count_if(
        geometry.value().entities.begin(), geometry.value().entities.end(),
        [](const SColoredEntityGeometry& entity)
        {
            return entity.type == SEntityType::Hatch;
        });
    const auto line_count = std::count_if(
        geometry.value().entities.begin(), geometry.value().entities.end(),
        [](const SColoredEntityGeometry& entity)
        {
            return entity.type == SEntityType::Line;
        });
    QCOMPARE(hatch_count, std::ptrdiff_t(1));
    QCOMPARE(line_count, std::ptrdiff_t(0));
}

void SSvgVectorImportTest::fillsImportedSvgAsIndependentTransaction()
{
    SCadDocument document;
    QVERIFY(importVectorGeometry(document, overlappingColorBlocks()));
    const std::size_t imported_count = document.entities().size();

    SVectorImportSettings settings;
    settings.fill_mode = SImportFillMode::SingleLine;
    settings.fill_spacing = 2.0;
    settings.preserve_outlines = false;
    settings.include_color_blocks = false;
    const SResult<SSvgFillReport> result = fillSvgColorBlocks(document, settings);
    QVERIFY2(result, qPrintable(result.errorMessage()));
    QCOMPARE(result.value().source_region_count, std::size_t(2));
    QVERIFY(result.value().created_polyline_count > 0);
    QCOMPARE(document.entities().size(),
             imported_count + result.value().created_polyline_count);
    QVERIFY(std::all_of(
        document.entities().begin() + static_cast<std::ptrdiff_t>(imported_count),
        document.entities().end(),
        [](const SEntityRecord& entity)
        {
            return entity.type == SEntityType::Polyline &&
                   entity.layer_name.startsWith(QStringLiteral("SVG_FILL_"));
        }));

    document.undo();
    QCOMPARE(document.entities().size(), imported_count);
    QVERIFY(std::all_of(document.entities().begin(), document.entities().end(),
                        [](const SEntityRecord& entity)
                        {
                            return entity.type == SEntityType::Hatch;
                        }));
}

void SSvgVectorImportTest::fillsOnlySelectedSvgColorBlock()
{
    SCadDocument document;
    QVERIFY(importVectorGeometry(document, overlappingColorBlocks()));
    const SEntityRecord first_source = document.entities().front();
    const SEntityRecord second_source = document.entities().back();

    SVectorImportSettings settings;
    settings.fill_mode = SImportFillMode::SingleLine;
    settings.fill_spacing = 2.0;
    const SResult<SSvgFillReport> initial_result =
        fillSvgColorBlocks(document, settings);
    QVERIFY2(initial_result, qPrintable(initial_result.errorMessage()));

    const QString first_fill_layer =
        QStringLiteral("SVG_FILL_%1").arg(first_source.layer_name.mid(4));
    const QString second_fill_layer =
        QStringLiteral("SVG_FILL_%1").arg(second_source.layer_name.mid(4));
    const auto count_layer_entities =
        [&document](const QString& layer_name)
        {
            return std::count_if(
                document.entities().begin(), document.entities().end(),
                [&layer_name](const SEntityRecord& entity)
                {
                    return entity.layer_name == layer_name;
                });
        };
    const std::ptrdiff_t first_fill_count =
        count_layer_entities(first_fill_layer);
    const std::ptrdiff_t second_fill_count =
        count_layer_entities(second_fill_layer);
    QVERIFY(first_fill_count > 0);
    QVERIFY(second_fill_count > 0);

    const SSvgVectorData selected_data =
        svgColorBlockVectorData(document, {first_source.id});
    QCOMPARE(selected_data.regions.size(), std::size_t(1));
    settings.fill_spacing = 1.0;
    const SResult<SSvgFillReport> selected_result =
        fillSvgColorBlocks(document, settings, {first_source.id});
    QVERIFY2(selected_result, qPrintable(selected_result.errorMessage()));
    QCOMPARE(selected_result.value().source_region_count, std::size_t(1));
    QCOMPARE(selected_result.value().replaced_entity_count,
             static_cast<std::size_t>(first_fill_count));
    QVERIFY(count_layer_entities(first_fill_layer) > first_fill_count);
    QCOMPARE(count_layer_entities(second_fill_layer), second_fill_count);

    const std::size_t entity_count = document.entities().size();
    const SResult<SSvgFillReport> invalid_result =
        fillSvgColorBlocks(document, settings, {999999});
    QVERIFY(!invalid_result);
    QCOMPARE(document.entities().size(), entity_count);
}

void SSvgVectorImportTest::deduplicatesByRequestedLayerPriority()
{
    SCadDocument upper_first;
    QVERIFY(importVectorGeometry(upper_first, overlappingColorBlocks()));
    const SResult<SSvgDeduplicateReport> upper_result =
        deduplicateSvgColorBlocks(upper_first, SSvgLayerPriority::UpperFirst);
    QVERIFY2(upper_result, qPrintable(upper_result.errorMessage()));
    QCOMPARE(hatchAreaForColor(upper_first, QColor(Qt::blue)), 100.0);
    QCOMPARE(hatchAreaForColor(upper_first, QColor(Qt::red)), 50.0);
    QCOMPARE(hatchAreaForColor(upper_first, QColor(Qt::blue)) +
                 hatchAreaForColor(upper_first, QColor(Qt::red)),
             150.0);

    SCadDocument lower_first;
    QVERIFY(importVectorGeometry(lower_first, overlappingColorBlocks()));
    const SResult<SSvgDeduplicateReport> lower_result =
        deduplicateSvgColorBlocks(lower_first, SSvgLayerPriority::LowerFirst);
    QVERIFY2(lower_result, qPrintable(lower_result.errorMessage()));
    QCOMPARE(hatchAreaForColor(lower_first, QColor(Qt::red)), 100.0);
    QCOMPARE(hatchAreaForColor(lower_first, QColor(Qt::blue)), 50.0);
    QCOMPARE(hatchAreaForColor(lower_first, QColor(Qt::red)) +
                 hatchAreaForColor(lower_first, QColor(Qt::blue)),
             150.0);
}

void SSvgVectorImportTest::vectorizesSolidBitmapAsOneRegion()
{
    QImage bitmap(300, 300, QImage::Format_ARGB32);
    bitmap.fill(QColor(30, 90, 180));
    SBitmapVectorSettings settings;
    settings.ignore_background = false;
    const SResult<QByteArray> svg = bitmapToSvgData(bitmap, settings);
    QVERIFY2(svg, qPrintable(svg.errorMessage()));
    QVERIFY(svg.value().size() < 1024);
    const SResult<SSvgVectorData> parsed = parseSvgVectorData(svg.value());
    QVERIFY(parsed);
    QCOMPARE(parsed.value().regions.size(), std::size_t(1));
    QCOMPARE(parsed.value().regions.front().contours.size(), std::size_t(1));
    QCOMPARE(parsed.value().regions.front().contours.front().size(), std::size_t(4));
    QCOMPARE(parsed.value().source_width, 300.0);
    QCOMPARE(parsed.value().source_height, 300.0);
}

void SSvgVectorImportTest::keepsDiagonalPixelsAsSeparateContours()
{
    QImage bitmap(2, 2, QImage::Format_ARGB32);
    bitmap.fill(Qt::transparent);
    bitmap.setPixelColor(0, 0, QColor(Qt::red));
    bitmap.setPixelColor(1, 1, QColor(Qt::red));
    SBitmapVectorSettings settings;
    settings.ignore_background = false;
    const auto svg = bitmapToSvgData(bitmap, settings);
    QVERIFY(svg);
    const auto parsed = parseSvgVectorData(svg.value());
    QVERIFY(parsed);
    QCOMPARE(parsed.value().regions.size(), std::size_t(1));
    QCOMPARE(parsed.value().regions.front().contours.size(), std::size_t(2));
}

void SSvgVectorImportTest::vectorizesSplitMergeRunsWithSameParallelResult()
{
    QImage bitmap(12, 8, QImage::Format_ARGB32);
    bitmap.fill(Qt::transparent);
    const QColor color(25, 120, 210);
    for (int x = 1; x < 11; ++x)
    {
        bitmap.setPixelColor(x, 1, color);
    }
    for (int y = 2; y < 7; ++y)
    {
        for (int x = 3; x < 9; ++x)
        {
            bitmap.setPixelColor(x, y, color);
        }
    }
    SBitmapVectorSettings serial_settings;
    serial_settings.ignore_background = false;
    serial_settings.worker_count = 1;
    SBitmapVectorSettings parallel_settings = serial_settings;
    parallel_settings.worker_count = 4;
    const auto serial = bitmapToVectorResult(bitmap, serial_settings);
    const auto parallel = bitmapToVectorResult(bitmap, parallel_settings);
    QVERIFY2(serial, qPrintable(serial.errorMessage()));
    QVERIFY2(parallel, qPrintable(parallel.errorMessage()));
    QCOMPARE(serial.value().svg_data, parallel.value().svg_data);
    QCOMPARE(serial.value().metrics.component_count, qint64(1));
    QCOMPARE(serial.value().metrics.contour_count, qint64(1));
    QVERIFY(serial.value().metrics.segment_count < 20);
}

void SSvgVectorImportTest::vectorizesHoleAsTwoClosedContours()
{
    QImage bitmap(10, 10, QImage::Format_ARGB32);
    bitmap.fill(QColor(Qt::red));
    for (int y = 3; y < 7; ++y)
    {
        for (int x = 3; x < 7; ++x)
        {
            bitmap.setPixelColor(x, y, Qt::transparent);
        }
    }
    SBitmapVectorSettings settings;
    settings.ignore_background = false;
    settings.worker_count = 3;
    const auto result = bitmapToVectorResult(bitmap, settings);
    QVERIFY2(result, qPrintable(result.errorMessage()));
    QCOMPARE(result.value().metrics.component_count, qint64(1));
    QCOMPARE(result.value().metrics.contour_count, qint64(2));
    const auto parsed = parseSvgVectorData(result.value().svg_data);
    QVERIFY2(parsed, qPrintable(parsed.errorMessage()));
    QCOMPARE(parsed.value().regions.size(), std::size_t(1));
    QCOMPARE(parsed.value().regions.front().contours.size(), std::size_t(2));
}

void SSvgVectorImportTest::importsColorLayersInOneUndoableTransaction()
{
    SVectorImportGeometry geometry;
    geometry.entities = {
        {SLineEntity{{0.0, 0.0}, {10.0, 0.0}}, SEntityType::Line,
         QColor(255, 0, 0)},
        {SLineEntity{{0.0, 1.0}, {10.0, 1.0}}, SEntityType::Line,
         QColor(0, 0, 255, 128)},
    };
    SCadDocument document;
    const auto result = importVectorGeometry(document, geometry);
    QVERIFY2(result, qPrintable(result.errorMessage()));
    QCOMPARE(document.entities().size(), std::size_t(2));
    QCOMPARE(document.layers().size(), std::size_t(3));
    QVERIFY(document.canUndo());
    document.undo();
    QVERIFY(document.entities().empty());
    QCOMPARE(document.layers().size(), std::size_t(1));
    document.redo();
    QCOMPARE(document.entities().size(), std::size_t(2));
    QCOMPARE(document.layers().size(), std::size_t(3));
}

void SSvgVectorImportTest::persistsAndExportsImportedLineEntities()
{
    SVectorImportGeometry geometry;
    geometry.entities.push_back(
        {SLineEntity{{1.0, 2.0}, {8.0, 9.0}}, SEntityType::Line, QColor(20, 160, 80)});
    SCadDocument document;
    QVERIFY(importVectorGeometry(document, geometry));
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString native_path = directory.filePath(QStringLiteral("vector.smartcad"));
    QVERIFY(document.save(native_path));
    SCadDocument loaded;
    QVERIFY(loaded.load(native_path));
    QCOMPARE(loaded.entities().size(), std::size_t(1));
    QCOMPARE(loaded.entities().front().type, SEntityType::Line);
    const QString dxf_path = directory.filePath(QStringLiteral("vector.dxf"));
    const SDxfCodec codec;
    const auto export_result = codec.write(dxf_path, loaded);
    QVERIFY(export_result);
    QCOMPARE(export_result.value().exported_entity_count, std::size_t(1));
}

} // namespace smartGraphics

QTEST_APPLESS_MAIN(smartGraphics::SSvgVectorImportTest)

#include "s_svg_vector_import_test.moc"
