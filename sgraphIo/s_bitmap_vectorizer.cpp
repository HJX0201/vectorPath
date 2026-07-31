#include "s_bitmap_vectorizer.h"

#include <QMap>
#include <QString>
#include <QTextStream>

namespace smartCam
{
namespace
{

QString svgColor(QRgb color)
{
    return QStringLiteral("#%1%2%3")
        .arg(qRed(color), 2, 16, QLatin1Char('0'))
        .arg(qGreen(color), 2, 16, QLatin1Char('0'))
        .arg(qBlue(color), 2, 16, QLatin1Char('0'))
        .toUpper();
}

} // namespace

QByteArray bitmapContoursToSvgData(
    int width, int height, const std::vector<SBitmapContour>& contours)
{
    QMap<quint32, std::vector<const SBitmapContour*>> color_contours;
    for (const SBitmapContour& contour : contours)
    {
        color_contours[contour.color].push_back(&contour);
    }

    QString svg;
    QTextStream stream(&svg);
    stream << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << width
           << "\" height=\"" << height << "\" viewBox=\"0 0 " << width << ' '
           << height << "\" shape-rendering=\"crispEdges\">\n";
    for (auto iterator = color_contours.cbegin();
         iterator != color_contours.cend(); ++iterator)
    {
        stream << "  <path fill=\"" << svgColor(iterator.key())
               << "\" fill-opacity=\""
               << QString::number(qAlpha(iterator.key()) / 255.0, 'g', 8)
               << "\" fill-rule=\"evenodd\" d=\"";
        for (const SBitmapContour* contour : iterator.value())
        {
            if (contour->points.size() < 3)
            {
                continue;
            }
            stream << 'M' << contour->points.front().x() << ' '
                   << contour->points.front().y();
            for (std::size_t index = 1; index < contour->points.size(); ++index)
            {
                stream << 'L' << contour->points[index].x() << ' '
                       << contour->points[index].y();
            }
            stream << 'Z';
        }
        stream << "\"/>\n";
    }
    stream << "</svg>\n";
    return svg.toUtf8();
}

SResult<QByteArray> bitmapToSvgData(const QImage& source,
                                    const SBitmapVectorSettings& settings)
{
    SResult<SBitmapVectorResult> result =
        bitmapToVectorResult(source, settings);
    if (!result)
    {
        return SResult<QByteArray>::failure(result.errorMessage());
    }
    return SResult<QByteArray>::success(std::move(result.value().svg_data));
}

} // namespace smartCam
