#include "s_cad_document.h"
#include "s_document_transaction.h"

#include <QtTest>
#include <algorithm>

namespace smartCam
{

class SLayerColorWorkflowTest final : public QObject
{
    Q_OBJECT

  private slots:
    void isolatesEntityColorOnNewLayer();
    void removesEmptySourceLayerAndSupportsUndo();
    void mergesSameColorLayersAtomically();
};

void SLayerColorWorkflowTest::isolatesEntityColorOnNewLayer()
{
    SCadDocument document;
    QVERIFY(document.addLayer(QStringLiteral("SOURCE")));
    const QColor original_color(30, 80, 140);
    const QColor new_color(220, 40, 60);
    QVERIFY(document.setLayerColor(QStringLiteral("SOURCE"), original_color));
    QVERIFY(document.setCurrentLayer(QStringLiteral("SOURCE")));
    auto transaction = document.beginTransaction(QStringLiteral("create"));
    const SEntityId first_id = transaction->addLine({0.0, 0.0}, {10.0, 0.0});
    const SEntityId second_id = transaction->addLine({0.0, 5.0}, {10.0, 5.0});
    transaction->commit();

    const QString new_layer =
        document.assignEntitiesToColorLayer({first_id}, new_color);
    QVERIFY(!new_layer.isEmpty());
    QVERIFY(document.layer(QStringLiteral("SOURCE")));
    QCOMPARE(document.layerColor(QStringLiteral("SOURCE")), original_color);
    QCOMPARE(document.layerColor(new_layer), new_color);
    const auto first_iterator =
        std::find_if(document.entities().begin(), document.entities().end(),
                     [first_id](const SEntityRecord& entity)
                     {
                         return entity.id == first_id;
                     });
    const auto second_iterator =
        std::find_if(document.entities().begin(), document.entities().end(),
                     [second_id](const SEntityRecord& entity)
                     {
                         return entity.id == second_id;
                     });
    QVERIFY(first_iterator != document.entities().end());
    QVERIFY(second_iterator != document.entities().end());
    QCOMPARE(first_iterator->layer_name, new_layer);
    QCOMPARE(second_iterator->layer_name, QStringLiteral("SOURCE"));
}

void SLayerColorWorkflowTest::removesEmptySourceLayerAndSupportsUndo()
{
    SCadDocument document;
    QVERIFY(document.addLayer(QStringLiteral("SOURCE")));
    QVERIFY(document.setCurrentLayer(QStringLiteral("SOURCE")));
    auto transaction = document.beginTransaction(QStringLiteral("create"));
    const SEntityId entity_id = transaction->addCircle({5.0, 5.0}, 2.0);
    transaction->commit();

    const QString new_layer =
        document.assignEntitiesToColorLayer({entity_id}, QColor(20, 180, 90));
    QVERIFY(!new_layer.isEmpty());
    QVERIFY(!document.layer(QStringLiteral("SOURCE")));
    QCOMPARE(document.currentLayerName(), new_layer);

    document.undo();
    QVERIFY(document.layer(QStringLiteral("SOURCE")));
    QCOMPARE(document.currentLayerName(), QStringLiteral("SOURCE"));
    QCOMPARE(document.entities().front().layer_name, QStringLiteral("SOURCE"));
}

void SLayerColorWorkflowTest::mergesSameColorLayersAtomically()
{
    SCadDocument document;
    const QColor shared_color(75, 135, 215);
    QVERIFY(document.addLayer(QStringLiteral("FIRST")));
    QVERIFY(document.addLayer(QStringLiteral("SECOND")));
    QVERIFY(document.setLayerColor(QStringLiteral("FIRST"), shared_color));
    QVERIFY(document.setLayerColor(QStringLiteral("SECOND"), shared_color));
    QVERIFY(document.setCurrentLayer(QStringLiteral("FIRST")));
    auto first_transaction = document.beginTransaction(QStringLiteral("first"));
    first_transaction->addLine({0.0, 0.0}, {10.0, 0.0});
    first_transaction->commit();
    QVERIFY(document.setCurrentLayer(QStringLiteral("SECOND")));
    auto second_transaction = document.beginTransaction(QStringLiteral("second"));
    second_transaction->addLine({0.0, 5.0}, {10.0, 5.0});
    second_transaction->commit();

    QCOMPARE(document.mergeLayersByColor(), 1);
    QVERIFY(document.layer(QStringLiteral("FIRST")));
    QVERIFY(!document.layer(QStringLiteral("SECOND")));
    QCOMPARE(document.entities().at(0).layer_name, QStringLiteral("FIRST"));
    QCOMPARE(document.entities().at(1).layer_name, QStringLiteral("FIRST"));
    QCOMPARE(document.currentLayerName(), QStringLiteral("FIRST"));

    document.undo();
    QVERIFY(document.layer(QStringLiteral("FIRST")));
    QVERIFY(document.layer(QStringLiteral("SECOND")));
    QCOMPARE(document.entities().at(1).layer_name, QStringLiteral("SECOND"));
}

} // namespace smartCam

QTEST_APPLESS_MAIN(smartCam::SLayerColorWorkflowTest)
#include "s_layer_color_workflow_test.moc"
