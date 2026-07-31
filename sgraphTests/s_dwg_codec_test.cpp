#include "s_cad_document.h"
#include "s_document_transaction.h"
#include "s_dwg_codec.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using namespace vectorPath;

class SDwgCodecTest final : public QObject
{
    Q_OBJECT

  private slots:
    void bundledToolIsAvailable();
    void r2000RoundTrip();
};

void SDwgCodecTest::bundledToolIsAvailable()
{
    const SDwgCodec codec;
    QVERIFY(codec.isAvailable());
    QVERIFY(codec.toolVersion().contains(QStringLiteral("0.14")));
    QCOMPARE(codec.extensions(), QStringList{QStringLiteral("dwg")});
}

void SDwgCodecTest::r2000RoundTrip()
{
    SCadDocument source_document;
    QVERIFY(source_document.addLayer(QStringLiteral("GEOMETRY")));
    QVERIFY(source_document.setCurrentLayer(QStringLiteral("GEOMETRY")));
    auto transaction = source_document.beginTransaction(QStringLiteral("DWG fixture"));
    transaction->addLine({0.0, 0.0}, {120.0, 40.0});
    transaction->addCircle({50.0, 60.0}, 20.0);
    transaction->addArc({80.0, 30.0}, 15.0, 10.0, 150.0);
    transaction->addPolyline({{0.0, 0.0}, {25.0, 10.0}, {40.0, 0.0}}, false);
    transaction->commit();
    auto spline_transaction = source_document.beginTransaction(QStringLiteral("DWG spline"));
    spline_transaction->addSpline(
        {SPoint2d{0.0, 100.0}, SPoint2d{20.0, 120.0}, SPoint2d{40.0, 80.0},
         SPoint2d{60.0, 100.0}});
    spline_transaction->commit();

    QTemporaryDir temporary_directory;
    QVERIFY(temporary_directory.isValid());
    const QString file_path = temporary_directory.filePath(QStringLiteral("roundtrip.dwg"));
    const SDwgCodec codec;
    const SResult<SFileCompatibilityReport> write_result = codec.write(file_path, source_document);
    QVERIFY2(write_result.isSuccess(), qPrintable(write_result.errorMessage()));
    QVERIFY(write_result.value().hasWarnings());
    QFile dwg_file(file_path);
    QVERIFY(dwg_file.open(QIODevice::ReadOnly));
    QCOMPARE(dwg_file.read(6), QByteArray("AC1015"));

    SCadDocument loaded_document;
    const SResult<SFileCompatibilityReport> read_result = codec.read(file_path, loaded_document);
    QVERIFY2(read_result.isSuccess(), qPrintable(read_result.errorMessage()));
    QCOMPARE(loaded_document.entities().size(), std::size_t(5));
    QCOMPARE(read_result.value().imported_entity_count, std::size_t(5));
    QCOMPARE(loaded_document.entities().back().type, SEntityType::Spline);
    QVERIFY(read_result.value().hasWarnings());
}

QTEST_MAIN(SDwgCodecTest)
#include "s_dwg_codec_test.moc"
