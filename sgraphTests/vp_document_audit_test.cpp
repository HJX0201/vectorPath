#include "vp_cad_document.h"
#include "vp_document_transaction.h"
#include "vp_test_runner.h"

#include <QtTest>
#include <limits>

namespace Vp
{

class VpDocumentAuditTest final : public QObject
{
    Q_OBJECT

  private slots:
    void detectRepairAndUndo();
    void cleanDocumentReport();
};

void VpDocumentAuditTest::detectRepairAndUndo()
{
    VpCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("audit fixtures"));
    transaction->addCircle({0.0, 0.0}, -2.0);
    transaction->addLine({1.0, 1.0}, {1.0, 1.0});
    VpEntityRecord polyline;
    polyline.type = VpEntityType::Polyline;
    polyline.line_width_mm = -5.0;
    polyline.geometry = VpPolylineEntity{{{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}}, false, {0.0}};
    transaction->addEntityCopy(polyline);
    transaction->commit();
    QCOMPARE(document.entities().size(), std::size_t(3));

    const VpDocumentAuditReport check_report = document.audit(false);
    QCOMPARE(check_report.scanned_entity_count, 3);
    QCOMPARE(check_report.issue_count, 3);
    QCOMPARE(check_report.removed_entity_count, 0);
    QCOMPARE(document.entities().size(), std::size_t(3));

    const VpDocumentAuditReport repair_report = document.audit(true);
    QCOMPARE(repair_report.issue_count, 3);
    QCOMPARE(repair_report.removed_entity_count, 2);
    QCOMPARE(repair_report.repaired_entity_count, 1);
    QCOMPARE(document.entities().size(), std::size_t(1));
    const VpEntityRecord& repaired = document.entities().front();
    QCOMPARE(repaired.line_width_mm, -1.0);
    const auto& repaired_polyline = std::get<VpPolylineEntity>(repaired.geometry);
    QCOMPARE(repaired_polyline.bulges.size(), repaired_polyline.vertices.size());
    QCOMPARE(repaired_polyline.start_widths.size(), repaired_polyline.vertices.size());
    QCOMPARE(repaired_polyline.end_widths.size(), repaired_polyline.vertices.size());

    document.undo();
    QCOMPARE(document.entities().size(), std::size_t(3));
}

void VpDocumentAuditTest::cleanDocumentReport()
{
    VpCadDocument document;
    auto transaction = document.beginTransaction(QStringLiteral("valid geometry"));
    transaction->addCircle({2.0, 3.0}, 4.0);
    transaction->addLine({0.0, 0.0}, {10.0, 0.0});
    transaction->commit();
    const VpDocumentAuditReport report = document.audit(false);
    QVERIFY(report.isClean());
    QCOMPARE(report.scanned_entity_count, 2);
}

} // namespace Vp

VECTORPATH_TEST_ENTRY(Vp::VpDocumentAuditTest, vpRunVpDocumentAuditTest)
#include "vp_document_audit_test.moc"
