#include "vp_cad_document.h"
#include "vp_document_transaction.h"
#include "vp_dwg_codec.h"
#include "vp_qt_text.h"
#include "vp_test_runner.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using namespace Vp;

class VpDwgCodecTest final : public QObject
{
    Q_OBJECT

  private slots:
    void bundledToolIsAvailable();
    void r2000RoundTrip();
};

void VpDwgCodecTest::bundledToolIsAvailable()
{
    const VpDwgCodec codec;
    QVERIFY(codec.isAvailable());
    QVERIFY(codec.toolVersion().contains(QStringLiteral("0.14")));
    QCOMPARE(codec.extensions(), QStringList{QStringLiteral("dwg")});
}

void VpDwgCodecTest::r2000RoundTrip()
{
    VpCadDocument source_document;
    QVERIFY(source_document.addLayer(QStringLiteral("GEOMETRY")));
    QVERIFY(source_document.setCurrentLayer(QStringLiteral("GEOMETRY")));
    auto transaction = source_document.beginTransaction(QStringLiteral("DWG fixture"));
    transaction->addLine({0.0, 0.0}, {120.0, 40.0});
    transaction->addCircle({50.0, 60.0}, 20.0);
    transaction->addArc({80.0, 30.0}, 15.0, 10.0, 150.0);
    transaction->addPolyline({{0.0, 0.0}, {25.0, 10.0}, {40.0, 0.0}}, false);
    transaction->commit();
    auto spline_transaction = source_document.beginTransaction(QStringLiteral("DWG spline"));
    spline_transaction->addSpline({VpPoint2d{0.0, 100.0}, VpPoint2d{20.0, 120.0},
                                   VpPoint2d{40.0, 80.0}, VpPoint2d{60.0, 100.0}});
    spline_transaction->commit();

    QTemporaryDir temporary_directory;
    QVERIFY(temporary_directory.isValid());
    const QString file_path = temporary_directory.filePath(QStringLiteral("roundtrip.dwg"));
    const VpDwgCodec codec;
    const VpResult<VpFileCompatibilityReport> write_result =
        codec.write(file_path, source_document);
    QVERIFY2(write_result.isSuccess(), qPrintable(toQtError(write_result)));
    QVERIFY(write_result.value().hasWarnings());
    QFile dwg_file(file_path);
    QVERIFY(dwg_file.open(QIODevice::ReadOnly));
    QCOMPARE(dwg_file.read(6), QByteArray("AC1015"));

    VpCadDocument loaded_document;
    const VpResult<VpFileCompatibilityReport> read_result = codec.read(file_path, loaded_document);
    QVERIFY2(read_result.isSuccess(), qPrintable(toQtError(read_result)));
    QCOMPARE(loaded_document.entities().size(), std::size_t(5));
    QCOMPARE(read_result.value().imported_entity_count, std::size_t(5));
    QCOMPARE(loaded_document.entities().back().type, VpEntityType::Spline);
    QVERIFY(read_result.value().hasWarnings());
}

VECTORPATH_TEST_ENTRY(VpDwgCodecTest, vpRunVpDwgCodecTest)
#include "vp_dwg_codec_test.moc"
