#include "vp_qt_text.h"

#include <QTest>

namespace Vp
{

class VpQtAdapterTest final : public QObject
{
    Q_OBJECT

  private slots:
    void preservesUtf16CodeUnits()
    {
        // Preserve embedded NUL and unpaired surrogates as well as valid Unicode.
        const std::u16string original{u'中', u'文', 0, 0xd83d, 0xde42, 0xd800, u'x'};
        const QString qt_text = toQtText(original);
        QVERIFY(toCoreText(qt_text) == original);
        QCOMPARE(qt_text.size(), static_cast<int>(original.size()));
        QVERIFY(toCoreText(QString()).empty());
    }

    void preservesNativeDiagnosticConversion()
    {
        const QByteArray bytes = QStringLiteral("本地错误").toLocal8Bit();
        const auto result = VpResult<void>::failureWithNativeDetail(
            u"计算失败：", std::string(bytes.constData(), static_cast<std::size_t>(bytes.size())));
        QCOMPARE(toQtError(result), QStringLiteral("计算失败：") + QString::fromLocal8Bit(bytes));
        QCOMPARE(toQtError(VpResult<void>::failure(u"失败")), QStringLiteral("失败"));
        QVERIFY(toQtError(VpResult<void>::success()).isEmpty());
    }
};

} // namespace Vp

QTEST_APPLESS_MAIN(Vp::VpQtAdapterTest)
#include "vp_qt_adapter_test.moc"
