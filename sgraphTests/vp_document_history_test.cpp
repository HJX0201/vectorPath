#include "vp_cad_document.h"
#include "vp_document_transaction.h"
#include "vp_test_runner.h"

#include <QtTest>

namespace Vp
{
namespace
{

std::vector<VpEntityId> entityIds(const VpCadDocument& document)
{
    std::vector<VpEntityId> ids;
    for (const VpEntityRecord& entity : document.entities())
    {
        ids.push_back(entity.id);
    }
    return ids;
}

std::vector<VpEntityId> seedDocument(VpCadDocument& document)
{
    auto transaction = document.beginTransaction(QStringLiteral("seed"));
    std::vector<VpEntityId> ids;
    for (int index = 0; index < 5; ++index)
    {
        ids.push_back(transaction->addPolyline(
            {{static_cast<double>(index), 0.0}, {static_cast<double>(index), 10.0}}, false));
    }
    transaction->commit();
    return ids;
}

} // namespace

class VpDocumentHistoryTest final : public QObject
{
    Q_OBJECT

  private slots:
    void bulkEraseUndoRedoKeepsEstablishedOrder();
    void repeatedRemovalKeepsEstablishedHistory();
    void layerEraseUsesTheSameStableRemoval();
    void explicitReorderWithReplacementUndoRedo();
    void pendingReorderAlreadyAppliedDoesNotChangeBehavior();
    void commitAndHistoryNotificationOrder();
    void emptyAndCancelledTransactionsDoNotNotify();
};

void VpDocumentHistoryTest::bulkEraseUndoRedoKeepsEstablishedOrder()
{
    VpCadDocument document;
    const std::vector<VpEntityId> ids = seedDocument(document);
    auto transaction = document.beginTransaction(QStringLiteral("erase batch"));
    QVERIFY(transaction->removeEntity(ids[3]));
    QVERIFY(transaction->removeEntity(ids[1]));
    QVERIFY(!transaction->removeEntity(99999));
    transaction->commit();
    const std::vector<VpEntityId> survivors{ids[0], ids[2], ids[4]};
    QVERIFY(entityIds(document) == survivors);
    document.undo();
    // Ordinary undo has always appended removed records in transaction order.
    const std::vector<VpEntityId> restored{ids[0], ids[2], ids[4], ids[3], ids[1]};
    QVERIFY(entityIds(document) == restored);
    const auto& restored_geometry = std::get<VpPolylineEntity>(document.entities().back().geometry);
    QCOMPARE(restored_geometry.vertices.front().x, 1.0);
    document.redo();
    QVERIFY(entityIds(document) == survivors);
    document.undo();
    QVERIFY(entityIds(document) == restored);
}

void VpDocumentHistoryTest::repeatedRemovalKeepsEstablishedHistory()
{
    VpCadDocument document;
    const std::vector<VpEntityId> ids = seedDocument(document);
    auto transaction = document.beginTransaction(QStringLiteral("repeated erase"));
    QVERIFY(transaction->removeEntity(ids[2]));
    QVERIFY(transaction->removeEntity(ids[2]));
    transaction->commit();
    const std::vector<VpEntityId> survivors{ids[0], ids[1], ids[3], ids[4]};
    QVERIFY(entityIds(document) == survivors);
    document.undo();
    // Preserve existing duplicate staging behavior while optimizing the deletion pass.
    const std::vector<VpEntityId> restored{ids[0], ids[1], ids[3], ids[4], ids[2], ids[2]};
    QVERIFY(entityIds(document) == restored);
    document.redo();
    QVERIFY(entityIds(document) == survivors);
}

void VpDocumentHistoryTest::layerEraseUsesTheSameStableRemoval()
{
    VpCadDocument document;
    QVERIFY(document.addLayer(QStringLiteral("detail")));
    auto transaction = document.beginTransaction(QStringLiteral("layer entities"));
    const VpEntityId first_id = transaction->addLine({0.0, 0.0}, {1.0, 1.0});
    QVERIFY(document.setCurrentLayer(QStringLiteral("detail")));
    const VpEntityId removed_id = transaction->addLine({2.0, 2.0}, {3.0, 3.0});
    QVERIFY(document.setCurrentLayer(QStringLiteral("0")));
    const VpEntityId last_id = transaction->addLine({4.0, 4.0}, {5.0, 5.0});
    transaction->commit();
    QVERIFY(document.removeLayerWithContents(QStringLiteral("detail")));
    QVERIFY(entityIds(document) == std::vector<VpEntityId>({first_id, last_id}));
    document.undo();
    QVERIFY(document.layer(QStringLiteral("detail")) != nullptr);
    QVERIFY(entityIds(document) == std::vector<VpEntityId>({first_id, last_id, removed_id}));
    document.redo();
    QVERIFY(entityIds(document) == std::vector<VpEntityId>({first_id, last_id}));
    QVERIFY(document.layer(QStringLiteral("detail")) == nullptr);
}

void VpDocumentHistoryTest::explicitReorderWithReplacementUndoRedo()
{
    VpCadDocument document;
    const std::vector<VpEntityId> ids = seedDocument(document);
    VpEntityRecord replacement = document.entities()[1];
    std::get<VpPolylineEntity>(replacement.geometry).vertices.front().x = 42.0;
    auto transaction = document.beginTransaction(QStringLiteral("replace and reorder"));
    QVERIFY(transaction->replaceEntity(ids[1], replacement));
    const std::vector<VpEntityId> reordered{ids[4], ids[2], ids[0], ids[1], ids[3]};
    QVERIFY(transaction->setEntityOrder(reordered));
    transaction->commit();
    QVERIFY(entityIds(document) == reordered);
    QCOMPARE(std::get<VpPolylineEntity>(document.entities()[3].geometry).vertices.front().x, 42.0);
    document.undo();
    QVERIFY(entityIds(document) == ids);
    QCOMPARE(std::get<VpPolylineEntity>(document.entities()[1].geometry).vertices.front().x, 1.0);
    document.redo();
    QVERIFY(entityIds(document) == reordered);
    QCOMPARE(std::get<VpPolylineEntity>(document.entities()[3].geometry).vertices.front().x, 42.0);
    auto unchanged = document.beginTransaction(QStringLiteral("unchanged order"));
    QVERIFY(!unchanged->setEntityOrder(reordered));
    QVERIFY(!unchanged->setEntityOrder({ids[0]}));
    QVERIFY(!unchanged->setEntityOrder({ids[0], ids[0], ids[0], ids[0], ids[0]}));
}

void VpDocumentHistoryTest::pendingReorderAlreadyAppliedDoesNotChangeBehavior()
{
    VpCadDocument document;
    const std::vector<VpEntityId> ids = seedDocument(document);
    const std::vector<VpEntityId> reordered{ids[4], ids[3], ids[2], ids[1], ids[0]};
    auto pending = document.beginTransaction(QStringLiteral("pending order"));
    QVERIFY(pending->setEntityOrder(reordered));
    auto immediate = document.beginTransaction(QStringLiteral("same order"));
    QVERIFY(immediate->setEntityOrder(reordered));
    immediate->commit();
    pending->commit();
    QVERIFY(entityIds(document) == reordered);
    document.undo();
    QVERIFY(entityIds(document) == reordered);
    document.undo();
    QVERIFY(entityIds(document) == ids);
    document.redo();
    document.redo();
    QVERIFY(entityIds(document) == reordered);
}

void VpDocumentHistoryTest::commitAndHistoryNotificationOrder()
{
    VpCadDocument document;
    QStringList notifications;
    connect(&document, &VpCadDocument::modifiedChanged, this,
            [&notifications](bool modified)
            {
                notifications.append(modified ? QStringLiteral("modified:1")
                                              : QStringLiteral("modified:0"));
            });
    connect(&document, &VpCadDocument::documentChanged, this,
            [&notifications]()
            {
                notifications.append(QStringLiteral("document"));
            });
    connect(&document, &VpCadDocument::historyChanged, this,
            [&notifications](bool can_undo, bool can_redo)
            {
                notifications.append(QStringLiteral("history:%1%2").arg(can_undo).arg(can_redo));
            });
    auto transaction = document.beginTransaction(QStringLiteral("line"));
    transaction->addLine({0.0, 0.0}, {1.0, 1.0});
    transaction->commit();
    QCOMPARE(notifications, QStringList({QStringLiteral("modified:1"), QStringLiteral("document"),
                                         QStringLiteral("history:10")}));
    notifications.clear();
    transaction->commit();
    QVERIFY(notifications.empty());
    document.undo();
    QCOMPARE(notifications,
             QStringList({QStringLiteral("document"), QStringLiteral("history:01")}));
    notifications.clear();
    document.redo();
    QCOMPARE(notifications,
             QStringList({QStringLiteral("document"), QStringLiteral("history:10")}));
}

void VpDocumentHistoryTest::emptyAndCancelledTransactionsDoNotNotify()
{
    VpCadDocument document;
    QSignalSpy changed(&document, &VpCadDocument::documentChanged);
    QSignalSpy history(&document, &VpCadDocument::historyChanged);
    auto empty = document.beginTransaction(QStringLiteral("empty"));
    empty->commit();
    auto cancelled = document.beginTransaction(QStringLiteral("cancel"));
    cancelled->addPolyline({{0.0, 0.0}, {1.0, 2.0}}, false);
    cancelled.reset();
    QVERIFY(document.entities().empty());
    QVERIFY(!document.canUndo());
    QVERIFY(!document.isModified());
    QCOMPARE(changed.count(), 0);
    QCOMPARE(history.count(), 0);
}

} // namespace Vp

VECTORPATH_TEST_ENTRY(Vp::VpDocumentHistoryTest, vpRunVpDocumentHistoryTest)
#include "vp_document_history_test.moc"
