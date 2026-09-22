#include "vp_standard_shape.h"

#include <QtTest>
#include <cmath>

namespace Vp
{

class VpStandardShapeTest final : public QObject
{
    Q_OBJECT

  private slots:
    void createsExpectedVertexCounts();
    void createsAlternatingStarRadii();
};

void VpStandardShapeTest::createsExpectedVertexCounts()
{
    const VpPoint2d center{10.0, 20.0};
    const VpPoint2d radius_point{20.0, 20.0};
    QCOMPARE(standardShapeVertices(VpStandardShapeType::FivePointStar, center, radius_point).size(),
             std::size_t(10));
    QCOMPARE(standardShapeVertices(VpStandardShapeType::Triangle, center, radius_point).size(),
             std::size_t(3));
    QCOMPARE(standardShapeVertices(VpStandardShapeType::Pentagon, center, radius_point).size(),
             std::size_t(5));
    QCOMPARE(standardShapeVertices(VpStandardShapeType::Hexagon, center, radius_point).size(),
             std::size_t(6));
    QCOMPARE(standardShapeVertices(VpStandardShapeType::Octagon, center, radius_point).size(),
             std::size_t(8));
    QCOMPARE(standardShapeVertices(VpStandardShapeType::Diamond, center, radius_point).size(),
             std::size_t(4));
}

void VpStandardShapeTest::createsAlternatingStarRadii()
{
    const VpPoint2d center{0.0, 0.0};
    const std::vector<VpPoint2d> vertices =
        standardShapeVertices(VpStandardShapeType::FivePointStar, center, {10.0, 0.0});
    for (std::size_t index = 0; index < vertices.size(); ++index)
    {
        const double radius = std::hypot(vertices[index].x, vertices[index].y);
        const double expected_radius = index % 2 == 0 ? 10.0 : 3.819660112501051;
        QVERIFY(std::abs(radius - expected_radius) < 1.0e-9);
    }
}

} // namespace Vp

QTEST_APPLESS_MAIN(Vp::VpStandardShapeTest)
#include "vp_standard_shape_test.moc"
