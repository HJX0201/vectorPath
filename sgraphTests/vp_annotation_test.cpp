#include "vp_cad_document.h"
#include "vp_cad_viewport.h"
#include "vp_cad_viewport_geometry.h"
#include "vp_document_transaction.h"
#include "vp_dxf_codec.h"
#include "vp_test_runner.h"

#include <QTemporaryDir>
#include <QtTest>

namespace Vp
{

class VpAnnotationTest final : public QObject
{
    Q_OBJECT

  private slots:
    void textStyleUndoAndPersistence();
    void annotationPersistenceAndTransforms();
    void viewportCreationAndUndo();
    void mtextDxfCompatibility();
};

VpTextStyleRecord notesStyle()
{
    VpTextStyleRecord style;
    style.name = QStringLiteral("Notes");
    style.font_family = QStringLiteral("Microsoft YaHei UI");
    style.fixed_height = 3.5;
    style.width_factor = 0.9;
    style.oblique_angle = 8.0;
    style.is_bold = true;
    return style;
}

void VpAnnotationTest::textStyleUndoAndPersistence()
{
    QTemporaryDir temporary_directory;
    QVERIFY(temporary_directory.isValid());
    VpCadDocument document;
    QVERIFY(document.addOrUpdateTextStyle(notesStyle()));
    QCOMPARE(document.textStyles().size(), std::size_t(2));
    QVERIFY(document.setCurrentTextStyle(QStringLiteral("Notes")));
    QCOMPARE(document.currentTextStyleName(), QStringLiteral("Notes"));
    document.undo();
    QCOMPARE(document.currentTextStyleName(), QStringLiteral("Standard"));
    document.redo();
    QCOMPARE(document.currentTextStyleName(), QStringLiteral("Notes"));

    const QString file_path = temporary_directory.filePath(QStringLiteral("styles.smartcad"));
    QVERIFY(document.save(file_path).isSuccess());
    VpCadDocument loaded_document;
    QVERIFY(loaded_document.load(file_path).isSuccess());
    QCOMPARE(loaded_document.currentTextStyleName(), QStringLiteral("Notes"));
    const VpTextStyleRecord* loaded_style = loaded_document.textStyle(QStringLiteral("notes"));
    QVERIFY(loaded_style);
    QCOMPARE(loaded_style->font_family, QStringLiteral("Microsoft YaHei UI"));
    QCOMPARE(loaded_style->fixed_height, 3.5);
    QVERIFY(loaded_style->is_bold);
}

void VpAnnotationTest::annotationPersistenceAndTransforms()
{
    QTemporaryDir temporary_directory;
    QVERIFY(temporary_directory.isValid());
    VpCadDocument document;
    QVERIFY(document.addOrUpdateTextStyle(notesStyle()));
    QVERIFY(document.setCurrentTextStyle(QStringLiteral("Notes")));
    auto transaction = document.beginTransaction(QStringLiteral("annotations"));
    transaction->addMText({10.0, 20.0}, QStringLiteral("<b>第一行</b><br/>第二行"), 35.0, 3.5, 15.0,
                          VpTextHorizontalAlignment::Center);
    transaction->addLeader({{0.0, 0.0}, {5.0, 8.0}, {15.0, 8.0}}, QStringLiteral("检查此处"), 3.5,
                           2.0);
    transaction->addText({20.0, 5.0}, QStringLiteral("居中文字"), 3.5, 0.0,
                         VpTextHorizontalAlignment::Center);
    transaction->commit();
    const QString file_path = temporary_directory.filePath(QStringLiteral("annotation.smartcad"));
    QVERIFY(document.save(file_path).isSuccess());

    VpCadDocument loaded_document;
    QVERIFY(loaded_document.load(file_path).isSuccess());
    QCOMPARE(loaded_document.entities().size(), std::size_t(3));
    QCOMPARE(loaded_document.entities()[0].type, VpEntityType::MText);
    const auto& loaded_text = std::get<VpMTextEntity>(loaded_document.entities()[0].geometry);
    QCOMPARE(loaded_text.width, 35.0);
    QCOMPARE(loaded_text.style_name, QStringLiteral("Notes"));
    QCOMPARE(loaded_text.horizontal_alignment, VpTextHorizontalAlignment::Center);
    const auto& loaded_leader = std::get<VpLeaderEntity>(loaded_document.entities()[1].geometry);
    QCOMPARE(loaded_leader.vertices.size(), std::size_t(3));
    QCOMPARE(loaded_leader.style_name, QStringLiteral("Notes"));
    QCOMPARE(std::get<VpTextEntity>(loaded_document.entities()[2].geometry).horizontal_alignment,
             VpTextHorizontalAlignment::Center);

    const VpEntityRecord moved = translatedEntity(loaded_document.entities()[1], 2.0, 3.0);
    QCOMPARE(std::get<VpLeaderEntity>(moved.geometry).vertices.front().x, 2.0);
    QCOMPARE(std::get<VpLeaderEntity>(moved.geometry).vertices.back().y, 11.0);
    QCOMPARE(entityGripHandles(loaded_document.entities()[1]).size(), std::size_t(3));
}

void VpAnnotationTest::viewportCreationAndUndo()
{
    VpCadDocument document;
    VpCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setPendingMText(QStringLiteral("<i>多行文字</i>"), 30.0, 3.0,
                             VpTextHorizontalAlignment::Right);
    viewport.setToolMode(VpToolMode::MText);
    viewport.submitWorldPoint({4.0, 5.0});
    QCOMPARE(document.entities().size(), std::size_t(1));
    QCOMPARE(document.entities().front().type, VpEntityType::MText);

    viewport.setPendingLeader(QStringLiteral("说明"), 2.5, 2.0);
    viewport.setToolMode(VpToolMode::Leader);
    viewport.submitWorldPoint({0.0, 0.0});
    viewport.submitWorldPoint({4.0, 4.0});
    viewport.submitWorldPoint({12.0, 4.0});
    QCOMPARE(document.entities().size(), std::size_t(2));
    QCOMPARE(document.entities().back().type, VpEntityType::Leader);
    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(1));
    document.undo();
    QVERIFY(document.entities().empty());
}

void VpAnnotationTest::mtextDxfCompatibility()
{
    QTemporaryDir temporary_directory;
    QVERIFY(temporary_directory.isValid());
    VpCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("dxf annotations"));
    transaction->addMText({2.0, 3.0}, QStringLiteral("<b>Rich</b> text"), 25.0, 3.0);
    transaction->addLeader({{0.0, 0.0}, {3.0, 3.0}}, QStringLiteral("Leader"));
    transaction->commit();
    const QString file_path = temporary_directory.filePath(QStringLiteral("annotation.dxf"));
    VpDxfCodec codec;
    const auto write_result = codec.write(file_path, document);
    QVERIFY(write_result.isSuccess());
    QCOMPARE(write_result.value().exported_entity_count, 1);
    QCOMPARE(write_result.value().skipped_entity_count, 1);
    QVERIFY(
        write_result.value().warnings.join(QLatin1Char('\n')).contains(QStringLiteral("富文本")));
    QVERIFY(
        write_result.value().warnings.join(QLatin1Char('\n')).contains(QStringLiteral("多重引线")));

    VpCadDocument loaded_document;
    const auto read_result = codec.read(file_path, loaded_document);
    QVERIFY(read_result.isSuccess());
    QCOMPARE(loaded_document.entities().size(), std::size_t(1));
    QCOMPARE(loaded_document.entities().front().type, VpEntityType::MText);
    QCOMPARE(std::get<VpMTextEntity>(loaded_document.entities().front().geometry).rich_text,
             QStringLiteral("Rich text"));
}

} // namespace Vp

VECTORPATH_TEST_ENTRY(Vp::VpAnnotationTest, vpRunVpAnnotationTest)
#include "vp_annotation_test.moc"
