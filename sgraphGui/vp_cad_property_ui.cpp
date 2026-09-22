#include "vp_cad_property_ui.h"

#include <QComboBox>
#include <QObject>

namespace Vp
{

const QVector<double>& standardLineWidths()
{
    static const QVector<double> kLineWidths{
        0.05, 0.09, 0.13, 0.15, 0.18, 0.20, 0.25, 0.30, 0.35, 0.40, 0.50, 0.53,
        0.60, 0.70, 0.80, 0.90, 1.00, 1.06, 1.20, 1.40, 1.58, 2.00, 2.11,
    };
    return kLineWidths;
}

QString lineWidthText(double line_width_mm, bool allow_by_layer)
{
    if (allow_by_layer && line_width_mm < 0.0)
    {
        return QObject::tr("ByLayer");
    }
    return QObject::tr("%1 mm").arg(line_width_mm, 0, 'f', 2);
}

void populateLineWidthCombo(QComboBox* combo_box, bool include_by_layer)
{
    combo_box->clear();
    if (include_by_layer)
    {
        combo_box->addItem(QObject::tr("ByLayer"), -1.0);
    }
    for (double line_width_mm : standardLineWidths())
    {
        combo_box->addItem(lineWidthText(line_width_mm, false), line_width_mm);
    }
}

QString propertyPanelStyle(const VpDesignToken& tokens)
{
    return QStringLiteral(
               "QWidget#smartPropertiesPanel { background: %1; }"
               "QFrame#smartPropertySummary { background: transparent; border: 0;"
               " border-bottom: 1px solid %3; border-radius: 0; }"
               "QLabel#smartPropertyCaption { color: %4; font-size: 11px; }"
               "QLabel#smartPropertySummaryText { color: %5; font-size: 12px;"
               " font-weight: 600; }"
               "QComboBox#smartPropertyCombo { min-height: 23px; padding: 0 6px;"
               " background: %2; border: 1px solid %3; border-radius: 2px; }"
               "QComboBox#smartPropertyCombo:hover { border-color: %6; }"
               "QTreeWidget#smartPropertyTree { background: %1; border: 0; outline: none; }"
               "QTreeWidget#smartPropertyTree::item { min-height: 22px; padding: 0 4px; }"
               "QTreeWidget#smartPropertyTree::item:hover { background: %2; }"
               "QTreeWidget#smartPropertyTree::item:selected { background: %6; color: white; }"
               "QHeaderView::section { background: %2; color: %4; border: none;"
               " border-bottom: 1px solid %3; padding: 4px 6px; font-weight: 600; }")
        .arg(tokens.surface.name(), tokens.elevated_surface.name(), tokens.border.name())
        .arg(tokens.text_secondary.name(), tokens.text_primary.name(), tokens.accent.name());
}

QString propertyColorButtonStyle(const QColor& color, const VpDesignToken& tokens)
{
    const QColor background = color.isValid() ? color : tokens.elevated_surface;
    const QColor foreground =
        color.isValid() ? (background.lightness() < 128 ? QColor(Qt::white) : QColor(Qt::black))
                        : tokens.text_secondary;
    return QStringLiteral(
               "QPushButton#smartPropertyColorButton { min-height: 23px; padding: 0 6px;"
               " text-align: left; background: %1; color: %2; border: 1px solid %3;"
               " border-radius: 2px; font-weight: 600; }"
               "QPushButton#smartPropertyColorButton:hover { border-color: %4; }"
               "QPushButton#smartPropertyColorButton:disabled { background: %5; color: %6; }")
        .arg(background.name(), foreground.name(), tokens.border.name())
        .arg(tokens.accent.name(), tokens.elevated_surface.name(), tokens.text_secondary.name());
}

} // namespace Vp
