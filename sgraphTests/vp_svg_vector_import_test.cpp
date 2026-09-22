#include "vp_bitmap_vectorizer.h"
#include "vp_cad_document.h"
#include "vp_document_transaction.h"
#include "vp_dxf_codec.h"
#include "vp_qt_text.h"
#include "vp_svg_document_operations.h"
#include "vp_svg_parser.h"
#include "vp_vector_document_import.h"
#include "vp_vector_fill.h"

#include <QImage>
#include <QTemporaryDir>
#include <QtTest>
#include <algorithm>
#include <variant>

namespace Vp
{
namespace
{

double polygonArea(const std::vector<VpPoint2d>& points)
{
    double twice_area = 0.0;
    for (std::size_t index = 0; index < points.size(); ++index)
    {
        const VpPoint2d& first = points[index];
        const VpPoint2d& second = points[(index + 1) % points.size()];
        twice_area += first.x * second.y - second.x * first.y;
    }
    return std::abs(twice_area) * 0.5;
}

double hatchArea(const VpHatchEntity& hatch)
{
    double result = polygonArea(hatch.boundary);
    for (const std::vector<VpPoint2d>& island : hatch.island_boundaries)
    {
        result -= polygonArea(island);
    }
    return result;
}

double hatchAreaForColor(const VpCadDocument& document, const QColor& color)
{
    double result = 0.0;
    for (const VpEntityRecord& entity : document.entities())
    {
        if (entity.type == VpEntityType::Hatch && document.layerColor(entity.layer_name) == color)
        {
            result += hatchArea(std::get<VpHatchEntity>(entity.geometry));
        }
    }
    return result;
}

VpVectorImportGeometry overlappingColorBlocks()
{
    VpHatchEntity lower;
    lower.boundary = {
        {0.0, 0.0},
        {10.0, 0.0},
        {10.0, 10.0},
        {0.0, 10.0},
    };
    VpHatchEntity upper;
    upper.boundary = {
        {5.0, 0.0},
        {15.0, 0.0},
        {15.0, 10.0},
        {5.0, 10.0},
    };
    VpVectorImportGeometry geometry;
    geometry.entities = {
        {lower, VpEntityType::Hatch, QColor(Qt::red)},
        {upper, VpEntityType::Hatch, QColor(Qt::blue)},
    };
    return geometry;
}

} // namespace

class VpSvgVectorImportTest final : public QObject
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

void VpSvgVectorImportTest::parsesSvgShapesPathCommandsTransformAndColors()
{
    const QByteArray svg = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 100 80\">"
                           "<g transform=\"translate(5 4)\" opacity=\"0.8\">"
                           "<rect x=\"0\" y=\"0\" width=\"20\" height=\"10\" fill=\"#123456\"/>"
                           "<path d=\"M30 10 L40 10 H45 V15 C45 20 50 20 50 15 "
                           "S55 10 60 15 Q65 20 70 15 T80 15 A5 5 0 0 1 85 20 Z\" "
                           "fill=\"rgb(200,20,30)\" fill-rule=\"evenodd\"/>"
                           "<polyline points=\"1,20 5,25 10,20\" fill=\"none\" stroke=\"#00ff00\"/>"
                           "</g><filter id=\"ignored\"/></svg>";
    const VpResult<VpSvgVectorData> result = parseSvgVectorData(svg);
    QVERIFY2(result, qPrintable(toQtError(result)));
    QCOMPARE(result.value().source_width, 100.0);
    QCOMPARE(result.value().source_height, 80.0);
    QCOMPARE(result.value().regions.size(), std::size_t(2));
    QCOMPARE(result.value().outlines.size(), std::size_t(3));
    QVERIFY(!result.value().outlines.back().is_closed);
    QCOMPARE(result.value().regions.front().color.red(), 0x12);
    QCOMPARE(result.value().regions.front().color.alpha(), 204);
    QCOMPARE(result.value().regions.back().fill_rule, VpVectorFillRule::EvenOdd);
    QVERIFY(!result.value().warnings.isEmpty());
}

void VpSvgVectorImportTest::createsSingleLineFillWithHole()
{
    VpVectorRegion region;
    region.fill_rule = VpVectorFillRule::EvenOdd;
    region.contours = {
        {{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {0.0, 10.0}},
        {{3.0, 3.0}, {7.0, 3.0}, {7.0, 7.0}, {3.0, 7.0}},
    };
    const VpResult<std::vector<VpPolylineEntity>> result = createSingleLineFill(region, 2.0, 0.0);
    QVERIFY2(result, qPrintable(toQtError(result)));
    QVERIFY(result.value().size() >= 6);
    for (const VpPolylineEntity& polyline : result.value())
    {
        QCOMPARE(polyline.vertices.size(), std::size_t(2));
        QVERIFY(!polyline.is_closed);
        if (std::abs(polyline.vertices.front().y - 4.0) < 1.0e-8)
        {
            QVERIFY(polyline.vertices.back().x <= 3.0 || polyline.vertices.front().x >= 7.0);
        }
    }
}

void VpSvgVectorImportTest::createsPolygonOffsetAsPolylineEntities()
{
    VpVectorRegion region;
    region.contours = {
        {{0.0, 0.0}, {12.0, 0.0}, {12.0, 12.0}, {0.0, 12.0}},
    };
    const VpResult<std::vector<VpPolylineEntity>> result = createPolygonOffsetFill(region, 2.0);
    QVERIFY2(result, qPrintable(toQtError(result)));
    QVERIFY(result.value().size() >= 2);
    QVERIFY(std::all_of(result.value().begin(), result.value().end(),
                        [](const VpPolylineEntity& polyline)
                        {
                            return polyline.is_closed && polyline.vertices.size() >= 3;
                        }));

    VpSvgVectorData data;
    data.source_width = 12.0;
    data.source_height = 12.0;
    data.regions.push_back(region);
    VpVectorImportSettings settings;
    settings.fill_mode = VpImportFillMode::PolygonOffset;
    settings.fill_spacing = 2.0;
    settings.preserve_outlines = false;
    settings.include_color_blocks = false;
    const auto geometry = createVectorImportGeometry(data, settings);
    QVERIFY(geometry);
    QVERIFY(std::all_of(geometry.value().entities.begin(), geometry.value().entities.end(),
                        [](const VpColoredEntityGeometry& entity)
                        {
                            return entity.type == VpEntityType::Polyline &&
                                   std::holds_alternative<VpPolylineEntity>(entity.geometry);
                        }));
}

void VpSvgVectorImportTest::importsSvgColorBlocksWithoutLineFill()
{
    const QByteArray svg = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 20 10\">"
                           "<rect width=\"20\" height=\"10\" fill=\"#336699\"/></svg>";
    const VpResult<VpSvgVectorData> parsed = parseSvgVectorData(svg);
    QVERIFY2(parsed, qPrintable(toQtError(parsed)));
    const VpResult<VpVectorImportGeometry> geometry =
        createVectorImportGeometry(parsed.value(), VpVectorImportSettings{});
    QVERIFY2(geometry, qPrintable(toQtError(geometry)));

    const auto hatch_count =
        std::count_if(geometry.value().entities.begin(), geometry.value().entities.end(),
                      [](const VpColoredEntityGeometry& entity)
                      {
                          return entity.type == VpEntityType::Hatch;
                      });
    const auto line_count =
        std::count_if(geometry.value().entities.begin(), geometry.value().entities.end(),
                      [](const VpColoredEntityGeometry& entity)
                      {
                          return entity.type == VpEntityType::Line;
                      });
    QCOMPARE(hatch_count, std::ptrdiff_t(1));
    QCOMPARE(line_count, std::ptrdiff_t(0));
}

void VpSvgVectorImportTest::fillsImportedSvgAsIndependentTransaction()
{
    VpCadDocument document;
    QVERIFY(importVectorGeometry(document, overlappingColorBlocks()));
    const std::size_t imported_count = document.entities().size();

    VpVectorImportSettings settings;
    settings.fill_mode = VpImportFillMode::SingleLine;
    settings.fill_spacing = 2.0;
    settings.preserve_outlines = false;
    settings.include_color_blocks = false;
    const VpResult<VpSvgFillReport> result = fillSvgColorBlocks(document, settings);
    QVERIFY2(result, qPrintable(toQtError(result)));
    QCOMPARE(result.value().source_region_count, std::size_t(2));
    QVERIFY(result.value().created_polyline_count > 0);
    QCOMPARE(document.entities().size(), imported_count + result.value().created_polyline_count);
    QVERIFY(std::all_of(document.entities().begin() + static_cast<std::ptrdiff_t>(imported_count),
                        document.entities().end(),
                        [](const VpEntityRecord& entity)
                        {
                            return entity.type == VpEntityType::Polyline &&
                                   entity.layer_name.startsWith(QStringLiteral("SVG_FILL_"));
                        }));

    document.undo();
    QCOMPARE(document.entities().size(), imported_count);
    QVERIFY(std::all_of(document.entities().begin(), document.entities().end(),
                        [](const VpEntityRecord& entity)
                        {
                            return entity.type == VpEntityType::Hatch;
                        }));
}

void VpSvgVectorImportTest::fillsOnlySelectedSvgColorBlock()
{
    VpCadDocument document;
    QVERIFY(importVectorGeometry(document, overlappingColorBlocks()));
    const VpEntityRecord first_source = document.entities().front();
    const VpEntityRecord second_source = document.entities().back();

    VpVectorImportSettings settings;
    settings.fill_mode = VpImportFillMode::SingleLine;
    settings.fill_spacing = 2.0;
    const VpResult<VpSvgFillReport> initial_result = fillSvgColorBlocks(document, settings);
    QVERIFY2(initial_result, qPrintable(toQtError(initial_result)));

    const QString first_fill_layer =
        QStringLiteral("SVG_FILL_%1").arg(first_source.layer_name.mid(4));
    const QString second_fill_layer =
        QStringLiteral("SVG_FILL_%1").arg(second_source.layer_name.mid(4));
    const auto count_layer_entities = [&document](const QString& layer_name)
    {
        return std::count_if(document.entities().begin(), document.entities().end(),
                             [&layer_name](const VpEntityRecord& entity)
                             {
                                 return entity.layer_name == layer_name;
                             });
    };
    const std::ptrdiff_t first_fill_count = count_layer_entities(first_fill_layer);
    const std::ptrdiff_t second_fill_count = count_layer_entities(second_fill_layer);
    QVERIFY(first_fill_count > 0);
    QVERIFY(second_fill_count > 0);

    const VpSvgVectorData selected_data = svgColorBlockVectorData(document, {first_source.id});
    QCOMPARE(selected_data.regions.size(), std::size_t(1));
    settings.fill_spacing = 1.0;
    const VpResult<VpSvgFillReport> selected_result =
        fillSvgColorBlocks(document, settings, {first_source.id});
    QVERIFY2(selected_result, qPrintable(toQtError(selected_result)));
    QCOMPARE(selected_result.value().source_region_count, std::size_t(1));
    QCOMPARE(selected_result.value().replaced_entity_count,
             static_cast<std::size_t>(first_fill_count));
    QVERIFY(count_layer_entities(first_fill_layer) > first_fill_count);
    QCOMPARE(count_layer_entities(second_fill_layer), second_fill_count);

    const std::size_t entity_count = document.entities().size();
    const VpResult<VpSvgFillReport> invalid_result =
        fillSvgColorBlocks(document, settings, {999999});
    QVERIFY(!invalid_result);
    QCOMPARE(document.entities().size(), entity_count);
}

void VpSvgVectorImportTest::deduplicatesByRequestedLayerPriority()
{
    VpCadDocument upper_first;
    QVERIFY(importVectorGeometry(upper_first, overlappingColorBlocks()));
    const VpResult<VpSvgDeduplicateReport> upper_result =
        deduplicateSvgColorBlocks(upper_first, VpSvgLayerPriority::UpperFirst);
    QVERIFY2(upper_result, qPrintable(toQtError(upper_result)));
    QCOMPARE(hatchAreaForColor(upper_first, QColor(Qt::blue)), 100.0);
    QCOMPARE(hatchAreaForColor(upper_first, QColor(Qt::red)), 50.0);
    QCOMPARE(hatchAreaForColor(upper_first, QColor(Qt::blue)) +
                 hatchAreaForColor(upper_first, QColor(Qt::red)),
             150.0);

    VpCadDocument lower_first;
    QVERIFY(importVectorGeometry(lower_first, overlappingColorBlocks()));
    const VpResult<VpSvgDeduplicateReport> lower_result =
        deduplicateSvgColorBlocks(lower_first, VpSvgLayerPriority::LowerFirst);
    QVERIFY2(lower_result, qPrintable(toQtError(lower_result)));
    QCOMPARE(hatchAreaForColor(lower_first, QColor(Qt::red)), 100.0);
    QCOMPARE(hatchAreaForColor(lower_first, QColor(Qt::blue)), 50.0);
    QCOMPARE(hatchAreaForColor(lower_first, QColor(Qt::red)) +
                 hatchAreaForColor(lower_first, QColor(Qt::blue)),
             150.0);
}

void VpSvgVectorImportTest::vectorizesSolidBitmapAsOneRegion()
{
    QImage bitmap(300, 300, QImage::Format_ARGB32);
    bitmap.fill(QColor(30, 90, 180));
    VpBitmapVectorSettings settings;
    settings.ignore_background = false;
    const VpResult<QByteArray> svg = bitmapToSvgData(bitmap, settings);
    QVERIFY2(svg, qPrintable(toQtError(svg)));
    QVERIFY(svg.value().size() < 1024);
    const VpResult<VpSvgVectorData> parsed = parseSvgVectorData(svg.value());
    QVERIFY(parsed);
    QCOMPARE(parsed.value().regions.size(), std::size_t(1));
    QCOMPARE(parsed.value().regions.front().contours.size(), std::size_t(1));
    QCOMPARE(parsed.value().regions.front().contours.front().size(), std::size_t(4));
    QCOMPARE(parsed.value().source_width, 300.0);
    QCOMPARE(parsed.value().source_height, 300.0);
}

void VpSvgVectorImportTest::keepsDiagonalPixelsAsSeparateContours()
{
    QImage bitmap(2, 2, QImage::Format_ARGB32);
    bitmap.fill(Qt::transparent);
    bitmap.setPixelColor(0, 0, QColor(Qt::red));
    bitmap.setPixelColor(1, 1, QColor(Qt::red));
    VpBitmapVectorSettings settings;
    settings.ignore_background = false;
    const auto svg = bitmapToSvgData(bitmap, settings);
    QVERIFY(svg);
    const auto parsed = parseSvgVectorData(svg.value());
    QVERIFY(parsed);
    QCOMPARE(parsed.value().regions.size(), std::size_t(1));
    QCOMPARE(parsed.value().regions.front().contours.size(), std::size_t(2));
}

void VpSvgVectorImportTest::vectorizesSplitMergeRunsWithSameParallelResult()
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
    VpBitmapVectorSettings serial_settings;
    serial_settings.ignore_background = false;
    serial_settings.worker_count = 1;
    VpBitmapVectorSettings parallel_settings = serial_settings;
    parallel_settings.worker_count = 4;
    const auto serial = bitmapToVectorResult(bitmap, serial_settings);
    const auto parallel = bitmapToVectorResult(bitmap, parallel_settings);
    QVERIFY2(serial, qPrintable(toQtError(serial)));
    QVERIFY2(parallel, qPrintable(toQtError(parallel)));
    QCOMPARE(serial.value().svg_data, parallel.value().svg_data);
    QCOMPARE(serial.value().metrics.component_count, qint64(1));
    QCOMPARE(serial.value().metrics.contour_count, qint64(1));
    QVERIFY(serial.value().metrics.segment_count < 20);
}

void VpSvgVectorImportTest::vectorizesHoleAsTwoClosedContours()
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
    VpBitmapVectorSettings settings;
    settings.ignore_background = false;
    settings.worker_count = 3;
    const auto result = bitmapToVectorResult(bitmap, settings);
    QVERIFY2(result, qPrintable(toQtError(result)));
    QCOMPARE(result.value().metrics.component_count, qint64(1));
    QCOMPARE(result.value().metrics.contour_count, qint64(2));
    const auto parsed = parseSvgVectorData(result.value().svg_data);
    QVERIFY2(parsed, qPrintable(toQtError(parsed)));
    QCOMPARE(parsed.value().regions.size(), std::size_t(1));
    QCOMPARE(parsed.value().regions.front().contours.size(), std::size_t(2));
}

void VpSvgVectorImportTest::importsColorLayersInOneUndoableTransaction()
{
    VpVectorImportGeometry geometry;
    geometry.entities = {
        {VpLineEntity{{0.0, 0.0}, {10.0, 0.0}}, VpEntityType::Line, QColor(255, 0, 0)},
        {VpLineEntity{{0.0, 1.0}, {10.0, 1.0}}, VpEntityType::Line, QColor(0, 0, 255, 128)},
    };
    VpCadDocument document;
    const auto result = importVectorGeometry(document, geometry);
    QVERIFY2(result, qPrintable(toQtError(result)));
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

void VpSvgVectorImportTest::persistsAndExportsImportedLineEntities()
{
    VpVectorImportGeometry geometry;
    geometry.entities.push_back(
        {VpLineEntity{{1.0, 2.0}, {8.0, 9.0}}, VpEntityType::Line, QColor(20, 160, 80)});
    VpCadDocument document;
    QVERIFY(importVectorGeometry(document, geometry));
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString native_path = directory.filePath(QStringLiteral("vector.smartcad"));
    QVERIFY(document.save(native_path));
    VpCadDocument loaded;
    QVERIFY(loaded.load(native_path));
    QCOMPARE(loaded.entities().size(), std::size_t(1));
    QCOMPARE(loaded.entities().front().type, VpEntityType::Line);
    const QString dxf_path = directory.filePath(QStringLiteral("vector.dxf"));
    const VpDxfCodec codec;
    const auto export_result = codec.write(dxf_path, loaded);
    QVERIFY(export_result);
    QCOMPARE(export_result.value().exported_entity_count, std::size_t(1));
}

} // namespace Vp

QTEST_APPLESS_MAIN(Vp::VpSvgVectorImportTest)

#include "vp_svg_vector_import_test.moc"
