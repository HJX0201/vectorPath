#pragma once

#include "vp_design_token.h"

#include <QString>
#include <QVector>

class QComboBox;

namespace Vp
{

// 返回属性面板使用的标准线宽列表。
const QVector<double>& standardLineWidths();
// 把线宽格式化为显示文本。
QString lineWidthText(double line_width_mm, bool allow_by_layer = true);
// 填充线宽下拉框的候选项。
void populateLineWidthCombo(QComboBox* combo_box, bool include_by_layer);
// 返回属性面板的整体样式表。
QString propertyPanelStyle(const VpDesignToken& tokens);
// 返回属性面板颜色按钮的样式表。
QString propertyColorButtonStyle(const QColor& color, const VpDesignToken& tokens);

} // namespace Vp
