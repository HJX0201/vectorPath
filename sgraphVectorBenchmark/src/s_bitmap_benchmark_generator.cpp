#include "s_bitmap_benchmark_generator.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QRandomGenerator>
#include <QSet>

namespace smartGraphics
{
namespace
{

const QVector<QRgb>& palette()
{
    static const QVector<QRgb> colors{
        qRgba(244, 246, 250, 255), qRgba(32, 82, 149, 255),
        qRgba(225, 69, 75, 255),   qRgba(40, 155, 102, 255),
        qRgba(244, 174, 66, 255),  qRgba(125, 85, 171, 255),
        qRgba(36, 158, 183, 255),  qRgba(112, 73, 51, 255),
        qRgba(239, 119, 178, 255), qRgba(93, 105, 118, 255),
        qRgba(181, 203, 74, 255),  qRgba(22, 33, 62, 255),
        qRgba(255, 214, 102, 255), qRgba(79, 70, 229, 255),
        qRgba(15, 118, 110, 255),  qRgba(190, 24, 93, 255)};
    return colors;
}

QSize caseSize(int index, bool smoke, QRandomGenerator& random)
{
    if (smoke)
    {
        const int values[] = {64, 96, 128, 192, 256};
        const int width = values[index % 5];
        return {width, index % 3 == 0 ? width : std::max(64, width * 3 / 4)};
    }
    int width = 4096;
    if (index < 400)
    {
        const int values[] = {64, 96, 128, 192, 256};
        width = values[random.bounded(5)];
    }
    else if (index < 750)
    {
        const int values[] = {512, 640, 768, 896, 1024};
        width = values[random.bounded(5)];
    }
    else if (index < 950)
    {
        const int values[] = {1280, 1536, 1792, 2048};
        width = values[random.bounded(4)];
    }
    else if (index < 990)
    {
        const int values[] = {2560, 3072};
        width = values[random.bounded(2)];
    }
    if (index >= 990)
    {
        return {4096, 4096};
    }
    const int ratio = index % 4;
    const int height =
        ratio == 0 ? width
                   : std::max(64, width * (ratio == 1 ? 3 : 2) /
                                      (ratio == 1 ? 4 : 3));
    return {width, height};
}

QString categoryForIndex(int index)
{
    static const QStringList categories{
        QStringLiteral("大色块与色带"),
        QStringLiteral("孔洞与嵌套"),
        QStringLiteral("分叉与汇合"),
        QStringLiteral("细线与通道"),
        QStringLiteral("斜对角与棋盘格"),
        QStringLiteral("随机规则色块"),
        QStringLiteral("离散同色色块"),
        QStringLiteral("真实样例变体")};
    return categories[index % categories.size()];
}

void drawBands(QImage& image, QRandomGenerator& random)
{
    QPainter painter(&image);
    const int band_count = 2 + random.bounded(10);
    for (int index = 0; index < band_count; ++index)
    {
        const bool vertical = index % 2 == 0;
        const int begin = vertical ? image.width() * index / band_count
                                   : image.height() * index / band_count;
        const int end = vertical ? image.width() * (index + 1) / band_count
                                 : image.height() * (index + 1) / band_count;
        const QRect rectangle =
            vertical ? QRect(begin, 0, end - begin, image.height())
                     : QRect(0, begin, image.width(), end - begin);
        painter.fillRect(rectangle,
                         QColor::fromRgba(palette()[(index + 1) % 8]));
    }
}

void drawNested(QImage& image, QRandomGenerator& random)
{
    QPainter painter(&image);
    const int layers = 3 + random.bounded(8);
    QRect rectangle = image.rect();
    for (int index = 0; index < layers && rectangle.width() > 4 &&
                        rectangle.height() > 4; ++index)
    {
        painter.fillRect(rectangle,
                         QColor::fromRgba(palette()[(index + 1) % 10]));
        const int inset = std::max(
            1, std::min(rectangle.width(), rectangle.height()) /
                   (layers * 2 + 2));
        rectangle.adjust(inset, inset, -inset, -inset);
    }
}

void drawSplitMerge(QImage& image, QRandomGenerator& random)
{
    QPainter painter(&image);
    const QColor color =
        QColor::fromRgba(palette()[1 + random.bounded(8)]);
    const int branch_width = std::max(1, image.width() / 16);
    const int center_x = image.width() / 2;
    painter.fillRect(center_x - branch_width, image.height() / 6,
                     branch_width * 2, image.height() * 2 / 3, color);
    painter.fillRect(image.width() / 6, image.height() / 6,
                     image.width() * 2 / 3, branch_width, color);
    painter.fillRect(image.width() / 6,
                     image.height() * 5 / 6 - branch_width,
                     image.width() * 2 / 3, branch_width, color);
    painter.fillRect(image.width() / 5, image.height() / 2,
                     image.width() * 3 / 5, branch_width, color);
}

void drawThinCorridors(QImage& image, QRandomGenerator& random)
{
    QPainter painter(&image);
    const int line_count = 8 + random.bounded(24);
    for (int index = 0; index < line_count; ++index)
    {
        const QColor color =
            QColor::fromRgba(palette()[1 + random.bounded(12)]);
        const int thickness = 1 + random.bounded(3);
        if (index % 2 == 0)
        {
            const int y = random.bounded(image.height());
            painter.fillRect(0, y, image.width(), thickness, color);
        }
        else
        {
            const int x = random.bounded(image.width());
            painter.fillRect(x, 0, thickness, image.height(), color);
        }
    }
}

void drawChecker(QImage& image, QRandomGenerator& random)
{
    QPainter painter(&image);
    const int minimum = std::min(image.width(), image.height());
    const int cell = std::max(1, minimum / (48 + random.bounded(48)));
    const QColor first = QColor::fromRgba(palette()[1]);
    const QColor second = QColor::fromRgba(palette()[2]);
    for (int y = 0; y < image.height(); y += cell)
    {
        for (int x = 0; x < image.width(); x += cell)
        {
            painter.fillRect(x, y, cell, cell,
                             ((x / cell + y / cell) % 2 == 0)
                             ? first : second);
        }
    }
}

void drawRandomRectangles(QImage& image, QRandomGenerator& random)
{
    QPainter painter(&image);
    const int count =
        std::min(256, 12 + image.width() * image.height() / 32768);
    for (int index = 0; index < count; ++index)
    {
        const int x = random.bounded(image.width());
        const int y = random.bounded(image.height());
        const int width = 1 + random.bounded(std::max(1, image.width() / 3));
        const int height = 1 + random.bounded(std::max(1, image.height() / 3));
        painter.fillRect(x, y, width, height,
                         QColor::fromRgba(
                             palette()[1 + random.bounded(15)]));
    }
}

void drawDisconnected(QImage& image, QRandomGenerator& random)
{
    QPainter painter(&image);
    const int columns = 4 + random.bounded(12);
    const int rows = 4 + random.bounded(12);
    const int cell_width = std::max(2, image.width() / columns);
    const int cell_height = std::max(2, image.height() / rows);
    const QColor shared = QColor::fromRgba(palette()[3]);
    for (int row = 0; row < rows; ++row)
    {
        for (int column = 0; column < columns; ++column)
        {
            const int inset = 1 + (row + column) % 3;
            painter.fillRect(column * cell_width + inset,
                             row * cell_height + inset,
                             std::max(1, cell_width - inset * 2),
                             std::max(1, cell_height - inset * 2),
                             (row + column) % 3 == 0
                             ? shared
                             : QColor::fromRgba(
                                   palette()[1 + (row + column) % 12]));
        }
    }
}

void drawSourceVariant(QImage& image, int index)
{
    const QImage source(
        QStringLiteral(SGRAPH_BENCHMARK_FIXTURE_DIR "/bitmap_vector_test.png"));
    if (source.isNull())
    {
        drawBands(image, *QRandomGenerator::global());
        return;
    }
    QImage variant =
        source.scaled(image.size(), Qt::IgnoreAspectRatio,
                      Qt::FastTransformation);
    if (index % 2 != 0)
    {
        variant = variant.mirrored(true, false);
    }
    if (index % 3 == 0)
    {
        variant = variant.mirrored(false, true);
    }
    image = variant.convertToFormat(QImage::Format_ARGB32);
}

QImage generateImage(const QSize& size, const QString& category,
                     quint32 seed, int index)
{
    QRandomGenerator random(seed);
    QImage image(size, QImage::Format_ARGB32);
    image.fill(palette().front());
    if (category == QStringLiteral("大色块与色带"))
    {
        drawBands(image, random);
    }
    else if (category == QStringLiteral("孔洞与嵌套"))
    {
        drawNested(image, random);
    }
    else if (category == QStringLiteral("分叉与汇合"))
    {
        drawSplitMerge(image, random);
    }
    else if (category == QStringLiteral("细线与通道"))
    {
        drawThinCorridors(image, random);
    }
    else if (category == QStringLiteral("斜对角与棋盘格"))
    {
        drawChecker(image, random);
    }
    else if (category == QStringLiteral("随机规则色块"))
    {
        drawRandomRectangles(image, random);
    }
    else if (category == QStringLiteral("离散同色色块"))
    {
        drawDisconnected(image, random);
    }
    else
    {
        drawSourceVariant(image, index);
    }
    return image;
}

int colorCount(const QImage& image)
{
    QSet<QRgb> colors;
    for (int y = 0; y < image.height(); ++y)
    {
        const QRgb* pixels =
            reinterpret_cast<const QRgb*>(image.constScanLine(y));
        for (int x = 0; x < image.width(); ++x)
        {
            colors.insert(pixels[x]);
        }
    }
    return colors.size();
}

bool writeManifest(const QString& output_directory,
                   const std::vector<SBitmapBenchmarkCase>& cases,
                   const SBitmapBenchmarkOptions& options)
{
    QJsonObject root;
    root.insert(QStringLiteral("seed"), static_cast<qint64>(options.seed));
    root.insert(QStringLiteral("case_count"), options.case_count);
    QJsonArray entries;
    for (const SBitmapBenchmarkCase& test_case : cases)
    {
        QJsonObject entry;
        entry.insert(QStringLiteral("id"), test_case.id);
        entry.insert(QStringLiteral("category"), test_case.category);
        entry.insert(QStringLiteral("width"), test_case.width);
        entry.insert(QStringLiteral("height"), test_case.height);
        entry.insert(QStringLiteral("seed"),
                     static_cast<qint64>(test_case.seed));
        entry.insert(QStringLiteral("color_count"), test_case.color_count);
        entry.insert(QStringLiteral("file"),
                     QDir(output_directory).relativeFilePath(
                         test_case.file_path));
        entries.append(entry);
    }
    root.insert(QStringLiteral("cases"), entries);
    QFile file(QDir(output_directory).filePath(QStringLiteral("manifest.json")));
    return file.open(QIODevice::WriteOnly) &&
           file.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) >= 0;
}

} // namespace

SResult<std::vector<SBitmapBenchmarkCase>> generateBitmapBenchmarkCases(
    const SBitmapBenchmarkOptions& options)
{
    QDir output(options.output_directory);
    if (!output.mkpath(QStringLiteral("cases")))
    {
        return SResult<std::vector<SBitmapBenchmarkCase>>::failure(
            QStringLiteral("无法创建测试图片目录。"));
    }
    QDir cases_directory(output.filePath(QStringLiteral("cases")));
    std::vector<SBitmapBenchmarkCase> cases;
    cases.reserve(static_cast<std::size_t>(options.case_count));
    QRandomGenerator size_random(options.seed);
    for (int index = 0; index < options.case_count; ++index)
    {
        const quint32 case_seed =
            options.seed ^ (0x9E3779B9U * static_cast<quint32>(index + 1));
        const QSize size = caseSize(index, options.smoke, size_random);
        const QString category = categoryForIndex(index);
        const QString file_name =
            QStringLiteral("case_%1_%2_%3x%4_%5.png")
                .arg(index + 1, 4, 10, QLatin1Char('0'))
                .arg(category)
                .arg(size.width())
                .arg(size.height())
                .arg(case_seed);
        const QString file_path = cases_directory.filePath(file_name);
        QImage image;
        if (QFileInfo::exists(file_path))
        {
            image.load(file_path);
        }
        if (image.size() != size)
        {
            image = generateImage(size, category, case_seed, index);
            if (!image.save(file_path, "PNG"))
            {
                return SResult<std::vector<SBitmapBenchmarkCase>>::failure(
                    QStringLiteral("无法保存测试图片：%1").arg(file_path));
            }
        }
        cases.push_back({index + 1, category, size.width(), size.height(),
                         case_seed, colorCount(image), file_path});
    }
    if (!writeManifest(options.output_directory, cases, options))
    {
        return SResult<std::vector<SBitmapBenchmarkCase>>::failure(
            QStringLiteral("无法写入测试清单 manifest.json。"));
    }
    return SResult<std::vector<SBitmapBenchmarkCase>>::success(std::move(cases));
}

} // namespace smartGraphics
