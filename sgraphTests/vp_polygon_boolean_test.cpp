#include "vp_polygon_boolean.h"
#include "vp_qt_text.h"

#include <QtTest>
#include <cmath>

namespace Vp
{
namespace
{

double pathArea(const VpPolygonPath& path)
{
    double twice_area = 0.0;
    for (std::size_t index = 0; index < path.size(); ++index)
    {
        const VpPoint2d& first = path[index];
        const VpPoint2d& second = path[(index + 1) % path.size()];
        twice_area += first.x * second.y - second.x * first.y;
    }
    return std::abs(twice_area) * 0.5;
}

double pathsArea(const VpPolygonPaths& paths)
{
    double area = 0.0;
    for (const VpPolygonPath& path : paths)
    {
        area += pathArea(path);
    }
    return area;
}

} // namespace

class VpPolygonBooleanTest final : public QObject
{
    Q_OBJECT

  private slots:
    void clipsOverlappingRectangles_data();
    void clipsOverlappingRectangles();
};

void VpPolygonBooleanTest::clipsOverlappingRectangles_data()
{
    QTest::addColumn<int>("operation");
    QTest::addColumn<double>("expected_area");
    QTest::newRow("union") << static_cast<int>(VpPolygonBooleanOperation::Union) << 150.0;
    QTest::newRow("intersection") << static_cast<int>(VpPolygonBooleanOperation::Intersection)
                                  << 50.0;
    QTest::newRow("difference") << static_cast<int>(VpPolygonBooleanOperation::Difference) << 50.0;
    QTest::newRow("xor") << static_cast<int>(VpPolygonBooleanOperation::Xor) << 100.0;
    QTest::newRow("complement") << static_cast<int>(VpPolygonBooleanOperation::Complement) << 50.0;
}

void VpPolygonBooleanTest::clipsOverlappingRectangles()
{
    QFETCH(int, operation);
    QFETCH(double, expected_area);
    const VpPolygonPaths subjects{{{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {0.0, 10.0}}};
    const VpPolygonPaths clips{{{5.0, 0.0}, {15.0, 0.0}, {15.0, 10.0}, {5.0, 10.0}}};
    const auto result =
        polygonBoolean(subjects, clips, static_cast<VpPolygonBooleanOperation>(operation));
    QVERIFY2(result.isSuccess(), qPrintable(toQtError(result)));
    QVERIFY(std::abs(pathsArea(result.value()) - expected_area) < 1.0e-6);
}

} // namespace Vp

QTEST_APPLESS_MAIN(Vp::VpPolygonBooleanTest)
#include "vp_polygon_boolean_test.moc"
