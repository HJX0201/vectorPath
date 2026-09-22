#include "vp_dxf_table_io.h"

#include "vp_cad_document.h"

#include <QColor>
#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace Vp
{
namespace
{

QColor aciColor(int aci)
{
    const std::array<QColor, 8> colors = {
        QColor(0, 0, 0),     QColor(255, 0, 0), QColor(255, 255, 0), QColor(0, 255, 0),
        QColor(0, 255, 255), QColor(0, 0, 255), QColor(255, 0, 255), QColor(255, 255, 255)};
    const int normalized = std::abs(aci);
    return normalized >= 1 && normalized <= 7 ? colors[static_cast<std::size_t>(normalized)]
                                              : QColor(220, 228, 238);
}

QColor layerColor(const std::vector<VpDxfPair>& record)
{
    const std::optional<double> true_color = findDxfDouble(record, 420);
    if (true_color)
    {
        const int rgb = static_cast<int>(*true_color);
        return QColor((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff);
    }
    return aciColor(static_cast<int>(findDxfDouble(record, 62).value_or(7.0)));
}

int layerTransparency(const std::vector<VpDxfPair>& record)
{
    const std::optional<double> encoded = findDxfDouble(record, 440);
    if (!encoded)
    {
        return 0;
    }
    const int transparent_byte = static_cast<int>(*encoded) & 0xff;
    return std::clamp(static_cast<int>(std::round(transparent_byte * 100.0 / 255.0)), 0, 90);
}

VpLayerRecord readLayerRecord(const std::vector<VpDxfPair>& record)
{
    VpLayerRecord layer;
    layer.name = findDxfString(record, 2).value_or(QString());
    const int flags = static_cast<int>(findDxfDouble(record, 70).value_or(0.0));
    const int aci = static_cast<int>(findDxfDouble(record, 62).value_or(7.0));
    layer.color = layerColor(record);
    layer.is_visible = aci >= 0;
    layer.is_frozen = (flags & 1) != 0;
    layer.is_locked = (flags & 4) != 0;
    layer.is_plottable = static_cast<int>(findDxfDouble(record, 290).value_or(1.0)) != 0;
    layer.line_type = findDxfString(record, 6).value_or(QStringLiteral("Continuous"));
    const double dxf_width = findDxfDouble(record, 370).value_or(25.0);
    layer.line_width_mm = dxf_width < 0.0 ? 0.25 : std::clamp(dxf_width / 100.0, 0.0, 2.11);
    layer.transparency = layerTransparency(record);
    return layer;
}

} // namespace

std::vector<VpLayerRecord> readDxfLayerTable(const std::vector<VpDxfPair>& pairs)
{
    std::vector<VpLayerRecord> layers;
    bool is_tables_section = false;
    bool is_layer_table = false;
    for (std::size_t index = 0; index < pairs.size();)
    {
        if (pairs[index].group_code == 0 && pairs[index].value == QLatin1String("SECTION") &&
            index + 1 < pairs.size() && pairs[index + 1].group_code == 2)
        {
            is_tables_section = pairs[index + 1].value == QLatin1String("TABLES");
            is_layer_table = false;
            index += 2;
            continue;
        }
        if (pairs[index].group_code == 0 && pairs[index].value == QLatin1String("ENDSEC"))
        {
            is_tables_section = false;
            is_layer_table = false;
            ++index;
            continue;
        }
        if (is_tables_section && pairs[index].group_code == 0 &&
            pairs[index].value == QLatin1String("TABLE") && index + 1 < pairs.size() &&
            pairs[index + 1].group_code == 2)
        {
            is_layer_table = pairs[index + 1].value == QLatin1String("LAYER");
            index += 2;
            continue;
        }
        if (is_layer_table && pairs[index].group_code == 0 &&
            pairs[index].value == QLatin1String("ENDTAB"))
        {
            is_layer_table = false;
            ++index;
            continue;
        }
        if (!is_layer_table || pairs[index].group_code != 0 ||
            pairs[index].value != QLatin1String("LAYER"))
        {
            ++index;
            continue;
        }
        ++index;
        std::vector<VpDxfPair> record;
        while (index < pairs.size() && pairs[index].group_code != 0)
        {
            record.push_back(pairs[index++]);
        }
        VpLayerRecord layer = readLayerRecord(record);
        if (!layer.name.isEmpty())
        {
            layers.push_back(std::move(layer));
        }
    }
    return layers;
}

QString readDxfCurrentLayer(const std::vector<VpDxfPair>& pairs)
{
    for (std::size_t index = 0; index + 1 < pairs.size(); ++index)
    {
        if (pairs[index].group_code == 9 && pairs[index].value == QLatin1String("$CLAYER") &&
            pairs[index + 1].group_code == 8)
        {
            return pairs[index + 1].value;
        }
    }
    return QStringLiteral("0");
}

void applyDxfLayerTable(VpCadDocument& document, const std::vector<VpLayerRecord>& layers,
                        const QString& current_layer_name)
{
    for (const VpLayerRecord& layer : layers)
    {
        if (!document.layer(layer.name))
        {
            document.addLayer(layer.name);
        }
        document.setLayerColor(layer.name, layer.color);
        document.setLayerLineWidth(layer.name, layer.line_width_mm);
        document.setLayerLineType(layer.name, layer.line_type);
        document.setLayerTransparency(layer.name, layer.transparency);
        document.setLayerPlottable(layer.name, layer.is_plottable);
        document.setLayerVisible(layer.name, layer.is_visible);
        document.setLayerFrozen(layer.name, layer.is_frozen);
        document.setLayerLocked(layer.name, layer.is_locked);
    }
    if (document.layer(current_layer_name))
    {
        document.setCurrentLayer(current_layer_name);
    }
}

} // namespace Vp
