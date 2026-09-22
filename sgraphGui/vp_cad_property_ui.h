#pragma once

#include "vp_design_token.h"

#include <QString>
#include <QVector>

class QComboBox;

namespace Vp
{

const QVector<double>& standardLineWidths();
QString lineWidthText(double line_width_mm, bool allow_by_layer = true);
void populateLineWidthCombo(QComboBox* combo_box, bool include_by_layer);
QString propertyPanelStyle(const VpDesignToken& tokens);
QString propertyColorButtonStyle(const QColor& color, const VpDesignToken& tokens);

} // namespace Vp
