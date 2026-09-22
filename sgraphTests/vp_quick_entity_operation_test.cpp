#include "vp_cad_document.h"
#include "vp_cad_viewport.h"
#include "vp_cad_workspace_widget.h"
#include "vp_command_catalog.h"
#include "vp_document_transaction.h"
#include "vp_quick_entity_operation.h"
#include "vp_test_runner.h"

#include <QToolBar>
#include <QtTest>
#include <cmath>

namespace Vp
{
namespace
{

VpEntityId addLine(VpCadDocument& document, const QString& layer_name, const VpPoint2d& start,
                   const VpPoint2d& end)
{
    VpEntityRecord entity;
    entity.type = VpEntityType::Line;
    entity.layer_name = layer_name;
    entity.geometry = VpLineEntity{start, end};
    std::unique_ptr<VpDocumentTransaction> transaction =
        document.beginTransaction(QStringLiteral("测试直线"));
    const VpEntityId id = transaction->addEntityCopy(entity);
    transaction->commit();
    return id;
}

const VpEntityRecord& entityById(const VpCadDocument& document, VpEntityId id)
{
    const auto iterator = std::find_if(document.entities().begin(), document.entities().end(),
                                       [id](const VpEntityRecord& entity)
                                       {
                                           return entity.id == id;
                                       });
    Q_ASSERT(iterator != document.entities().end());
    return *iterator;
}

void comparePoint(const VpPoint2d& actual, const VpPoint2d& expected)
{
    QVERIFY(std::abs(actual.x - expected.x) < 1.0e-9);
    QVERIFY(std::abs(actual.y - expected.y) < 1.0e-9);
}

} // namespace

class VpQuickEntityOperationTest final : public QObject
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

void VpQuickEntityOperationTest::selectedRotationUsesSharedCenter()
{
    VpCadDocument document;
    const VpEntityId first = addLine(document, document.currentLayerName(), {0.0, 0.0}, {2.0, 0.0});
    const VpEntityId second =
        addLine(document, document.currentLayerName(), {8.0, 2.0}, {10.0, 2.0});

    const VpQuickEntityOperationResult result = applyQuickEntityOperation(
        document, {first, second}, VpQuickEntityOperation::RotateClockwise90, {5.0, 1.0});

    QCOMPARE(result.changed_count, 2);
    const VpLineEntity first_line = std::get<VpLineEntity>(entityById(document, first).geometry);
    const VpLineEntity second_line = std::get<VpLineEntity>(entityById(document, second).geometry);
    comparePoint(first_line.start_point, {4.0, 6.0});
    comparePoint(first_line.end_point, {4.0, 4.0});
    comparePoint(second_line.start_point, {6.0, -2.0});
    comparePoint(second_line.end_point, {6.0, -4.0});
    QCOMPARE(document.entities()[0].id, first);
    QCOMPARE(document.entities()[1].id, second);
    document.undo();
    comparePoint(std::get<VpLineEntity>(entityById(document, first).geometry).start_point,
                 {0.0, 0.0});
    document.redo();
    comparePoint(std::get<VpLineEntity>(entityById(document, first).geometry).start_point,
                 {4.0, 6.0});
}

void VpQuickEntityOperationTest::mirrorCopiesAndKeepsOriginals()
{
    VpCadDocument document;
    const VpEntityId source =
        addLine(document, document.currentLayerName(), {1.0, 2.0}, {3.0, 4.0});

    const VpQuickEntityOperationResult result = applyQuickEntityOperation(
        document, {source}, VpQuickEntityOperation::MirrorVertical, {2.0, 3.0});

    QCOMPARE(result.created_entity_ids.size(), std::size_t(1));
    QCOMPARE(document.entities().size(), std::size_t(2));
    comparePoint(std::get<VpLineEntity>(entityById(document, source).geometry).start_point,
                 {1.0, 2.0});
    const VpLineEntity copy =
        std::get<VpLineEntity>(entityById(document, result.created_entity_ids.front()).geometry);
    comparePoint(copy.start_point, {3.0, 2.0});
    comparePoint(copy.end_point, {1.0, 4.0});
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(1));
    document.redo();
    QCOMPARE(document.entities().size(), std::size_t(2));
}

void VpQuickEntityOperationTest::reverseSelectionCreatesContiguousBlockAndUndoRedo()
{
    VpCadDocument document;
    const QString layer = document.currentLayerName();
    const VpEntityId first = addLine(document, layer, {0.0, 0.0}, {1.0, 0.0});
    const VpEntityId fixed = addLine(document, layer, {2.0, 0.0}, {3.0, 0.0});
    const VpEntityId third = addLine(document, layer, {4.0, 0.0}, {5.0, 0.0});
    const VpEntityId last = addLine(document, layer, {6.0, 0.0}, {7.0, 0.0});

    applyQuickEntityOperation(document, {first, third}, VpQuickEntityOperation::Reverse, {});

    QCOMPARE(document.entities()[0].id, third);
    QCOMPARE(document.entities()[1].id, first);
    QCOMPARE(document.entities()[2].id, fixed);
    QCOMPARE(document.entities()[3].id, last);
    comparePoint(std::get<VpLineEntity>(entityById(document, third).geometry).start_point,
                 {5.0, 0.0});
    document.undo();
    QCOMPARE(document.entities()[0].id, first);
    QCOMPARE(document.entities()[1].id, fixed);
    QCOMPARE(document.entities()[2].id, third);
    comparePoint(std::get<VpLineEntity>(entityById(document, third).geometry).start_point,
                 {4.0, 0.0});
    document.redo();
    QCOMPARE(document.entities()[0].id, third);
    comparePoint(std::get<VpLineEntity>(entityById(document, third).geometry).start_point,
                 {5.0, 0.0});
}

void VpQuickEntityOperationTest::allEntityScopeIncludesHiddenAndLockedLayers()
{
    VpCadDocument document;
    QVERIFY(document.addLayer(QStringLiteral("HIDDEN")));
    QVERIFY(document.addLayer(QStringLiteral("LOCKED")));
    const VpEntityId hidden = addLine(document, QStringLiteral("HIDDEN"), {0.0, 0.0}, {1.0, 0.0});
    const VpEntityId locked = addLine(document, QStringLiteral("LOCKED"), {2.0, 0.0}, {3.0, 0.0});
    QVERIFY(document.setLayerVisible(QStringLiteral("HIDDEN"), false));
    QVERIFY(document.setLayerLocked(QStringLiteral("LOCKED"), true));

    const std::vector<VpEntityId> targets = quickOperationTargetIds(document, {});
    QCOMPARE(targets, std::vector<VpEntityId>({hidden, locked}));
    applyQuickEntityOperation(document, {}, VpQuickEntityOperation::RotateCounterclockwise180,
                              {1.5, 0.0});
    comparePoint(std::get<VpLineEntity>(entityById(document, hidden).geometry).start_point,
                 {3.0, 0.0});
    comparePoint(std::get<VpLineEntity>(entityById(document, locked).geometry).start_point,
                 {1.0, 0.0});
}

void VpQuickEntityOperationTest::preferredColorCreatesLayerLazilyAndCopyKeepsLayer()
{
    VpCadDocument document;
    const std::size_t initial_layer_count = document.layers().size();
    const QColor preferred(QStringLiteral("#12AB34"));
    document.setPreferredDrawingColor(preferred);
    QCOMPARE(document.layers().size(), initial_layer_count);

    std::unique_ptr<VpDocumentTransaction> transaction =
        document.beginTransaction(QStringLiteral("首选颜色新实体"));
    const VpEntityId colored = transaction->addLine({0.0, 0.0}, {1.0, 0.0});
    transaction->commit();
    QCOMPARE(document.layers().size(), initial_layer_count + 1);
    QCOMPARE(document.layerColor(entityById(document, colored).layer_name), preferred);

    VpEntityRecord copy_source = entityById(document, colored);
    copy_source.layer_name = document.currentLayerName();
    std::unique_ptr<VpDocumentTransaction> copy_transaction =
        document.beginTransaction(QStringLiteral("复制不重染"));
    const VpEntityId copy = copy_transaction->addEntityCopy(copy_source);
    copy_transaction->commit();
    QCOMPARE(entityById(document, copy).layer_name, document.currentLayerName());
}

void VpQuickEntityOperationTest::selectionRightClickRequestsContextBarOnlyInSelectMode()
{
    VpCadDocument document;
    const VpEntityId entity_id =
        addLine(document, document.currentLayerName(), {0.0, 0.0}, {1.0, 0.0});
    VpCadViewport viewport;
    viewport.resize(320, 240);
    viewport.setDocument(&document);
    viewport.setSelectedEntityIds({entity_id});
    viewport.setToolMode(VpToolMode::Select);
    QSignalSpy context_spy(&viewport, &VpCadViewport::selectionContextRequested);

    QTest::mouseClick(&viewport, Qt::RightButton, Qt::NoModifier, QPoint(100, 100));
    QCOMPARE(context_spy.count(), 1);

    viewport.setSelectedEntityIds({entity_id});
    viewport.setToolMode(VpToolMode::Line);
    QTest::mouseClick(&viewport, Qt::RightButton, Qt::NoModifier, QPoint(100, 100));
    QCOMPARE(context_spy.count(), 1);
}

void VpQuickEntityOperationTest::quickBarStaysInsideWorkspaceAndCommandsAreDiscoverable()
{
    VpCadWorkspaceWidget workspace;
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

} // namespace Vp

VECTORPATH_TEST_ENTRY(Vp::VpQuickEntityOperationTest, vpRunVpQuickEntityOperationTest)
#include "vp_quick_entity_operation_test.moc"
