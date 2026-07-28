#include "s_vector_document_import.h"

#include "s_cad_document.h"
#include "s_document_transaction.h"

#include <QHash>
#include <QtGlobal>

namespace smartGraphics
{
namespace
{

QString layerName(const QColor& color)
{
    return QStringLiteral("SVG_%1%2%3_A%4")
        .arg(color.red(), 2, 16, QLatin1Char('0'))
        .arg(color.green(), 2, 16, QLatin1Char('0'))
        .arg(color.blue(), 2, 16, QLatin1Char('0'))
        .arg(color.alpha(), 3, 10, QLatin1Char('0'))
        .toUpper();
}

int layerTransparency(const QColor& color)
{
    return qBound(0, qRound((1.0 - color.alphaF()) * 90.0), 90);
}

} // namespace

SResult<SVectorDocumentImportReport> importVectorGeometry(
    SCadDocument& document, const SVectorImportGeometry& geometry)
{
    if (geometry.entities.empty())
    {
        return SResult<SVectorDocumentImportReport>::failure(
            QStringLiteral("没有可导入的矢量实体。"));
    }
    const std::size_t initial_layer_count = document.layers().size();
    auto transaction = document.beginTransaction(QObject::tr("导入 SVG 矢量"));
    QHash<QRgb, QString> layers;
    for (auto iterator = geometry.entities.rbegin(); iterator != geometry.entities.rend();
         ++iterator)
    {
        if (iterator->type != SEntityType::Hatch)
        {
            continue;
        }
        QColor opaque_color = iterator->color;
        const QRgb color_key = iterator->color.rgba();
        if (layers.contains(color_key))
        {
            continue;
        }
        opaque_color.setAlpha(255);
        const QString target_layer =
            transaction->ensureLayer(layerName(iterator->color), opaque_color,
                                     layerTransparency(iterator->color));
        if (target_layer.isEmpty())
        {
            return SResult<SVectorDocumentImportReport>::failure(
                QStringLiteral("无法创建 SVG 颜色图层。"));
        }
        layers.insert(color_key, target_layer);
    }
    for (const SColoredEntityGeometry& imported : geometry.entities)
    {
        QColor opaque_color = imported.color;
        const QRgb color_key = imported.color.rgba();
        opaque_color.setAlpha(255);
        QString target_layer = layers.value(color_key);
        if (target_layer.isEmpty())
        {
            target_layer =
                transaction->ensureLayer(layerName(imported.color), opaque_color,
                                         layerTransparency(imported.color));
            if (target_layer.isEmpty())
            {
                return SResult<SVectorDocumentImportReport>::failure(
                    QStringLiteral("无法创建 SVG 颜色图层。"));
            }
            layers.insert(color_key, target_layer);
        }
        transaction->addEntity(imported.type, imported.geometry, target_layer);
    }
    transaction->commit();
    SVectorDocumentImportReport report;
    report.imported_entity_count = geometry.entities.size();
    report.created_layer_count = document.layers().size() - initial_layer_count;
    return SResult<SVectorDocumentImportReport>::success(report);
}

} // namespace smartGraphics
