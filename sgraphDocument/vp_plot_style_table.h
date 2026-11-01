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

// 打印样式表中单条索引对应的打印属性。
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

// 打印样式表：把对象颜色映射为打印颜色与线宽。
class VpPlotStyleTable final
{
  public:
    // 从打印样式表文件加载。
    static VpResult<VpPlotStyleTable> load(const QString& file_path);
    // 按引用名加载，缺失时回退到内置样式表。
    static VpResult<VpPlotStyleTable> fromReference(const QString& reference);

    // 返回样式表类型。
    VpPlotStyleTableType type() const noexcept;
    // 返回样式表说明。
    QString description() const;
    // 返回全部打印样式记录。
    const std::vector<VpPlotStyleRecord>& styles() const noexcept;
    // 把对象颜色映射为打印颜色。
    QColor mappedColor(const QColor& source_color) const;
    // 把对象线宽映射为打印线宽。
    double mappedLineWidth(const QColor& source_color, double object_width_mm) const noexcept;

  private:
    // 解析样式表数据的友元入口。
    friend VpResult<VpPlotStyleTable> parsePlotStyleData(const QByteArray& data,
                                                         const QString& file_path);
    // 按颜色查找匹配的样式记录。
    const VpPlotStyleRecord* styleForColor(const QColor& source_color) const noexcept;
    // 返回内置的单色样式表。
    static VpPlotStyleTable monochromeTable();
    // 返回内置的灰度样式表。
    static VpPlotStyleTable grayscaleTable();

    VpPlotStyleTableType m_type = VpPlotStyleTableType::ColorDependent;
    QString m_description;
    std::vector<VpPlotStyleRecord> m_styles;
    bool m_force_monochrome = false;
    bool m_force_grayscale = false;
};

} // namespace Vp
