#include "s_cad_document.h"
#include "s_cad_viewport.h"
#include "s_cad_viewport_geometry.h"
#include "s_dxf_codec.h"
#include "s_ellipse_geometry.h"
#include "s_object_snap.h"

#include <QTemporaryDir>
#include <QtTest>
#include <cmath>

namespace smartGraphics
{

class SEllipseTest final : public QObject
{
    Q_OBJECT

  private slots:
    void interactivePersistenceAndDxf();
    void geometryTransformGripAndSnap();
};

void SEllipseTest::interactivePersistenceAndDxf()
{
    SCadDocument document;
    SCadViewport viewport;
    viewport.setDocument(&document);
    viewport.setObjectSnapEnabled(false);
    viewport.setToolMode(SToolMode::Ellipse);
    viewport.submitWorldPoint({-10.0, 0.0});
    viewport.submitWorldPoint({10.0, 0.0});
    viewport.submitWorldPoint({0.0, 5.0});
    QCOMPARE(document.entities().size(), std::size_t(1));
    QCOMPARE(document.entities().front().type, SEntityType::Ellipse);
    const auto& ellipse = std::get<SEllipseEntity>(document.entities().front().geometry);
    QCOMPARE(ellipse.center.x, 0.0);
    QCOMPARE(std::hypot(ellipse.major_axis.x, ellipse.major_axis.y), 10.0);
    QCOMPARE(std::hypot(ellipse.minor_axis.x, ellipse.minor_axis.y), 5.0);
    QCOMPARE(ellipseApproximation(ellipse).size(), std::size_t(97));

    QTemporaryDir temporary_directory;
    QVERIFY(temporary_directory.isValid());
    const QString native_path = temporary_directory.filePath(QStringLiteral("ellipse.smartcad"));
    QVERIFY(document.save(native_path).isSuccess());
    SCadDocument loaded_document;
    QVERIFY(loaded_document.load(native_path).isSuccess());
    QCOMPARE(loaded_document.entities().front().type, SEntityType::Ellipse);

    const QString dxf_path = temporary_directory.filePath(QStringLiteral("ellipse.dxf"));
    SDxfCodec codec;
    QVERIFY(codec.write(dxf_path, document).isSuccess());
    SCadDocument dxf_document;
    QVERIFY(codec.read(dxf_path, dxf_document).isSuccess());
    QCOMPARE(dxf_document.entities().size(), std::size_t(1));
    const auto& dxf_ellipse = std::get<SEllipseEntity>(dxf_document.entities().front().geometry);
    QVERIFY(std::abs(std::hypot(dxf_ellipse.minor_axis.x, dxf_ellipse.minor_axis.y) - 5.0) <
            1.0e-9);
}

void SEllipseTest::geometryTransformGripAndSnap()
{
    SEntityRecord source;
    source.id = 501;
    source.type = SEntityType::Ellipse;
    source.geometry = SEllipseEntity{{2.0, 3.0}, {8.0, 0.0}, {0.0, 4.0}};
    const SEntityRecord rotated = rotatedEntity(source, {2.0, 3.0}, 90.0);
    const auto& rotated_ellipse = std::get<SEllipseEntity>(rotated.geometry);
    QVERIFY(std::abs(rotated_ellipse.major_axis.x) < 1.0e-9);
    QVERIFY(std::abs(rotated_ellipse.major_axis.y - 8.0) < 1.0e-9);

    const std::vector<SGripHandle> grips = entityGripHandles(source);
    QCOMPARE(grips.size(), std::size_t(5));
    SEntityRecord edited;
    QVERIFY(gripEditedEntity(source, grips[1], {12.0, 3.0}, edited));
    QCOMPARE(std::get<SEllipseEntity>(edited.geometry).major_axis.x, 10.0);

    const std::vector<const SEntityRecord*> entities{&source};
    const auto center_snap = findObjectSnap(entities, {2.01, 3.0}, std::nullopt, 0.1);
    QVERIFY(center_snap.has_value());
    QCOMPARE(center_snap->type, SObjectSnapType::Center);
}

} // namespace smartGraphics

QTEST_MAIN(smartGraphics::SEllipseTest)
#include "s_ellipse_test.moc"
