#pragma once

#include "vp_result.h"

#include <QByteArray>
#include <QColor>
#include <QString>
#include <vector>

namespace Vp
{

enum class VpPlotStyleTableType
{
    ColorDependent,
    Named
};

struct VpPlotStyleRecord
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

class VpPlotStyleTable final
{
  public:
    static VpResult<VpPlotStyleTable> load(const QString& file_path);
    static VpResult<VpPlotStyleTable> fromReference(const QString& reference);

    VpPlotStyleTableType type() const noexcept;
    QString description() const;
    const std::vector<VpPlotStyleRecord>& styles() const noexcept;
    QColor mappedColor(const QColor& source_color) const;
    double mappedLineWidth(const QColor& source_color, double object_width_mm) const noexcept;

  private:
    friend VpResult<VpPlotStyleTable> parsePlotStyleData(const QByteArray& data,
                                                         const QString& file_path);
    const VpPlotStyleRecord* styleForColor(const QColor& source_color) const noexcept;
    static VpPlotStyleTable monochromeTable();
    static VpPlotStyleTable grayscaleTable();

    VpPlotStyleTableType m_type = VpPlotStyleTableType::ColorDependent;
    QString m_description;
    std::vector<VpPlotStyleRecord> m_styles;
    bool m_force_monochrome = false;
    bool m_force_grayscale = false;
};

} // namespace Vp
