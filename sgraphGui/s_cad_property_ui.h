#pragma once

#include "s_design_token.h"

#include <QString>
#include <QVector>

class QComboBox;

namespace vectorPath
{

const QVector<double>& standardLineWidths();
QString lineWidthText(double line_width_mm, bool allow_by_layer = true);
void populateLineWidthCombo(QComboBox* combo_box, bool include_by_layer);
QString propertyPanelStyle(const SDesignToken& tokens);
QString propertyColorButtonStyle(const QColor& color, const SDesignToken& tokens);

} // namespace vectorPath
