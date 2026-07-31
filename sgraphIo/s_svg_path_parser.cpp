#include "s_svg_path_parser.h"

#include <QPointF>
#include <algorithm>
#include <cmath>

namespace vectorPath
{
namespace
{

constexpr double kPi = 3.14159265358979323846;

class SPathReader final
{
  public:
    explicit SPathReader(const QString& text) : m_text(text)
    {
    }

    bool atEnd()
    {
        skipSeparators();
        return m_index >= m_text.size();
    }

    bool nextIsCommand()
    {
        skipSeparators();
        return m_index < m_text.size() && m_text.at(m_index).isLetter();
    }

    QChar readCommand()
    {
        skipSeparators();
        return m_index < m_text.size() ? m_text.at(m_index++) : QChar();
    }

    bool readNumber(double& result)
    {
        skipSeparators();
        if (m_index >= m_text.size())
        {
            return false;
        }
        const int start = m_index;
        if (m_text.at(m_index) == QLatin1Char('+') ||
            m_text.at(m_index) == QLatin1Char('-'))
        {
            ++m_index;
        }
        bool has_digit = false;
        while (m_index < m_text.size() && m_text.at(m_index).isDigit())
        {
            has_digit = true;
            ++m_index;
        }
        if (m_index < m_text.size() && m_text.at(m_index) == QLatin1Char('.'))
        {
            ++m_index;
            while (m_index < m_text.size() && m_text.at(m_index).isDigit())
            {
                has_digit = true;
                ++m_index;
            }
        }
        if (!has_digit)
        {
            m_index = start;
            return false;
        }
        if (m_index < m_text.size() &&
            (m_text.at(m_index) == QLatin1Char('e') ||
             m_text.at(m_index) == QLatin1Char('E')))
        {
            const int exponent_start = m_index++;
            if (m_index < m_text.size() &&
                (m_text.at(m_index) == QLatin1Char('+') ||
                 m_text.at(m_index) == QLatin1Char('-')))
            {
                ++m_index;
            }
            bool has_exponent_digit = false;
            while (m_index < m_text.size() && m_text.at(m_index).isDigit())
            {
                has_exponent_digit = true;
                ++m_index;
            }
            if (!has_exponent_digit)
            {
                m_index = exponent_start;
            }
        }
        bool is_valid = false;
        result = m_text.mid(start, m_index - start).toDouble(&is_valid);
        return is_valid;
    }

  private:
    void skipSeparators()
    {
        while (m_index < m_text.size())
        {
            const QChar character = m_text.at(m_index);
            if (!character.isSpace() && character != QLatin1Char(','))
            {
                break;
            }
            ++m_index;
        }
    }

    QString m_text;
    int m_index = 0;
};

double vectorAngle(double ux, double uy, double vx, double vy)
{
    const double dot = ux * vx + uy * vy;
    const double length = std::hypot(ux, uy) * std::hypot(vx, vy);
    if (length <= 1.0e-12)
    {
        return 0.0;
    }
    const double magnitude = std::acos(std::clamp(dot / length, -1.0, 1.0));
    return ux * vy - uy * vx < 0.0 ? -magnitude : magnitude;
}

void appendSvgArc(QPainterPath& path, const QPointF& start, const QPointF& end,
                  double radius_x, double radius_y, double rotation_degrees,
                  bool large_arc, bool sweep)
{
    radius_x = std::abs(radius_x);
    radius_y = std::abs(radius_y);
    if (radius_x <= 1.0e-12 || radius_y <= 1.0e-12 || start == end)
    {
        path.lineTo(end);
        return;
    }

    const double rotation = rotation_degrees * kPi / 180.0;
    const double cosine = std::cos(rotation);
    const double sine = std::sin(rotation);
    const double half_x = (start.x() - end.x()) * 0.5;
    const double half_y = (start.y() - end.y()) * 0.5;
    const double x_prime = cosine * half_x + sine * half_y;
    const double y_prime = -sine * half_x + cosine * half_y;
    double radii_scale =
        x_prime * x_prime / (radius_x * radius_x) +
        y_prime * y_prime / (radius_y * radius_y);
    if (radii_scale > 1.0)
    {
        radii_scale = std::sqrt(radii_scale);
        radius_x *= radii_scale;
        radius_y *= radii_scale;
    }

    const double numerator =
        std::max(0.0, radius_x * radius_x * radius_y * radius_y -
                          radius_x * radius_x * y_prime * y_prime -
                          radius_y * radius_y * x_prime * x_prime);
    const double denominator =
        radius_x * radius_x * y_prime * y_prime +
        radius_y * radius_y * x_prime * x_prime;
    double coefficient = denominator <= 1.0e-12 ? 0.0 : std::sqrt(numerator / denominator);
    if (large_arc == sweep)
    {
        coefficient = -coefficient;
    }
    const double center_x_prime = coefficient * radius_x * y_prime / radius_y;
    const double center_y_prime = -coefficient * radius_y * x_prime / radius_x;
    const double center_x =
        cosine * center_x_prime - sine * center_y_prime + (start.x() + end.x()) * 0.5;
    const double center_y =
        sine * center_x_prime + cosine * center_y_prime + (start.y() + end.y()) * 0.5;

    const double start_angle =
        vectorAngle(1.0, 0.0, (x_prime - center_x_prime) / radius_x,
                    (y_prime - center_y_prime) / radius_y);
    double sweep_angle =
        vectorAngle((x_prime - center_x_prime) / radius_x,
                    (y_prime - center_y_prime) / radius_y,
                    (-x_prime - center_x_prime) / radius_x,
                    (-y_prime - center_y_prime) / radius_y);
    if (!sweep && sweep_angle > 0.0)
    {
        sweep_angle -= 2.0 * kPi;
    }
    else if (sweep && sweep_angle < 0.0)
    {
        sweep_angle += 2.0 * kPi;
    }

    const int segment_count =
        std::max(1, static_cast<int>(std::ceil(std::abs(sweep_angle) / (kPi * 0.5))));
    const double segment_angle = sweep_angle / segment_count;
    double angle = start_angle;
    for (int segment = 0; segment < segment_count; ++segment)
    {
        const double next_angle = angle + segment_angle;
        const double factor = 4.0 / 3.0 * std::tan((next_angle - angle) * 0.25);
        const auto map_point = [&](double unit_x, double unit_y)
        {
            return QPointF(center_x + cosine * radius_x * unit_x -
                               sine * radius_y * unit_y,
                           center_y + sine * radius_x * unit_x +
                               cosine * radius_y * unit_y);
        };
        const QPointF first = map_point(std::cos(angle), std::sin(angle));
        const QPointF second = map_point(std::cos(next_angle), std::sin(next_angle));
        const QPointF first_control =
            map_point(std::cos(angle) - factor * std::sin(angle),
                      std::sin(angle) + factor * std::cos(angle));
        const QPointF second_control =
            map_point(std::cos(next_angle) + factor * std::sin(next_angle),
                      std::sin(next_angle) - factor * std::cos(next_angle));
        if (segment == 0 && QLineF(path.currentPosition(), first).length() > 1.0e-8)
        {
            path.lineTo(first);
        }
        path.cubicTo(first_control, second_control, second);
        angle = next_angle;
    }
}

QPointF reflected(const QPointF& control, const QPointF& around)
{
    return around * 2.0 - control;
}

bool readPair(SPathReader& reader, double& first, double& second)
{
    return reader.readNumber(first) && reader.readNumber(second);
}

} // namespace

SResult<std::vector<SSvgSubpath>> parseSvgPathData(const QString& path_data)
{
    SPathReader reader(path_data);
    std::vector<SSvgSubpath> result;
    SSvgSubpath* current_subpath = nullptr;
    QPointF current;
    QPointF subpath_start;
    QPointF last_cubic_control;
    QPointF last_quadratic_control;
    QChar command;
    QChar previous_command;

    const auto ensure_subpath = [&]() -> SSvgSubpath&
    {
        if (!current_subpath)
        {
            result.push_back({});
            current_subpath = &result.back();
            current_subpath->path.moveTo(current);
            subpath_start = current;
        }
        return *current_subpath;
    };

    while (!reader.atEnd())
    {
        if (reader.nextIsCommand())
        {
            command = reader.readCommand();
        }
        else if (command.isNull())
        {
            return SResult<std::vector<SSvgSubpath>>::failure(
                QStringLiteral("SVG path 缺少命令。"));
        }
        const bool relative = command.isLower();
        const QChar normalized = command.toUpper();

        if (normalized == QLatin1Char('Z'))
        {
            if (current_subpath)
            {
                current_subpath->path.closeSubpath();
                current_subpath->is_closed = true;
                current = subpath_start;
            }
            current_subpath = nullptr;
            previous_command = command;
            command = {};
            continue;
        }

        double values[7]{};
        int required = 0;
        if (normalized == QLatin1Char('M') || normalized == QLatin1Char('L') ||
            normalized == QLatin1Char('T'))
        {
            required = 2;
        }
        else if (normalized == QLatin1Char('H') || normalized == QLatin1Char('V'))
        {
            required = 1;
        }
        else if (normalized == QLatin1Char('C'))
        {
            required = 6;
        }
        else if (normalized == QLatin1Char('S') || normalized == QLatin1Char('Q'))
        {
            required = 4;
        }
        else if (normalized == QLatin1Char('A'))
        {
            required = 7;
        }
        else
        {
            return SResult<std::vector<SSvgSubpath>>::failure(
                QStringLiteral("SVG path 包含不支持的命令：%1").arg(command));
        }
        for (int index = 0; index < required; ++index)
        {
            if (!reader.readNumber(values[index]))
            {
                return SResult<std::vector<SSvgSubpath>>::failure(
                    QStringLiteral("SVG path 命令 %1 的参数不完整。").arg(command));
            }
        }

        if (normalized == QLatin1Char('M'))
        {
            QPointF point(values[0], values[1]);
            if (relative)
            {
                point += current;
            }
            result.push_back({});
            current_subpath = &result.back();
            current_subpath->path.moveTo(point);
            current = point;
            subpath_start = point;
            command = relative ? QLatin1Char('l') : QLatin1Char('L');
        }
        else
        {
            QPainterPath& path = ensure_subpath().path;
            if (normalized == QLatin1Char('L'))
            {
                QPointF point(values[0], values[1]);
                current = relative ? current + point : point;
                path.lineTo(current);
            }
            else if (normalized == QLatin1Char('H'))
            {
                current.setX(relative ? current.x() + values[0] : values[0]);
                path.lineTo(current);
            }
            else if (normalized == QLatin1Char('V'))
            {
                current.setY(relative ? current.y() + values[0] : values[0]);
                path.lineTo(current);
            }
            else if (normalized == QLatin1Char('C'))
            {
                QPointF first(values[0], values[1]);
                QPointF second(values[2], values[3]);
                QPointF end(values[4], values[5]);
                if (relative)
                {
                    first += current;
                    second += current;
                    end += current;
                }
                path.cubicTo(first, second, end);
                last_cubic_control = second;
                current = end;
            }
            else if (normalized == QLatin1Char('S'))
            {
                QPointF second(values[0], values[1]);
                QPointF end(values[2], values[3]);
                if (relative)
                {
                    second += current;
                    end += current;
                }
                const QChar previous = previous_command.toUpper();
                const QPointF first =
                    previous == QLatin1Char('C') || previous == QLatin1Char('S')
                    ? reflected(last_cubic_control, current)
                    : current;
                path.cubicTo(first, second, end);
                last_cubic_control = second;
                current = end;
            }
            else if (normalized == QLatin1Char('Q'))
            {
                QPointF control(values[0], values[1]);
                QPointF end(values[2], values[3]);
                if (relative)
                {
                    control += current;
                    end += current;
                }
                path.quadTo(control, end);
                last_quadratic_control = control;
                current = end;
            }
            else if (normalized == QLatin1Char('T'))
            {
                QPointF end(values[0], values[1]);
                if (relative)
                {
                    end += current;
                }
                const QChar previous = previous_command.toUpper();
                const QPointF control =
                    previous == QLatin1Char('Q') || previous == QLatin1Char('T')
                    ? reflected(last_quadratic_control, current)
                    : current;
                path.quadTo(control, end);
                last_quadratic_control = control;
                current = end;
            }
            else if (normalized == QLatin1Char('A'))
            {
                QPointF end(values[5], values[6]);
                if (relative)
                {
                    end += current;
                }
                appendSvgArc(path, current, end, values[0], values[1], values[2],
                             std::abs(values[3]) > 0.5, std::abs(values[4]) > 0.5);
                current = end;
            }
        }
        previous_command = normalized == QLatin1Char('M')
                               ? (relative ? QLatin1Char('l') : QLatin1Char('L'))
                               : command;
    }
    result.erase(std::remove_if(result.begin(), result.end(),
                                [](const SSvgSubpath& subpath)
                                {
                                    return subpath.path.elementCount() < 2;
                                }),
                 result.end());
    return SResult<std::vector<SSvgSubpath>>::success(std::move(result));
}

} // namespace vectorPath
