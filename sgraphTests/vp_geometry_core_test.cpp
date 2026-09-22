#include "vp_curve_entities.h"
#include "vp_ellipse_geometry.h"
#include "vp_geometry_types.h"
#include "vp_polygon_boolean.h"
#include "vp_polygon_geometry.h"
#include "vp_spline_geometry.h"
#include "vp_standard_shape.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace Vp
{
namespace
{

constexpr double kTolerance = 1.0e-8;
constexpr double kPi = 3.14159265358979323846;

void expect(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

bool near(double actual, double expected)
{
    return std::abs(actual - expected) <= kTolerance;
}

void expectPoint(const VpPoint2d& actual, const VpPoint2d& expected)
{
    expect(near(actual.x, expected.x) && near(actual.y, expected.y),
           "geometry point differs from expected coordinates");
}

void distanceAndStandardShapes()
{
    expect(near(distance({-1.0, -2.0}, {2.0, 2.0}), 5.0), "distance must use both axes");
    expect(near(distance({4.0, 7.0}, {4.0, 7.0}), 0.0), "coincident distance must be zero");
    const std::array<std::pair<VpStandardShapeType, std::size_t>, 6> shapes{
        {{VpStandardShapeType::FivePointStar, 10},
         {VpStandardShapeType::Triangle, 3},
         {VpStandardShapeType::Pentagon, 5},
         {VpStandardShapeType::Hexagon, 6},
         {VpStandardShapeType::Octagon, 8},
         {VpStandardShapeType::Diamond, 4}}};
    const VpPoint2d center{4.0, -3.0};
    const VpPoint2d radius_point{4.0, 7.0};
    for (const auto& shape : shapes)
    {
        const auto vertices = standardShapeVertices(shape.first, center, radius_point);
        expect(vertices.size() == shape.second, "standard shape vertex count must remain stable");
        expectPoint(vertices.front(), radius_point);
        expect(standardShapeVertices(shape.first, center, center).empty(),
               "zero radius must not create a shape");
        if (shape.first != VpStandardShapeType::FivePointStar)
        {
            for (const VpPoint2d& vertex : vertices)
            {
                expect(near(distance(vertex, center), 10.0), "polygon radius must be preserved");
            }
        }
        else
        {
            expect(distance(vertices[1], center) < distance(vertices[0], center),
                   "star must alternate inner and outer vertices");
        }
    }
}

void splineBoundariesAndApproximation()
{
    const VpSplineEntity spline{
        {VpPoint2d{0.0, 0.0}, VpPoint2d{1.0, 0.0}, VpPoint2d{2.0, 0.0}, VpPoint2d{3.0, 0.0}}};
    expectPoint(splinePoint(spline, -1.0), {0.0, 0.0});
    expectPoint(splinePoint(spline, 2.0), {3.0, 0.0});
    expectPoint(splinePoint(spline, 0.5), {1.5, 0.0});
    expectPoint(splineTangent(spline, 0.0), {3.0, 0.0});
    expectPoint(splineTangent(spline, 1.0), {3.0, 0.0});
    const auto minimum = splineApproximation(spline, 0);
    expect(minimum.size() == 5, "spline must retain minimum sampling bound");
    expectPoint(minimum.front(), spline.control_points.front());
    expectPoint(minimum.back(), spline.control_points.back());
    expect(splineApproximation(spline, 10000).size() == 4097,
           "spline must retain maximum sampling bound");
    expect(near(splineApproximateLength(spline), 3.0), "straight spline length must be exact");
    const VpSplineEntity collapsed{};
    expect(near(splineApproximateLength(collapsed), 0.0), "collapsed spline must have zero length");
}

void ellipseBoundariesAndApproximation()
{
    const VpEllipseEntity ellipse{{2.0, -3.0}, {4.0, 0.0}, {0.0, 2.0}};
    expect(isValidEllipse(ellipse), "nonzero ellipse axes must be accepted");
    expectPoint(ellipsePoint(ellipse, 0.0), {6.0, -3.0});
    expectPoint(ellipsePoint(ellipse, kPi * 0.5), {2.0, -1.0});
    const auto minimum = ellipseApproximation(ellipse, 0);
    expect(minimum.size() == 13, "ellipse must retain minimum sampling bound");
    expectPoint(minimum.front(), minimum.back());
    expect(ellipseApproximation(ellipse, 10000).size() == 4097,
           "ellipse must retain maximum sampling bound");
    const VpEllipseEntity collapsed{{2.0, -3.0}, {4.0, 0.0}, {0.0, 0.0}};
    expect(!isValidEllipse(collapsed), "zero minor axis must be rejected");
    expect(ellipseApproximation(collapsed).empty(), "invalid ellipse must produce no preview");
}

void polygonMeasurements()
{
    VpPolygonPath polygon{{-2.0, -1.0}, {4.0, -1.0}, {4.0, 3.0}, {-2.0, 3.0}};
    expect(near(hatchLoopArea(polygon), 24.0), "translated polygon area must be preserved");
    expect(pointInsidePolygon({0.0, 0.0}, polygon), "interior point must be contained");
    expect(!pointInsidePolygon({5.0, 0.0}, polygon), "exterior point must not be contained");
    std::reverse(polygon.begin(), polygon.end());
    expect(near(hatchLoopArea(polygon), 24.0), "area must not depend on winding");
    expect(pointInsidePolygon({0.0, 0.0}, polygon), "containment must not depend on winding");
    expect(near(hatchLoopArea({{0.0, 0.0}, {1.0, 1.0}}), 0.0),
           "incomplete polygon must have zero area");
    expect(!pointInsidePolygon({0.0, 0.0}, {}), "empty polygon must not contain points");
}

void polygonBooleanOperationsAndErrors()
{
    const VpPolygonPaths subjects{{{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {0.0, 10.0}}};
    const VpPolygonPaths clips{{{5.0, 0.0}, {15.0, 0.0}, {15.0, 5.0}, {5.0, 5.0}}};
    const std::array<std::pair<VpPolygonBooleanOperation, double>, 5> operations{
        {{VpPolygonBooleanOperation::Union, 125.0},
         {VpPolygonBooleanOperation::Intersection, 25.0},
         {VpPolygonBooleanOperation::Difference, 75.0},
         {VpPolygonBooleanOperation::Xor, 100.0},
         {VpPolygonBooleanOperation::Complement, 25.0}}};
    for (const auto& operation : operations)
    {
        const auto result = polygonBoolean(subjects, clips, operation.first);
        expect(result.isSuccess(), "valid boolean operation must succeed");
        double area = 0.0;
        for (const VpPolygonPath& path : result.value())
        {
            area += hatchLoopArea(path);
        }
        expect(near(area, operation.second), "boolean result area must match selected operation");
    }
    const auto union_only = polygonBoolean(subjects, {}, VpPolygonBooleanOperation::Union);
    expect(union_only.isSuccess(), "union must accept a single subject");
    expect(union_only.value().size() == 1 && near(hatchLoopArea(union_only.value()[0]), 100.0),
           "single subject union must preserve area");
    const auto empty = polygonBoolean({}, clips, VpPolygonBooleanOperation::Union);
    expect(!empty && empty.errorMessage() == u"布尔运算需要至少一个有效的闭合多段线。",
           "empty input must preserve the user-facing error");
    const auto missing_clip = polygonBoolean(subjects, {}, VpPolygonBooleanOperation::Difference);
    expect(!missing_clip && missing_clip.errorMessage() == u"该布尔运算需要至少两个闭合多段线。",
           "binary operation must require clip geometry");
    const auto invalid =
        polygonBoolean(subjects, {{{0.0, 0.0}, {1.0, 1.0}}}, VpPolygonBooleanOperation::Union);
    expect(!invalid && invalid.value().empty(), "incomplete clip must be rejected without output");
    const VpPolygonPaths oversized{{{0.0, 0.0}, {1.0e20, 0.0}, {1.0e20, 1.0e20}}};
    const auto overflow = polygonBoolean(oversized, {}, VpPolygonBooleanOperation::Union);
    expect(!overflow && overflow.errorMessage() == u"Clipper2 布尔运算失败：",
           "Clipper range errors must be caught and keep the translated prefix");
    expect(overflow.nativeErrorDetail() == "Values exceed permitted range",
           "Clipper diagnostic bytes must reach the adapter unchanged");
}

} // namespace
} // namespace Vp

int vpRunGeometryCoreTests()
{
    try
    {
        Vp::distanceAndStandardShapes();
        Vp::splineBoundariesAndApproximation();
        Vp::ellipseBoundariesAndApproximation();
        Vp::polygonMeasurements();
        Vp::polygonBooleanOperationsAndErrors();
        std::cout << "Pure geometry checks passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << exception.what() << '\n';
        return 1;
    }
}
