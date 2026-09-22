#include "vp_plot_style_table.h"

#include "vp_qt_text.h"

#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QtEndian>
#include <algorithm>
#include <cmath>
#include <limits>

namespace Vp
{
namespace
{

constexpr int kObjectColor = -1;
constexpr int kObjectColorAlternate = -1006632961;
constexpr int kGrayscalePolicy = 2;
const QByteArray kPlotStyleSignature("PIAFILEVERSION_2.0,CTBVER1,compress\r\npmzlibcodec");

struct VpParsedStyle
{
    VpPlotStyleRecord record;
    int line_weight_index = 0;
    qint64 encoded_color = kObjectColor;
    qint64 mode_color = kObjectColor;
};

QString propertyValue(const QString& line)
{
    QString value = line.section(QLatin1Char('='), 1).trimmed();
    if (value.startsWith(QLatin1Char('"')))
    {
        value.remove(0, 1);
    }
    const int appendix_index = value.indexOf(QStringLiteral(" ("));
    return appendix_index > 0 ? value.left(appendix_index) : value;
}

bool parseBoolean(const QString& value)
{
    return value.compare(QLatin1String("TRUE"), Qt::CaseInsensitive) == 0;
}

QColor decodeColor(qint64 encoded_color)
{
    const quint32 value = static_cast<quint32>(encoded_color);
    return QColor(static_cast<int>((value >> 16) & 0xff), static_cast<int>((value >> 8) & 0xff),
                  static_cast<int>(value & 0xff));
}

QByteArray decompressedPlotStyleData(const QByteArray& file_data)
{
    if (file_data.size() <= 60 || !file_data.startsWith(kPlotStyleSignature))
    {
        return {};
    }
    const quint32 uncompressed_size =
        qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(file_data.constData() + 52));
    QByteArray qt_compressed(4, Qt::Uninitialized);
    qToBigEndian<quint32>(uncompressed_size, reinterpret_cast<uchar*>(qt_compressed.data()));
    qt_compressed.append(file_data.mid(60));
    QByteArray result = qUncompress(qt_compressed);
    if (result.endsWith('\0'))
    {
        result.chop(1);
    }
    return result;
}

void parseStyleProperty(VpParsedStyle& style, const QString& line)
{
    const QString key = line.section(QLatin1Char('='), 0, 0).trimmed();
    const QString value = propertyValue(line);
    if (key == QLatin1String("name"))
    {
        style.record.name = value;
    }
    else if (key == QLatin1String("color"))
    {
        style.encoded_color = value.toLongLong();
    }
    else if (key == QLatin1String("mode_color"))
    {
        style.mode_color = value.toLongLong();
    }
    else if (key == QLatin1String("color_policy"))
    {
        const int policy = value.toInt();
        style.record.dithering = (policy & 1) != 0;
        style.record.grayscale = (policy & kGrayscalePolicy) != 0;
    }
    else if (key == QLatin1String("screen"))
    {
        style.record.screen_percentage = std::clamp(value.toInt(), 0, 100);
    }
    else if (key == QLatin1String("lineweight"))
    {
        style.line_weight_index = std::max(0, value.toInt());
    }
}

} // namespace

VpResult<VpPlotStyleTable> parsePlotStyleData(const QByteArray& data, const QString& file_path)
{
    const QString text = QString::fromLatin1(data);
    const QStringList lines = text.split(QLatin1Char('\n'));
    bool is_color_dependent = true;
    QString description;
    bool in_plot_styles = false;
    bool in_line_weights = false;
    bool in_style = false;
    VpParsedStyle current_style;
    std::vector<VpParsedStyle> parsed_styles;
    QHash<int, double> line_weights;
    for (QString line : lines)
    {
        line = line.trimmed();
        if (line.isEmpty())
        {
            continue;
        }
        if (in_style)
        {
            if (line == QLatin1String("}"))
            {
                parsed_styles.push_back(current_style);
                in_style = false;
            }
            else if (line.contains(QLatin1Char('=')))
            {
                parseStyleProperty(current_style, line);
            }
            continue;
        }
        if (in_plot_styles)
        {
            if (line == QLatin1String("}"))
            {
                in_plot_styles = false;
                continue;
            }
            if (line.endsWith(QLatin1Char('{')))
            {
                bool is_valid = false;
                const int index = line.left(line.size() - 1).trimmed().toInt(&is_valid);
                if (is_valid)
                {
                    current_style = {};
                    current_style.record.index = index;
                    in_style = true;
                }
            }
            continue;
        }
        if (in_line_weights)
        {
            if (line == QLatin1String("}"))
            {
                in_line_weights = false;
                continue;
            }
            bool is_valid = false;
            const int index = line.section(QLatin1Char('='), 0, 0).trimmed().toInt(&is_valid);
            const double width = propertyValue(line).toDouble();
            if (is_valid && width >= 0.0)
            {
                line_weights.insert(index, width);
            }
            continue;
        }
        if (line == QLatin1String("plot_style{"))
        {
            in_plot_styles = true;
        }
        else if (line == QLatin1String("custom_lineweight_table{"))
        {
            in_line_weights = true;
        }
        else if (line.startsWith(QLatin1String("description=")))
        {
            description = propertyValue(line);
        }
        else if (line.startsWith(QLatin1String("aci_table_available=")))
        {
            is_color_dependent = parseBoolean(propertyValue(line));
        }
    }
    if (parsed_styles.empty())
    {
        return VpResult<VpPlotStyleTable>::failure(
            toCoreText(QObject::tr("打印样式表不包含可用样式：%1").arg(file_path)));
    }
    VpPlotStyleTable table;
    table.m_type =
        is_color_dependent ? VpPlotStyleTableType::ColorDependent : VpPlotStyleTableType::Named;
    table.m_description = description;
    table.m_styles.reserve(parsed_styles.size());
    for (VpParsedStyle& parsed_style : parsed_styles)
    {
        parsed_style.record.use_object_color = parsed_style.encoded_color == kObjectColor ||
                                               parsed_style.encoded_color == kObjectColorAlternate;
        if (!parsed_style.record.use_object_color)
        {
            const qint64 color_value = parsed_style.mode_color == kObjectColor
                                           ? parsed_style.encoded_color
                                           : parsed_style.mode_color;
            parsed_style.record.color = decodeColor(color_value);
        }
        parsed_style.record.line_width_mm = line_weights.value(parsed_style.line_weight_index, 0.0);
        if (parsed_style.record.line_width_mm <= 0.0)
        {
            parsed_style.record.line_width_mm = -1.0;
        }
        table.m_styles.push_back(std::move(parsed_style.record));
    }
    return VpResult<VpPlotStyleTable>::success(std::move(table));
}

namespace
{

QColor aciColor(int aci)
{
    static const QColor basic_colors[]{QColor(),
                                       QColor(255, 0, 0),
                                       QColor(255, 255, 0),
                                       QColor(0, 255, 0),
                                       QColor(0, 255, 255),
                                       QColor(0, 0, 255),
                                       QColor(255, 0, 255),
                                       QColor(255, 255, 255),
                                       QColor(128, 128, 128),
                                       QColor(192, 192, 192)};
    if (aci >= 1 && aci <= 9)
    {
        return basic_colors[aci];
    }
    if (aci >= 10 && aci <= 249)
    {
        const int offset = aci - 10;
        const int hue = (offset / 10) * 15;
        const int shade = (offset % 10) / 2;
        const int values[]{255, 165, 127, 76, 38};
        const int saturation = (offset % 2) == 0 ? 255 : 128;
        return QColor::fromHsv(hue, saturation, values[shade]);
    }
    const int gray_values[]{51, 80, 105, 130, 190, 255};
    const int gray_index = std::clamp(aci - 250, 0, 5);
    return QColor(gray_values[gray_index], gray_values[gray_index], gray_values[gray_index]);
}

int nearestAci(const QColor& color)
{
    int nearest_index = 7;
    qint64 nearest_distance = std::numeric_limits<qint64>::max();
    for (int index = 1; index <= 255; ++index)
    {
        const QColor candidate = aciColor(index);
        const qint64 red_delta = color.red() - candidate.red();
        const qint64 green_delta = color.green() - candidate.green();
        const qint64 blue_delta = color.blue() - candidate.blue();
        const qint64 distance =
            red_delta * red_delta + green_delta * green_delta + blue_delta * blue_delta;
        if (distance < nearest_distance)
        {
            nearest_distance = distance;
            nearest_index = index;
        }
    }
    return nearest_index;
}

} // namespace

VpResult<VpPlotStyleTable> VpPlotStyleTable::load(const QString& file_path)
{
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly))
    {
        return VpResult<VpPlotStyleTable>::failure(
            toCoreText(QObject::tr("无法读取打印样式表：%1").arg(file.errorString())));
    }
    const QByteArray data = decompressedPlotStyleData(file.readAll());
    if (data.isEmpty())
    {
        return VpResult<VpPlotStyleTable>::failure(
            toCoreText(QObject::tr("打印样式表头或压缩数据无效：%1").arg(file_path)));
    }
    return parsePlotStyleData(data, file_path);
}

VpResult<VpPlotStyleTable> VpPlotStyleTable::fromReference(const QString& reference)
{
    if (reference.compare(QLatin1String("monochrome.ctb"), Qt::CaseInsensitive) == 0)
    {
        return VpResult<VpPlotStyleTable>::success(monochromeTable());
    }
    if (reference.compare(QLatin1String("grayscale.ctb"), Qt::CaseInsensitive) == 0)
    {
        return VpResult<VpPlotStyleTable>::success(grayscaleTable());
    }
    if (reference.compare(QLatin1String("vectorPath.stb"), Qt::CaseInsensitive) == 0 ||
        reference.compare(QLatin1String("smartCam.stb"), Qt::CaseInsensitive) == 0 ||
        reference.compare(QLatin1String("smartCad.stb"), Qt::CaseInsensitive) == 0)
    {
        VpPlotStyleTable table;
        table.m_type = VpPlotStyleTableType::Named;
        table.m_description = QObject::tr("vectorPath 默认命名打印样式");
        table.m_styles.push_back({0, QStringLiteral("Normal")});
        return VpResult<VpPlotStyleTable>::success(std::move(table));
    }
    return load(reference);
}

VpPlotStyleTableType VpPlotStyleTable::type() const noexcept
{
    return m_type;
}

QString VpPlotStyleTable::description() const
{
    return m_description;
}

const std::vector<VpPlotStyleRecord>& VpPlotStyleTable::styles() const noexcept
{
    return m_styles;
}

const VpPlotStyleRecord* VpPlotStyleTable::styleForColor(const QColor& source_color) const noexcept
{
    if (m_styles.empty())
    {
        return nullptr;
    }
    if (m_type == VpPlotStyleTableType::Named)
    {
        const auto iterator = std::find_if(m_styles.begin(), m_styles.end(),
                                           [](const VpPlotStyleRecord& style)
                                           {
                                               return style.name.compare(QLatin1String("Normal"),
                                                                         Qt::CaseInsensitive) == 0;
                                           });
        return iterator == m_styles.end() ? &m_styles.front() : &*iterator;
    }
    const int style_index = nearestAci(source_color) - 1;
    const auto iterator = std::find_if(m_styles.begin(), m_styles.end(),
                                       [style_index](const VpPlotStyleRecord& style)
                                       {
                                           return style.index == style_index;
                                       });
    return iterator == m_styles.end() ? nullptr : &*iterator;
}

QColor VpPlotStyleTable::mappedColor(const QColor& source_color) const
{
    QColor result = source_color;
    const VpPlotStyleRecord* style = styleForColor(source_color);
    if (style && !style->use_object_color && style->color.isValid())
    {
        result = style->color;
    }
    if (m_force_monochrome)
    {
        result = Qt::black;
    }
    else if (m_force_grayscale || (style && style->grayscale))
    {
        const int gray_value = qGray(result.rgb());
        result = QColor(gray_value, gray_value, gray_value);
    }
    const int screening = style ? style->screen_percentage : 100;
    if (screening < 100)
    {
        result.setRed(255 - ((255 - result.red()) * screening / 100));
        result.setGreen(255 - ((255 - result.green()) * screening / 100));
        result.setBlue(255 - ((255 - result.blue()) * screening / 100));
    }
    result.setAlpha(source_color.alpha());
    return result;
}

double VpPlotStyleTable::mappedLineWidth(const QColor& source_color,
                                         double object_width_mm) const noexcept
{
    const VpPlotStyleRecord* style = styleForColor(source_color);
    return style && style->line_width_mm > 0.0 ? style->line_width_mm : object_width_mm;
}

VpPlotStyleTable VpPlotStyleTable::monochromeTable()
{
    VpPlotStyleTable table;
    table.m_description = QObject::tr("所有颜色打印为黑色");
    table.m_force_monochrome = true;
    return table;
}

VpPlotStyleTable VpPlotStyleTable::grayscaleTable()
{
    VpPlotStyleTable table;
    table.m_description = QObject::tr("所有颜色打印为灰度");
    table.m_force_grayscale = true;
    return table;
}

} // namespace Vp
