#include "s_standard_shape.h"

#include <QtTest>
#include <cmath>

namespace vectorPath
{

class SStandardShapeTest final : public QObject
{
    Q_OBJECT

  private slots:
    void createsExpectedVertexCounts();
    void createsAlternatingStarRadii();
};

void SStandardShapeTest::createsExpectedVertexCounts()
{
    const SPoint2d center{10.0, 20.0};
    const SPoint2d radius_point{20.0, 20.0};
    QCOMPARE(standardShapeVertices(SStandardShapeType::FivePointStar, center, radius_point).size(),
             std::size_t(10));
    QCOMPARE(standardShapeVertices(SStandardShapeType::Triangle, center, radius_point).size(),
             std::size_t(3));
    QCOMPARE(standardShapeVertices(SStandardShapeType::Pentagon, center, radius_point).size(),
             std::size_t(5));
    QCOMPARE(standardShapeVertices(SStandardShapeType::Hexagon, center, radius_point).size(),
             std::size_t(6));
    QCOMPARE(standardShapeVertices(SStandardShapeType::Octagon, center, radius_point).size(),
             std::size_t(8));
    QCOMPARE(standardShapeVertices(SStandardShapeType::Diamond, center, radius_point).size(),
             std::size_t(4));
}

void SStandardShapeTest::createsAlternatingStarRadii()
{
    const SPoint2d center{0.0, 0.0};
    const std::vector<SPoint2d> vertices =
        standardShapeVertices(SStandardShapeType::FivePointStar, center, {10.0, 0.0});
    for (std::size_t index = 0; index < vertices.size(); ++index)
    {
        const double radius = std::hypot(vertices[index].x, vertices[index].y);
        const double expected_radius = index % 2 == 0 ? 10.0 : 3.819660112501051;
        QVERIFY(std::abs(radius - expected_radius) < 1.0e-9);
    }
}

} // namespace vectorPath

QTEST_APPLESS_MAIN(vectorPath::SStandardShapeTest)
#include "s_standard_shape_test.moc"
