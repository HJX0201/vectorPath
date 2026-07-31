#include "s_cad_document.h"
#include "s_document_transaction.h"

#include <QtTest>
#include <limits>

namespace smartCam
{

class SDocumentAuditTest final : public QObject
{
    Q_OBJECT

  private slots:
    void detectRepairAndUndo();
    void cleanDocumentReport();
};

void SDocumentAuditTest::detectRepairAndUndo()
{
    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("audit fixtures"));
    transaction->addCircle({0.0, 0.0}, -2.0);
    transaction->addLine({1.0, 1.0}, {1.0, 1.0});
    SEntityRecord polyline;
    polyline.type = SEntityType::Polyline;
    polyline.line_width_mm = -5.0;
    polyline.geometry = SPolylineEntity{{{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}}, false, {0.0}};
    transaction->addEntityCopy(polyline);
    transaction->commit();
    QCOMPARE(document.entities().size(), std::size_t(3));

    const SDocumentAuditReport check_report = document.audit(false);
    QCOMPARE(check_report.scanned_entity_count, 3);
    QCOMPARE(check_report.issue_count, 3);
    QCOMPARE(check_report.removed_entity_count, 0);
    QCOMPARE(document.entities().size(), std::size_t(3));

    const SDocumentAuditReport repair_report = document.audit(true);
    QCOMPARE(repair_report.issue_count, 3);
    QCOMPARE(repair_report.removed_entity_count, 2);
    QCOMPARE(repair_report.repaired_entity_count, 1);
    QCOMPARE(document.entities().size(), std::size_t(1));
    const SEntityRecord& repaired = document.entities().front();
    QCOMPARE(repaired.line_width_mm, -1.0);
    const auto& repaired_polyline = std::get<SPolylineEntity>(repaired.geometry);
    QCOMPARE(repaired_polyline.bulges.size(), repaired_polyline.vertices.size());
    QCOMPARE(repaired_polyline.start_widths.size(), repaired_polyline.vertices.size());
    QCOMPARE(repaired_polyline.end_widths.size(), repaired_polyline.vertices.size());

    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(3));
}

void SDocumentAuditTest::cleanDocumentReport()
{
    SCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("valid geometry"));
    transaction->addCircle({2.0, 3.0}, 4.0);
    transaction->addLine({0.0, 0.0}, {10.0, 0.0});
    transaction->commit();
    const SDocumentAuditReport report = document.audit(false);
    QVERIFY(report.isClean());
    QCOMPARE(report.scanned_entity_count, 2);
}

} // namespace smartCam

QTEST_MAIN(smartCam::SDocumentAuditTest)
#include "s_document_audit_test.moc"
