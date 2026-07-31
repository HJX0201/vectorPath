#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_cad_workspace_widget.h"
#include "s_command_catalog.h"
#include "s_document_transaction.h"
#include "s_quick_entity_operation.h"

#include <QtTest>
#include <QToolBar>
#include <cmath>

namespace smartCam
{
namespace
{

SEntityId addLine(SCadDocument& document, const QString& layer_name, const SPoint2d& start,
                  const SPoint2d& end)
{
    SEntityRecord entity;
    entity.type = SEntityType::Line;
    entity.layer_name = layer_name;
    entity.geometry = SLineEntity{start, end};
    std::unique_ptr<SDocumentTransaction> transaction =
        document.beginTransaction(QStringLiteral("测试直线"));
    const SEntityId id = transaction->addEntityCopy(entity);
    transaction->commit();
    return id;
}

const SEntityRecord& entityById(const SCadDocument& document, SEntityId id)
{
    const auto iterator =
        std::find_if(document.entities().begin(), document.entities().end(),
                     [id](const SEntityRecord& entity)
                     {
                         return entity.id == id;
                     });
    Q_ASSERT(iterator != document.entities().end());
    return *iterator;
}

void comparePoint(const SPoint2d& actual, const SPoint2d& expected)
{
    QVERIFY(std::abs(actual.x - expected.x) < 1.0e-9);
    QVERIFY(std::abs(actual.y - expected.y) < 1.0e-9);
}

} // namespace

class SQuickEntityOperationTest final : public QObject
{
    Q_OBJECT

  private slots:
    void selectedRotationUsesSharedCenter();
    void mirrorCopiesAndKeepsOriginals();
    void reverseSelectionCreatesContiguousBlockAndUndoRedo();
    void allEntityScopeIncludesHiddenAndLockedLayers();
    void preferredColorCreatesLayerLazilyAndCopyKeepsLayer();
    void selectionRightClickRequestsContextBarOnlyInSelectMode();
    void quickBarStaysInsideWorkspaceAndCommandsAreDiscoverable();
};

void SQuickEntityOperationTest::selectedRotationUsesSharedCenter()
{
    SCadDocument document;
    const SEntityId first = addLine(document, document.currentLayerName(), {0.0, 0.0}, {2.0, 0.0});
    const SEntityId second = addLine(document, document.currentLayerName(), {8.0, 2.0}, {10.0, 2.0});

    const SQuickEntityOperationResult result = applyQuickEntityOperation(
        document, {first, second}, SQuickEntityOperation::RotateClockwise90, {5.0, 1.0});

    QCOMPARE(result.changed_count, 2);
    const SLineEntity first_line = std::get<SLineEntity>(entityById(document, first).geometry);
    const SLineEntity second_line = std::get<SLineEntity>(entityById(document, second).geometry);
    comparePoint(first_line.start_point, {4.0, 6.0});
    comparePoint(first_line.end_point, {4.0, 4.0});
    comparePoint(second_line.start_point, {6.0, -2.0});
    comparePoint(second_line.end_point, {6.0, -4.0});
    QCOMPARE(document.entities()[0].id, first);
    QCOMPARE(document.entities()[1].id, second);
    document.undo();
    comparePoint(std::get<SLineEntity>(entityById(document, first).geometry).start_point,
                 {0.0, 0.0});
    document.redo();
    comparePoint(std::get<SLineEntity>(entityById(document, first).geometry).start_point,
                 {4.0, 6.0});
}

void SQuickEntityOperationTest::mirrorCopiesAndKeepsOriginals()
{
    SCadDocument document;
    const SEntityId source = addLine(document, document.currentLayerName(), {1.0, 2.0}, {3.0, 4.0});

    const SQuickEntityOperationResult result = applyQuickEntityOperation(
        document, {source}, SQuickEntityOperation::MirrorVertical, {2.0, 3.0});

    QCOMPARE(result.created_entity_ids.size(), std::size_t(1));
    QCOMPARE(document.entities().size(), std::size_t(2));
    comparePoint(std::get<SLineEntity>(entityById(document, source).geometry).start_point,
                 {1.0, 2.0});
    const SLineEntity copy =
        std::get<SLineEntity>(entityById(document, result.created_entity_ids.front()).geometry);
    comparePoint(copy.start_point, {3.0, 2.0});
    comparePoint(copy.end_point, {1.0, 4.0});
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(1));
    document.redo();
    QCOMPARE(document.entities().size(), std::size_t(2));
}

void SQuickEntityOperationTest::reverseSelectionCreatesContiguousBlockAndUndoRedo()
{
    SCadDocument document;
    const QString layer = document.currentLayerName();
    const SEntityId first = addLine(document, layer, {0.0, 0.0}, {1.0, 0.0});
    const SEntityId fixed = addLine(document, layer, {2.0, 0.0}, {3.0, 0.0});
    const SEntityId third = addLine(document, layer, {4.0, 0.0}, {5.0, 0.0});
    const SEntityId last = addLine(document, layer, {6.0, 0.0}, {7.0, 0.0});

    applyQuickEntityOperation(document, {first, third}, SQuickEntityOperation::Reverse, {});

    QCOMPARE(document.entities()[0].id, third);
    QCOMPARE(document.entities()[1].id, first);
    QCOMPARE(document.entities()[2].id, fixed);
    QCOMPARE(document.entities()[3].id, last);
    comparePoint(std::get<SLineEntity>(entityById(document, third).geometry).start_point,
                 {5.0, 0.0});
    document.undo();
    QCOMPARE(document.entities()[0].id, first);
    QCOMPARE(document.entities()[1].id, fixed);
    QCOMPARE(document.entities()[2].id, third);
    comparePoint(std::get<SLineEntity>(entityById(document, third).geometry).start_point,
                 {4.0, 0.0});
    document.redo();
    QCOMPARE(document.entities()[0].id, third);
    comparePoint(std::get<SLineEntity>(entityById(document, third).geometry).start_point,
                 {5.0, 0.0});
}

void SQuickEntityOperationTest::allEntityScopeIncludesHiddenAndLockedLayers()
{
    SCadDocument document;
    QVERIFY(document.addLayer(QStringLiteral("HIDDEN")));
    QVERIFY(document.addLayer(QStringLiteral("LOCKED")));
    const SEntityId hidden = addLine(document, QStringLiteral("HIDDEN"), {0.0, 0.0}, {1.0, 0.0});
    const SEntityId locked = addLine(document, QStringLiteral("LOCKED"), {2.0, 0.0}, {3.0, 0.0});
    QVERIFY(document.setLayerVisible(QStringLiteral("HIDDEN"), false));
    QVERIFY(document.setLayerLocked(QStringLiteral("LOCKED"), true));

    const std::vector<SEntityId> targets = quickOperationTargetIds(document, {});
    QCOMPARE(targets, std::vector<SEntityId>({hidden, locked}));
    applyQuickEntityOperation(document, {}, SQuickEntityOperation::RotateCounterclockwise180,
                              {1.5, 0.0});
    comparePoint(std::get<SLineEntity>(entityById(document, hidden).geometry).start_point,
                 {3.0, 0.0});
    comparePoint(std::get<SLineEntity>(entityById(document, locked).geometry).start_point,
                 {1.0, 0.0});
}

void SQuickEntityOperationTest::preferredColorCreatesLayerLazilyAndCopyKeepsLayer()
{
    SCadDocument document;
    const std::size_t initial_layer_count = document.layers().size();
    const QColor preferred(QStringLiteral("#12AB34"));
    document.setPreferredDrawingColor(preferred);
    QCOMPARE(document.layers().size(), initial_layer_count);

    std::unique_ptr<SDocumentTransaction> transaction =
        document.beginTransaction(QStringLiteral("首选颜色新实体"));
    const SEntityId colored = transaction->addLine({0.0, 0.0}, {1.0, 0.0});
    transaction->commit();
    QCOMPARE(document.layers().size(), initial_layer_count + 1);
    QCOMPARE(document.layerColor(entityById(document, colored).layer_name), preferred);

    SEntityRecord copy_source = entityById(document, colored);
    copy_source.layer_name = document.currentLayerName();
    std::unique_ptr<SDocumentTransaction> copy_transaction =
        document.beginTransaction(QStringLiteral("复制不重染"));
    const SEntityId copy = copy_transaction->addEntityCopy(copy_source);
    copy_transaction->commit();
    QCOMPARE(entityById(document, copy).layer_name, document.currentLayerName());
}

void SQuickEntityOperationTest::selectionRightClickRequestsContextBarOnlyInSelectMode()
{
    SCadDocument document;
    const SEntityId entity_id =
        addLine(document, document.currentLayerName(), {0.0, 0.0}, {1.0, 0.0});
    SCadViewport viewport;
    viewport.resize(320, 240);
    viewport.setDocument(&document);
    viewport.setSelectedEntityIds({entity_id});
    viewport.setToolMode(SToolMode::Select);
    QSignalSpy context_spy(&viewport, &SCadViewport::selectionContextRequested);

    QTest::mouseClick(&viewport, Qt::RightButton, Qt::NoModifier, QPoint(100, 100));
    QCOMPARE(context_spy.count(), 1);

    viewport.setSelectedEntityIds({entity_id});
    viewport.setToolMode(SToolMode::Line);
    QTest::mouseClick(&viewport, Qt::RightButton, Qt::NoModifier, QPoint(100, 100));
    QCOMPARE(context_spy.count(), 1);
}

void SQuickEntityOperationTest::quickBarStaysInsideWorkspaceAndCommandsAreDiscoverable()
{
    SCadWorkspaceWidget workspace;
    workspace.resize(500, 360);
    workspace.show();
    QApplication::processEvents();
    QToolBar* toolbar = workspace.quickOperationBar();
    QCOMPARE(toolbar->parentWidget(), &workspace);
    QVERIFY(toolbar->geometry().right() < workspace.width());
    QVERIFY(toolbar->geometry().bottom() <= workspace.height());

    const QStringList entries = commandCompletionEntries();
    QVERIFY(entries.contains(QStringLiteral("REVERSE")));
    QVERIFY(entries.contains(QStringLiteral("ROTATE CW90")));
    QVERIFY(entries.contains(QStringLiteral("MIRROR HORIZONTAL")));
    QVERIFY(entries.contains(QStringLiteral("DRAWCOLOR #37A6E6")));
    QVERIFY(entries.contains(QStringLiteral("SELCOLOR #37A6E6")));
}

} // namespace smartCam

QTEST_MAIN(smartCam::SQuickEntityOperationTest)
#include "s_quick_entity_operation_test.moc"
