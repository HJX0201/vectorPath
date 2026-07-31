#include "s_polygon_boolean.h"

#include <QtTest>
#include <cmath>

namespace vectorPath
{
namespace
{

double pathArea(const SPolygonPath& path)
{
    double twice_area = 0.0;
    for (std::size_t index = 0; index < path.size(); ++index)
    {
        const SPoint2d& first = path[index];
        const SPoint2d& second = path[(index + 1) % path.size()];
        twice_area += first.x * second.y - second.x * first.y;
    }
    return std::abs(twice_area) * 0.5;
}

double pathsArea(const SPolygonPaths& paths)
{
    double area = 0.0;
    for (const SPolygonPath& path : paths)
    {
        area += pathArea(path);
    }
    return area;
}

} // namespace

class SPolygonBooleanTest final : public QObject
{
    Q_OBJECT

  private slots:
    void clipsOverlappingRectangles_data();
    void clipsOverlappingRectangles();
};

void SPolygonBooleanTest::clipsOverlappingRectangles_data()
{
    QTest::addColumn<int>("operation");
    QTest::addColumn<double>("expected_area");
    QTest::newRow("union") << static_cast<int>(SPolygonBooleanOperation::Union) << 150.0;
    QTest::newRow("intersection")
        << static_cast<int>(SPolygonBooleanOperation::Intersection) << 50.0;
    QTest::newRow("difference")
        << static_cast<int>(SPolygonBooleanOperation::Difference) << 50.0;
    QTest::newRow("xor") << static_cast<int>(SPolygonBooleanOperation::Xor) << 100.0;
    QTest::newRow("complement")
        << static_cast<int>(SPolygonBooleanOperation::Complement) << 50.0;
}

void SPolygonBooleanTest::clipsOverlappingRectangles()
{
    QFETCH(int, operation);
    QFETCH(double, expected_area);
    const SPolygonPaths subjects{{{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {0.0, 10.0}}};
    const SPolygonPaths clips{{{5.0, 0.0}, {15.0, 0.0}, {15.0, 10.0}, {5.0, 10.0}}};
    const auto result = polygonBoolean(
        subjects, clips, static_cast<SPolygonBooleanOperation>(operation));
    QVERIFY2(result.isSuccess(), qPrintable(result.errorMessage()));
    QVERIFY(std::abs(pathsArea(result.value()) - expected_area) < 1.0e-6);
}

} // namespace vectorPath

QTEST_APPLESS_MAIN(vectorPath::SPolygonBooleanTest)
#include "s_polygon_boolean_test.moc"
