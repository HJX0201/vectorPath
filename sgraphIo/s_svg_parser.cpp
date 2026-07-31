#include "s_svg_parser.h"

#include "s_svg_path_parser.h"

#include <QLineF>
#include <QPainterPath>
#include <QRegularExpression>
#include <QSet>
#include <QTransform>
#include <QXmlStreamReader>
#include <algorithm>
#include <cmath>
#include <optional>

namespace smartCam
{
namespace
{

struct SSvgStyle
{
    QColor fill{0, 0, 0};
    QColor stroke;
    double opacity = 1.0;
    double fill_opacity = 1.0;
    double stroke_opacity = 1.0;
    bool is_visible = true;
    SVectorFillRule fill_rule = SVectorFillRule::NonZero;
};

struct SSvgContext
{
    SSvgStyle style;
    QTransform transform;
};

struct SSvgViewport
{
    double minimum_x = 0.0;
    double minimum_y = 0.0;
    double width = 0.0;
    double height = 0.0;
};

double number(const QString& value, double fallback = 0.0)
{
    static const QRegularExpression pattern(
        QStringLiteral(R"(^\s*([+-]?(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?))"));
    const QRegularExpressionMatch match = pattern.match(value);
    if (!match.hasMatch())
    {
        return fallback;
    }
    bool is_valid = false;
    const double result = match.captured(1).toDouble(&is_valid);
    return is_valid ? result : fallback;
}

std::vector<double> numbers(const QString& value)
{
    static const QRegularExpression pattern(
        QStringLiteral(R"([+-]?(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?)"));
    std::vector<double> result;
    QRegularExpressionMatchIterator iterator = pattern.globalMatch(value);
    while (iterator.hasNext())
    {
        result.push_back(iterator.next().captured().toDouble());
    }
    return result;
}

QColor parseColor(const QString& text, QSet<QString>& warning_keys, QStringList& warnings)
{
    const QString value = text.trimmed();
    if (value.isEmpty() || value.compare(QStringLiteral("none"), Qt::CaseInsensitive) == 0)
    {
        return {};
    }
    if (value.startsWith(QStringLiteral("url("), Qt::CaseInsensitive))
    {
        if (!warning_keys.contains(QStringLiteral("paint_server")))
        {
            warning_keys.insert(QStringLiteral("paint_server"));
            warnings.append(QObject::tr("SVG 渐变或图案已按黑色纯色几何导入。"));
        }
        return QColor(Qt::black);
    }
    static const QRegularExpression rgb_pattern(
        QStringLiteral(R"(^rgba?\s*\(([^)]+)\)$)"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch rgb_match = rgb_pattern.match(value);
    if (rgb_match.hasMatch())
    {
        const QStringList components = rgb_match.captured(1).split(QLatin1Char(','));
        if (components.size() >= 3)
        {
            const auto channel = [](const QString& component)
            {
                const QString trimmed = component.trimmed();
                if (trimmed.endsWith(QLatin1Char('%')))
                {
                    return qBound(0, qRound(number(trimmed) * 2.55), 255);
                }
                return qBound(0, qRound(number(trimmed)), 255);
            };
            QColor color(channel(components[0]), channel(components[1]), channel(components[2]));
            if (components.size() == 4)
            {
                const double alpha = number(components[3], 1.0);
                color.setAlpha(alpha <= 1.0 ? qRound(alpha * 255.0)
                                            : qBound(0, qRound(alpha), 255));
            }
            return color;
        }
    }
    QColor color(value);
    if (!color.isValid() && !warning_keys.contains(QStringLiteral("color")))
    {
        warning_keys.insert(QStringLiteral("color"));
        warnings.append(QObject::tr("SVG 中存在无法识别的颜色，已按黑色处理。"));
        return QColor(Qt::black);
    }
    return color;
}

void applyOpacity(QColor& color, double opacity)
{
    if (color.isValid())
    {
        color.setAlpha(qBound(0, qRound(color.alpha() * qBound(0.0, opacity, 1.0)), 255));
    }
}

QHash<QString, QString> inlineStyle(const QString& text)
{
    QHash<QString, QString> result;
    for (const QString& declaration :
         text.split(QLatin1Char(';'), QString::SkipEmptyParts))
    {
        const int separator = declaration.indexOf(QLatin1Char(':'));
        if (separator > 0)
        {
            result.insert(declaration.left(separator).trimmed().toLower(),
                          declaration.mid(separator + 1).trimmed());
        }
    }
    return result;
}

SSvgStyle elementStyle(const QXmlStreamAttributes& attributes, const SSvgStyle& inherited,
                       QSet<QString>& warning_keys, QStringList& warnings)
{
    SSvgStyle result = inherited;
    QHash<QString, QString> properties;
    const QStringList names{QStringLiteral("fill"),         QStringLiteral("stroke"),
                            QStringLiteral("opacity"),      QStringLiteral("fill-opacity"),
                            QStringLiteral("stroke-opacity"), QStringLiteral("fill-rule"),
                            QStringLiteral("display"),      QStringLiteral("visibility")};
    for (const QString& name : names)
    {
        if (attributes.hasAttribute(name))
        {
            properties.insert(name, attributes.value(name).toString());
        }
    }
    const QHash<QString, QString> style_values =
        inlineStyle(attributes.value(QStringLiteral("style")).toString());
    for (auto iterator = style_values.constBegin(); iterator != style_values.constEnd(); ++iterator)
    {
        properties.insert(iterator.key(), iterator.value());
    }
    if (properties.contains(QStringLiteral("fill")))
    {
        result.fill = parseColor(properties.value(QStringLiteral("fill")), warning_keys, warnings);
    }
    if (properties.contains(QStringLiteral("stroke")))
    {
        result.stroke =
            parseColor(properties.value(QStringLiteral("stroke")), warning_keys, warnings);
    }
    if (properties.contains(QStringLiteral("opacity")))
    {
        result.opacity *= number(properties.value(QStringLiteral("opacity")), 1.0);
    }
    if (properties.contains(QStringLiteral("fill-opacity")))
    {
        result.fill_opacity *= number(properties.value(QStringLiteral("fill-opacity")), 1.0);
    }
    if (properties.contains(QStringLiteral("stroke-opacity")))
    {
        result.stroke_opacity *= number(properties.value(QStringLiteral("stroke-opacity")), 1.0);
    }
    if (properties.value(QStringLiteral("fill-rule")).compare(
            QStringLiteral("evenodd"), Qt::CaseInsensitive) == 0)
    {
        result.fill_rule = SVectorFillRule::EvenOdd;
    }
    if (properties.value(QStringLiteral("display")).compare(
            QStringLiteral("none"), Qt::CaseInsensitive) == 0 ||
        properties.value(QStringLiteral("visibility")).compare(
            QStringLiteral("hidden"), Qt::CaseInsensitive) == 0)
    {
        result.is_visible = false;
    }
    return result;
}

QTransform svgTransform(const QString& text)
{
    static const QRegularExpression operation_pattern(
        QStringLiteral(R"(([A-Za-z]+)\s*\(([^)]*)\))"));
    QTransform result;
    QRegularExpressionMatchIterator iterator = operation_pattern.globalMatch(text);
    while (iterator.hasNext())
    {
        const QRegularExpressionMatch match = iterator.next();
        const QString operation = match.captured(1).toLower();
        const std::vector<double> values = numbers(match.captured(2));
        QTransform transform;
        if (operation == QLatin1String("matrix") && values.size() >= 6)
        {
            transform = QTransform(values[0], values[1], values[2], values[3], values[4],
                                   values[5]);
        }
        else if (operation == QLatin1String("translate") && !values.empty())
        {
            transform.translate(values[0], values.size() > 1 ? values[1] : 0.0);
        }
        else if (operation == QLatin1String("scale") && !values.empty())
        {
            transform.scale(values[0], values.size() > 1 ? values[1] : values[0]);
        }
        else if (operation == QLatin1String("rotate") && !values.empty())
        {
            if (values.size() > 2)
            {
                transform.translate(values[1], values[2]);
                transform.rotate(values[0]);
                transform.translate(-values[1], -values[2]);
            }
            else
            {
                transform.rotate(values[0]);
            }
        }
        else if (operation == QLatin1String("skewx") && !values.empty())
        {
            transform.shear(std::tan(values[0] * 3.14159265358979323846 / 180.0), 0.0);
        }
        else if (operation == QLatin1String("skewy") && !values.empty())
        {
            transform.shear(0.0, std::tan(values[0] * 3.14159265358979323846 / 180.0));
        }
        result = transform * result;
    }
    return result;
}

std::vector<QPointF> pointList(const QString& text)
{
    const std::vector<double> values = numbers(text);
    std::vector<QPointF> result;
    for (std::size_t index = 1; index < values.size(); index += 2)
    {
        result.emplace_back(values[index - 1], values[index]);
    }
    return result;
}

std::vector<SSvgSubpath> elementPaths(const QString& name,
                                      const QXmlStreamAttributes& attributes,
                                      QString& error_message)
{
    std::vector<SSvgSubpath> result;
    QPainterPath path;
    if (name == QLatin1String("path"))
    {
        const auto parsed = parseSvgPathData(attributes.value(QStringLiteral("d")).toString());
        if (!parsed)
        {
            error_message = parsed.errorMessage();
            return {};
        }
        return parsed.value();
    }
    if (name == QLatin1String("line"))
    {
        path.moveTo(number(attributes.value(QStringLiteral("x1")).toString()),
                    number(attributes.value(QStringLiteral("y1")).toString()));
        path.lineTo(number(attributes.value(QStringLiteral("x2")).toString()),
                    number(attributes.value(QStringLiteral("y2")).toString()));
    }
    else if (name == QLatin1String("rect"))
    {
        const QRectF rectangle(number(attributes.value(QStringLiteral("x")).toString()),
                               number(attributes.value(QStringLiteral("y")).toString()),
                               number(attributes.value(QStringLiteral("width")).toString()),
                               number(attributes.value(QStringLiteral("height")).toString()));
        const double radius_x = number(attributes.value(QStringLiteral("rx")).toString());
        const double radius_y = number(attributes.value(QStringLiteral("ry")).toString(),
                                       radius_x);
        if (radius_x > 0.0 || radius_y > 0.0)
        {
            path.addRoundedRect(rectangle, radius_x, radius_y);
        }
        else
        {
            path.addRect(rectangle);
        }
    }
    else if (name == QLatin1String("circle"))
    {
        const double center_x = number(attributes.value(QStringLiteral("cx")).toString());
        const double center_y = number(attributes.value(QStringLiteral("cy")).toString());
        const double radius = number(attributes.value(QStringLiteral("r")).toString());
        path.addEllipse(QPointF(center_x, center_y), radius, radius);
    }
    else if (name == QLatin1String("ellipse"))
    {
        const double center_x = number(attributes.value(QStringLiteral("cx")).toString());
        const double center_y = number(attributes.value(QStringLiteral("cy")).toString());
        const double radius_x = number(attributes.value(QStringLiteral("rx")).toString());
        const double radius_y = number(attributes.value(QStringLiteral("ry")).toString());
        path.addEllipse(QPointF(center_x, center_y), radius_x, radius_y);
    }
    else if (name == QLatin1String("polyline") || name == QLatin1String("polygon"))
    {
        const std::vector<QPointF> points =
            pointList(attributes.value(QStringLiteral("points")).toString());
        if (!points.empty())
        {
            path.moveTo(points.front());
            for (std::size_t index = 1; index < points.size(); ++index)
            {
                path.lineTo(points[index]);
            }
            if (name == QLatin1String("polygon"))
            {
                path.closeSubpath();
            }
        }
    }
    if (path.elementCount() > 1)
    {
        result.push_back({path, name != QLatin1String("line") &&
                                   name != QLatin1String("polyline")});
    }
    return result;
}

std::vector<SPoint2d> flattenedPoints(const QPainterPath& path,
                                      const QTransform& transform)
{
    const QList<QPolygonF> polygons = transform.map(path).toSubpathPolygons();
    if (polygons.isEmpty())
    {
        return {};
    }
    std::vector<SPoint2d> result;
    result.reserve(static_cast<std::size_t>(polygons.front().size()));
    for (const QPointF& point : polygons.front())
    {
        if (result.empty() || QLineF(result.back().toPointF(), point).length() > 1.0e-8)
        {
            result.push_back(SPoint2d::fromPointF(point));
        }
    }
    return result;
}

QColor outlineColor(const SSvgStyle& style)
{
    QColor result = style.stroke.isValid() ? style.stroke : style.fill;
    if (!result.isValid())
    {
        result = QColor(Qt::black);
    }
    applyOpacity(result, style.opacity *
                             (style.stroke.isValid() ? style.stroke_opacity
                                                     : style.fill_opacity));
    return result;
}

QColor regionColor(const SSvgStyle& style)
{
    QColor result = style.fill.isValid() ? style.fill : style.stroke;
    if (!result.isValid())
    {
        result = QColor(Qt::black);
    }
    applyOpacity(result, style.opacity *
                             (style.fill.isValid() ? style.fill_opacity
                                                   : style.stroke_opacity));
    return result;
}

void normalizeCoordinates(SSvgVectorData& data, SSvgViewport viewport)
{
    double maximum_x = 0.0;
    double maximum_y = 0.0;
    bool has_point = false;
    const auto inspect = [&](const std::vector<SPoint2d>& points)
    {
        for (const SPoint2d& point : points)
        {
            maximum_x = has_point ? std::max(maximum_x, point.x) : point.x;
            maximum_y = has_point ? std::max(maximum_y, point.y) : point.y;
            has_point = true;
        }
    };
    for (const SVectorOutline& outline : data.outlines)
    {
        inspect(outline.points);
    }
    if (viewport.width <= 0.0)
    {
        viewport.width = maximum_x - viewport.minimum_x;
    }
    if (viewport.height <= 0.0)
    {
        viewport.height = maximum_y - viewport.minimum_y;
    }
    const auto normalize = [&](std::vector<SPoint2d>& points)
    {
        for (SPoint2d& point : points)
        {
            point.x -= viewport.minimum_x;
            point.y = viewport.minimum_y + viewport.height - point.y;
        }
    };
    for (SVectorOutline& outline : data.outlines)
    {
        normalize(outline.points);
    }
    for (SVectorRegion& region : data.regions)
    {
        for (std::vector<SPoint2d>& contour : region.contours)
        {
            normalize(contour);
        }
    }
    data.source_width = std::max(0.0, viewport.width);
    data.source_height = std::max(0.0, viewport.height);
}

} // namespace

SResult<SSvgVectorData> parseSvgVectorData(const QByteArray& svg_data)
{
    if (svg_data.trimmed().isEmpty())
    {
        return SResult<SSvgVectorData>::failure(QStringLiteral("SVG 数据为空。"));
    }
    QXmlStreamReader reader(svg_data);
    SSvgVectorData result;
    SSvgViewport viewport;
    std::vector<SSvgContext> contexts{{}};
    QSet<QString> warning_keys;
    int root_depth = 0;
    const QSet<QString> geometry_elements{
        QStringLiteral("path"),    QStringLiteral("line"),    QStringLiteral("rect"),
        QStringLiteral("circle"),  QStringLiteral("ellipse"), QStringLiteral("polyline"),
        QStringLiteral("polygon")};
    const QSet<QString> unsupported_elements{
        QStringLiteral("text"), QStringLiteral("image"), QStringLiteral("use"),
        QStringLiteral("filter"), QStringLiteral("mask"), QStringLiteral("clipPath")};

    while (!reader.atEnd())
    {
        reader.readNext();
        if (reader.isStartElement())
        {
            const QString name = reader.name().toString();
            const QXmlStreamAttributes attributes = reader.attributes();
            SSvgContext context = contexts.back();
            context.style =
                elementStyle(attributes, context.style, warning_keys, result.warnings);
            context.transform =
                svgTransform(attributes.value(QStringLiteral("transform")).toString()) *
                context.transform;
            contexts.push_back(context);
            ++root_depth;
            if (name == QLatin1String("svg") && root_depth == 1)
            {
                const std::vector<double> view_box =
                    numbers(attributes.value(QStringLiteral("viewBox")).toString());
                if (view_box.size() >= 4)
                {
                    viewport = {view_box[0], view_box[1], view_box[2], view_box[3]};
                }
                else
                {
                    viewport.width =
                        number(attributes.value(QStringLiteral("width")).toString());
                    viewport.height =
                        number(attributes.value(QStringLiteral("height")).toString());
                }
            }
            if (unsupported_elements.contains(name) &&
                !warning_keys.contains(QStringLiteral("unsupported_") + name))
            {
                warning_keys.insert(QStringLiteral("unsupported_") + name);
                result.warnings.append(QObject::tr("SVG 元素 <%1> 未转换，已跳过。").arg(name));
            }
            if (!context.style.is_visible || !geometry_elements.contains(name))
            {
                continue;
            }
            QString path_error;
            const std::vector<SSvgSubpath> subpaths =
                elementPaths(name, attributes, path_error);
            if (!path_error.isEmpty())
            {
                return SResult<SSvgVectorData>::failure(path_error);
            }
            SVectorRegion region;
            region.color = regionColor(context.style);
            region.fill_rule = context.style.fill_rule;
            for (const SSvgSubpath& subpath : subpaths)
            {
                std::vector<SPoint2d> points =
                    flattenedPoints(subpath.path, context.transform);
                if (points.size() < 2)
                {
                    continue;
                }
                if (subpath.is_closed && points.front().x == points.back().x &&
                    points.front().y == points.back().y)
                {
                    points.pop_back();
                }
                result.outlines.push_back(
                    {points, subpath.is_closed, outlineColor(context.style)});
                if (subpath.is_closed && points.size() >= 3)
                {
                    region.contours.push_back(std::move(points));
                }
            }
            if (!region.contours.empty())
            {
                result.regions.push_back(std::move(region));
            }
        }
        else if (reader.isEndElement())
        {
            if (contexts.size() > 1)
            {
                contexts.pop_back();
            }
            --root_depth;
        }
    }
    if (reader.hasError())
    {
        return SResult<SSvgVectorData>::failure(
            QStringLiteral("SVG XML 解析失败：%1").arg(reader.errorString()));
    }
    if (result.outlines.empty())
    {
        return SResult<SSvgVectorData>::failure(QStringLiteral("SVG 中没有可导入的矢量几何。"));
    }
    normalizeCoordinates(result, viewport);
    return SResult<SSvgVectorData>::success(std::move(result));
}

} // namespace smartCam
