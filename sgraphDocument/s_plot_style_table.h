#pragma once

#include "s_result.h"

#include <QByteArray>
#include <QColor>
#include <QString>
#include <vector>

namespace vectorPath
{

enum class SPlotStyleTableType
{
    ColorDependent,
    Named
};

struct SPlotStyleRecord
{
    int index = 0;
    QString name;
    QColor color;
    bool use_object_color = true;
    bool grayscale = false;
    bool dithering = true;
    int screen_percentage = 100;
    double line_width_mm = -1.0;
};

class SPlotStyleTable final
{
  public:
    static SResult<SPlotStyleTable> load(const QString& file_path);
    static SResult<SPlotStyleTable> fromReference(const QString& reference);

    SPlotStyleTableType type() const noexcept;
    QString description() const;
    const std::vector<SPlotStyleRecord>& styles() const noexcept;
    QColor mappedColor(const QColor& source_color) const;
    double mappedLineWidth(const QColor& source_color, double object_width_mm) const noexcept;

  private:
    friend SResult<SPlotStyleTable> parsePlotStyleData(const QByteArray& data,
                                                       const QString& file_path);
    const SPlotStyleRecord* styleForColor(const QColor& source_color) const noexcept;
    static SPlotStyleTable monochromeTable();
    static SPlotStyleTable grayscaleTable();

    SPlotStyleTableType m_type = SPlotStyleTableType::ColorDependent;
    QString m_description;
    std::vector<SPlotStyleRecord> m_styles;
    bool m_force_monochrome = false;
    bool m_force_grayscale = false;
};

} // namespace vectorPath
