#include "s_bitmap_benchmark_validation.h"

#include "s_svg_parser.h"

#include <QCryptographicHash>
#include <QMap>
#include <algorithm>

namespace vectorPath
{
namespace
{

QByteArray contourHash(const std::vector<SBitmapContour>& contours)
{
    std::vector<QByteArray> entries;
    entries.reserve(contours.size());
    for (const SBitmapContour& contour : contours)
    {
        QByteArray entry = QByteArray::number(contour.color, 16);
        entry.append(':');
        for (const QPoint& point : contour.points)
        {
            entry.append(QByteArray::number(point.x()));
            entry.append(',');
            entry.append(QByteArray::number(point.y()));
            entry.append(';');
        }
        entries.push_back(std::move(entry));
    }
    std::sort(entries.begin(), entries.end());
    QCryptographicHash hash(QCryptographicHash::Sha256);
    for (const QByteArray& entry : entries)
    {
        hash.addData(entry);
        hash.addData("|", 1);
    }
    return hash.result().toHex();
}

QImage rasterize(int width, int height,
                 const std::vector<SBitmapContour>& contours)
{
    QMap<QRgb, std::vector<const SBitmapContour*>> color_contours;
    for (const SBitmapContour& contour : contours)
    {
        if (contour.points.size() >= 3)
        {
            color_contours[contour.color].push_back(&contour);
        }
    }
    QImage result(width, height, QImage::Format_ARGB32);
    result.fill(Qt::transparent);
    for (auto color_iterator = color_contours.cbegin();
         color_iterator != color_contours.cend(); ++color_iterator)
    {
        std::vector<std::vector<int>> intersections(
            static_cast<std::size_t>(height));
        for (const SBitmapContour* contour : color_iterator.value())
        {
            for (std::size_t index = 0; index < contour->points.size(); ++index)
            {
                const QPoint& first = contour->points[index];
                const QPoint& second =
                    contour->points[(index + 1) % contour->points.size()];
                if (first.x() != second.x())
                {
                    continue;
                }
                const int begin_y =
                    std::max(0, std::min(first.y(), second.y()));
                const int end_y =
                    std::min(height, std::max(first.y(), second.y()));
                for (int y = begin_y; y < end_y; ++y)
                {
                    intersections[static_cast<std::size_t>(y)]
                        .push_back(first.x());
                }
            }
        }
        for (int y = 0; y < height; ++y)
        {
            std::vector<int>& row =
                intersections[static_cast<std::size_t>(y)];
            std::sort(row.begin(), row.end());
            QRgb* pixels =
                reinterpret_cast<QRgb*>(result.scanLine(y));
            for (std::size_t index = 0; index + 1 < row.size(); index += 2)
            {
                const int begin_x = std::max(0, row[index]);
                const int end_x = std::min(width, row[index + 1]);
                std::fill(pixels + begin_x, pixels + end_x,
                          color_iterator.key());
            }
        }
    }
    return result;
}

bool compareImage(const QImage& source, const QImage& actual,
                  QImage* difference)
{
    bool matches = true;
    if (difference)
    {
        *difference =
            QImage(source.size(), QImage::Format_ARGB32);
        difference->fill(Qt::transparent);
    }
    for (int y = 0; y < source.height(); ++y)
    {
        for (int x = 0; x < source.width(); ++x)
        {
            QRgb expected = source.pixel(x, y);
            if (qAlpha(expected) == 0)
            {
                expected = qRgba(0, 0, 0, 0);
            }
            if (actual.pixel(x, y) != expected)
            {
                matches = false;
                if (difference)
                {
                    difference->setPixel(x, y, qRgba(255, 0, 0, 255));
                }
            }
        }
    }
    return matches;
}

QString validateSvg(const SBitmapVectorResult& result,
                    const QString& algorithm)
{
    const auto parsed = parseSvgVectorData(result.svg_data);
    if (!parsed)
    {
        return QStringLiteral("%1 SVG 重新解析失败：%2")
            .arg(algorithm, parsed.errorMessage());
    }
    return {};
}

} // namespace

SBitmapValidationResult validateBitmapBenchmarkCase(
    const QImage& source, const SBitmapVectorResult& flood_fill,
    const SBitmapVectorResult& run_serial,
    const SBitmapVectorResult& run_parallel)
{
    SBitmapValidationResult result;
    result.flood_hash = contourHash(flood_fill.contours);
    result.serial_hash = contourHash(run_serial.contours);
    result.parallel_hash = contourHash(run_parallel.contours);
    if (flood_fill.metrics.component_count !=
            run_serial.metrics.component_count ||
        run_serial.metrics.component_count !=
            run_parallel.metrics.component_count)
    {
        result.error = QStringLiteral("三种算法的四邻域色块数量不同。");
        return result;
    }
    if (result.flood_hash != result.serial_hash ||
        result.serial_hash != result.parallel_hash)
    {
        result.error = QStringLiteral("三种算法的规范化轮廓哈希不同。");
        return result;
    }
    if (run_serial.svg_data != run_parallel.svg_data)
    {
        result.error = QStringLiteral("拆分法单线程与多线程 SVG 不一致。");
        return result;
    }
    const QString flood_svg_error = validateSvg(flood_fill,
                                                 QStringLiteral("四邻域"));
    const QString serial_svg_error = validateSvg(run_serial,
                                                  QStringLiteral("拆分单线程"));
    const QString parallel_svg_error = validateSvg(
        run_parallel, QStringLiteral("拆分多线程"));
    if (!flood_svg_error.isEmpty() || !serial_svg_error.isEmpty() ||
        !parallel_svg_error.isEmpty())
    {
        result.error = flood_svg_error + serial_svg_error + parallel_svg_error;
        return result;
    }

    const QImage flood_image =
        rasterize(source.width(), source.height(), flood_fill.contours);
    if (!compareImage(source, flood_image, &result.difference))
    {
        result.error = QStringLiteral("四邻域轮廓回栅格结果与输入不同。");
        return result;
    }
    const QImage serial_image =
        rasterize(source.width(), source.height(), run_serial.contours);
    if (!compareImage(source, serial_image, &result.difference))
    {
        result.error = QStringLiteral("拆分法单线程回栅格结果与输入不同。");
        return result;
    }
    const QImage parallel_image =
        rasterize(source.width(), source.height(), run_parallel.contours);
    if (!compareImage(source, parallel_image, &result.difference))
    {
        result.error = QStringLiteral("拆分法多线程回栅格结果与输入不同。");
        return result;
    }
    result.difference = {};
    result.passed = true;
    return result;
}

} // namespace vectorPath
