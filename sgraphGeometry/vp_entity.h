#pragma once

#include "vp_curve_entities.h"

#include <QColor>
#include <QString>
#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

namespace Vp
{

using VpEntityId = std::uint64_t;

enum class VpEntityType : std::uint8_t
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

enum class VpTextHorizontalAlignment : std::uint8_t
{
    Left = 0,
    Center = 1,
    Right = 2,
    Justified = 3
};

enum class VpTextVerticalAlignment : std::uint8_t
{
    Top = 0,
    Middle = 1,
    Baseline = 2,
    Bottom = 3
};

enum class VpDimensionType : std::uint8_t
{
    Linear = 0,
    Aligned = 1,
    Angular = 2,
    Radius = 3,
    Diameter = 4,
    ArcLength = 5,
    Ordinate = 6
};

struct VpTextEntity
{
    VpPoint2d position;
    QString text;
    double height = 2.5;
    double rotation = 0.0;
    QString style_name = QStringLiteral("Standard");
    VpTextHorizontalAlignment horizontal_alignment = VpTextHorizontalAlignment::Left;
    VpTextVerticalAlignment vertical_alignment = VpTextVerticalAlignment::Baseline;
};

struct VpMTextEntity
{
    VpPoint2d position;
    QString rich_text;
    double width = 40.0;
    double height = 2.5;
    double rotation = 0.0;
    QString style_name = QStringLiteral("Standard");
    VpTextHorizontalAlignment horizontal_alignment = VpTextHorizontalAlignment::Left;
};

struct VpLeaderEntity
{
    std::vector<VpPoint2d> vertices;
    QString text;
    double text_height = 2.5;
    double arrow_size = 2.5;
    QString style_name = QStringLiteral("Standard");
};

struct VpLinearDimensionEntity
{
    VpPoint2d first_point;
    VpPoint2d second_point;
    VpPoint2d dimension_line_point;
    VpPoint2d center_point;
    VpDimensionType dimension_type = VpDimensionType::Linear;
    QString style_name;
    QString text_override;
};

enum class VpHatchFillType : std::uint8_t
{
    Solid = 0,
    Pattern = 1,
    Gradient = 2
};

struct VpHatchEntity
{
    std::vector<VpPoint2d> boundary;
    std::vector<std::vector<VpPoint2d>> island_boundaries;
    VpHatchFillType fill_type = VpHatchFillType::Solid;
    QString pattern_name = QStringLiteral("ANSI31");
    double pattern_scale = 1.0;
    double pattern_angle = 45.0;
    QColor gradient_start{72, 142, 255};
    QColor gradient_end{19, 45, 77};
    VpEntityId associative_boundary_id = 0;
};

using VpEntityGeometry =
    std::variant<VpLineEntity, VpCircleEntity, VpArcEntity, VpPolylineEntity, VpTextEntity,
                 VpLinearDimensionEntity, VpHatchEntity, VpSplineEntity, VpEllipseEntity,
                 VpMTextEntity, VpLeaderEntity>;

using VpArrayId = std::uint64_t;

enum class VpArrayType : std::uint8_t
{
    Rectangular = 1,
    Polar = 2,
    Path = 3
};

struct VpAssociativeArrayData
{
    VpArrayId array_id = 0;
    VpArrayType array_type = VpArrayType::Rectangular;
    std::uint32_t source_index = 0;
    std::uint32_t item_index = 0;
    int column_count = 1;
    int row_count = 1;
    int item_count = 1;
    double column_spacing = 0.0;
    double row_spacing = 0.0;
    double fill_angle = 360.0;
    VpPoint2d center;
    VpPoint2d source_base_point;
    VpEntityId path_entity_id = 0;
    bool align_to_path = true;
    VpEntityType source_type = VpEntityType::Line;
    VpEntityGeometry source_geometry = VpLineEntity{};
};

struct VpEntityRecord
{
    VpEntityId id = 0;
    VpEntityType type = VpEntityType::Line;
    QString layer_name = QStringLiteral("0");
    double line_width_mm = -1.0;
    VpEntityGeometry geometry = VpLineEntity{};
    std::optional<VpAssociativeArrayData> associative_array;
    QString space_name;
};

} // namespace Vp
