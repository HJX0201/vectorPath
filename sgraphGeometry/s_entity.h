#pragma once

#include "s_geometry_types.h"

#include <QColor>
#include <QString>
#include <array>
#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

namespace smartGraphics
{

using SEntityId = std::uint64_t;

enum class SEntityType : std::uint8_t
{
    Line = 1,
    Circle = 2,
    Arc = 3,
    Polyline = 4,
    Text = 5,
    LinearDimension = 6,
    Hatch = 7,
    Spline = 8,
    Ellipse = 9,
    MText = 10,
    Leader = 11
};

enum class STextHorizontalAlignment : std::uint8_t
{
    Left = 0,
    Center = 1,
    Right = 2,
    Justified = 3
};

enum class STextVerticalAlignment : std::uint8_t
{
    Top = 0,
    Middle = 1,
    Baseline = 2,
    Bottom = 3
};

enum class SDimensionType : std::uint8_t
{
    Linear = 0,
    Aligned = 1,
    Angular = 2,
    Radius = 3,
    Diameter = 4,
    ArcLength = 5,
    Ordinate = 6
};

struct SLineEntity
{
    SPoint2d start_point;
    SPoint2d end_point;
};

struct SCircleEntity
{
    SPoint2d center;
    double radius = 0.0;
};

struct SArcEntity
{
    SPoint2d center;
    double radius = 0.0;
    double start_angle = 0.0;
    double end_angle = 0.0;
    bool is_clockwise = false;
};

struct SPolylineEntity
{
    std::vector<SPoint2d> vertices;
    bool is_closed = false;
    std::vector<double> bulges;
    std::vector<double> start_widths;
    std::vector<double> end_widths;
};

struct STextEntity
{
    SPoint2d position;
    QString text;
    double height = 2.5;
    double rotation = 0.0;
    QString style_name = QStringLiteral("Standard");
    STextHorizontalAlignment horizontal_alignment = STextHorizontalAlignment::Left;
    STextVerticalAlignment vertical_alignment = STextVerticalAlignment::Baseline;
};

struct SMTextEntity
{
    SPoint2d position;
    QString rich_text;
    double width = 40.0;
    double height = 2.5;
    double rotation = 0.0;
    QString style_name = QStringLiteral("Standard");
    STextHorizontalAlignment horizontal_alignment = STextHorizontalAlignment::Left;
};

struct SLeaderEntity
{
    std::vector<SPoint2d> vertices;
    QString text;
    double text_height = 2.5;
    double arrow_size = 2.5;
    QString style_name = QStringLiteral("Standard");
};

struct SLinearDimensionEntity
{
    SPoint2d first_point;
    SPoint2d second_point;
    SPoint2d dimension_line_point;
    SPoint2d center_point;
    SDimensionType dimension_type = SDimensionType::Linear;
    QString style_name;
    QString text_override;
};

enum class SHatchFillType : std::uint8_t
{
    Solid = 0,
    Pattern = 1,
    Gradient = 2
};

struct SHatchEntity
{
    std::vector<SPoint2d> boundary;
    std::vector<std::vector<SPoint2d>> island_boundaries;
    SHatchFillType fill_type = SHatchFillType::Solid;
    QString pattern_name = QStringLiteral("ANSI31");
    double pattern_scale = 1.0;
    double pattern_angle = 45.0;
    QColor gradient_start{72, 142, 255};
    QColor gradient_end{19, 45, 77};
    SEntityId associative_boundary_id = 0;
};

struct SSplineEntity
{
    std::array<SPoint2d, 4> control_points;
};

struct SEllipseEntity
{
    SPoint2d center;
    SPoint2d major_axis;
    SPoint2d minor_axis;
};

using SEntityGeometry =
    std::variant<SLineEntity, SCircleEntity, SArcEntity, SPolylineEntity, STextEntity,
                 SLinearDimensionEntity, SHatchEntity, SSplineEntity, SEllipseEntity, SMTextEntity,
                 SLeaderEntity>;

using SArrayId = std::uint64_t;

enum class SArrayType : std::uint8_t
{
    Rectangular = 1,
    Polar = 2,
    Path = 3
};

struct SAssociativeArrayData
{
    SArrayId array_id = 0;
    SArrayType array_type = SArrayType::Rectangular;
    std::uint32_t source_index = 0;
    std::uint32_t item_index = 0;
    int column_count = 1;
    int row_count = 1;
    int item_count = 1;
    double column_spacing = 0.0;
    double row_spacing = 0.0;
    double fill_angle = 360.0;
    SPoint2d center;
    SPoint2d source_base_point;
    SEntityId path_entity_id = 0;
    bool align_to_path = true;
    SEntityType source_type = SEntityType::Line;
    SEntityGeometry source_geometry = SLineEntity{};
};

struct SEntityRecord
{
    SEntityId id = 0;
    SEntityType type = SEntityType::Line;
    QString layer_name = QStringLiteral("0");
    double line_width_mm = -1.0;
    SEntityGeometry geometry = SLineEntity{};
    std::optional<SAssociativeArrayData> associative_array;
    QString space_name;
};

} // namespace smartGraphics
