#pragma once

#include "s_entity.h"

#include <array>
#include <vector>

namespace smartGraphics
{

bool calculateThreePointArc(const SPoint2d& first_point, const SPoint2d& second_point,
                            const SPoint2d& third_point, SPoint2d& center, double& radius,
                            double& start_angle, double& end_angle) noexcept;
bool tangentCircleToTwoEntities(const SEntityRecord& first_source,
                                const SEntityRecord& second_source, const SPoint2d& first_pick,
                                const SPoint2d& second_pick, double radius, SCircleEntity& result);
bool tangentCircleToThreeLines(const std::array<SEntityRecord, 3>& sources,
                               const std::array<SPoint2d, 3>& picks, SCircleEntity& result);
bool tangentCircleToThreeEntities(const std::array<SEntityRecord, 3>& sources,
                                  const std::array<SPoint2d, 3>& picks, SCircleEntity& result);
bool arcFromStartEndRadius(const SPoint2d& start_point, const SPoint2d& end_point, double radius,
                           double side_sign, SArcEntity& result) noexcept;
bool arcFromStartEndDirection(const SPoint2d& start_point, const SPoint2d& end_point,
                              double direction_angle, SArcEntity& result) noexcept;
bool arcFromStartEndAngle(const SPoint2d& start_point, const SPoint2d& end_point,
                          double included_angle, SArcEntity& result) noexcept;
bool arcFromCenterStartAngle(const SPoint2d& center, const SPoint2d& start_point,
                             double included_angle, SArcEntity& result) noexcept;
bool bulgeArc(const SPoint2d& start_point, const SPoint2d& end_point, double bulge,
              SArcEntity& result) noexcept;
bool threePointBulge(const SPoint2d& start_point, const SPoint2d& point_on_arc,
                     const SPoint2d& end_point, double& bulge) noexcept;
std::vector<SPoint2d> polylineSegmentOutline(const SPolylineEntity& polyline,
                                             std::size_t segment_index, int arc_segment_count = 32);
double entityAngleDegrees(const SPoint2d& center, const SPoint2d& point) noexcept;
SEntityRecord translatedEntity(const SEntityRecord& source, double delta_x, double delta_y);
SEntityRecord rotatedEntity(const SEntityRecord& source, const SPoint2d& center, double angle);
SEntityRecord scaledEntity(const SEntityRecord& source, const SPoint2d& base_point,
                           double scale_factor);
SEntityRecord mirroredEntity(const SEntityRecord& source, const SPoint2d& axis_start,
                             const SPoint2d& axis_end);
SEntityRecord alignedEntity(const SEntityRecord& source, const SPoint2d& first_source_point,
                            const SPoint2d& first_target_point, const SPoint2d& second_source_point,
                            const SPoint2d& second_target_point, bool scale_to_fit);
bool offsetEntity(const SEntityRecord& source, const SPoint2d& through_point,
                  SEntityRecord& result);
bool trimmedLineEntity(const SEntityRecord& cutting_entity, const SEntityRecord& target_entity,
                       const SPoint2d& pick_point, SEntityRecord& result);
std::vector<SEntityRecord> trimmedEntityParts(const SEntityRecord& cutting_entity,
                                              const SEntityRecord& target_entity,
                                              const SPoint2d& pick_point);
bool extendedLineEntity(const SEntityRecord& boundary_entity, const SEntityRecord& target_entity,
                        SEntityRecord& result);
bool extendedEntity(const SEntityRecord& boundary_entity, const SEntityRecord& target_entity,
                    const SPoint2d& pick_point, SEntityRecord& result);
std::vector<SEntityRecord> brokenLineEntities(const SEntityRecord& source,
                                              const SPoint2d& first_break_point,
                                              const SPoint2d& second_break_point);
std::vector<SEntityRecord> brokenEntityParts(const SEntityRecord& source,
                                             const SPoint2d& first_break_point,
                                             const SPoint2d& second_break_point);
bool joinedLineEntity(const std::vector<SEntityRecord>& sources, double tolerance,
                      SEntityRecord& result);
bool joinedEntity(const std::vector<SEntityRecord>& sources, double tolerance,
                  SEntityRecord& result);
bool polylineClosedStateEntity(const SEntityRecord& source, bool is_closed, SEntityRecord& result);
bool polylineConstantWidthEntity(const SEntityRecord& source, double width, SEntityRecord& result);
bool reversedPolylineEntity(const SEntityRecord& source, SEntityRecord& result);
bool decurvedPolylineEntity(const SEntityRecord& source, SEntityRecord& result);
std::vector<SEntityRecord> explodedEntityParts(const SEntityRecord& source);
bool stretchedEntity(const SEntityRecord& source, const SPoint2d& first_corner,
                     const SPoint2d& second_corner, double delta_x, double delta_y,
                     SEntityRecord& result);
bool lengthenedEntity(const SEntityRecord& source, const SPoint2d& pick_point,
                      const SPoint2d& destination, SEntityRecord& result);
bool filletedLineEntities(const SEntityRecord& first_source, const SEntityRecord& second_source,
                          const SPoint2d& first_pick, const SPoint2d& second_pick, double radius,
                          SEntityRecord& first_result, SEntityRecord& second_result,
                          SEntityRecord& arc_result);
bool filletedEntities(const SEntityRecord& first_source, const SEntityRecord& second_source,
                      const SPoint2d& first_pick, const SPoint2d& second_pick, double radius,
                      SEntityRecord& first_result, SEntityRecord& second_result,
                      SEntityRecord& arc_result);
bool filletedPolylineEntity(const SEntityRecord& source, double radius, SEntityRecord& result);
bool chamferedLineEntities(const SEntityRecord& first_source, const SEntityRecord& second_source,
                           const SPoint2d& first_pick, const SPoint2d& second_pick,
                           double first_distance, double second_distance,
                           SEntityRecord& first_result, SEntityRecord& second_result,
                           SEntityRecord& chamfer_result);
bool chamferedEntities(const SEntityRecord& first_source, const SEntityRecord& second_source,
                       const SPoint2d& first_pick, const SPoint2d& second_pick,
                       double first_distance, double second_distance, SEntityRecord& first_result,
                       SEntityRecord& second_result, SEntityRecord& chamfer_result);
bool chamferedPolylineEntity(const SEntityRecord& source, double first_distance,
                             double second_distance, SEntityRecord& result);
bool blendedSplineEntity(const SEntityRecord& first_source, const SEntityRecord& second_source,
                         const SPoint2d& first_pick, const SPoint2d& second_pick,
                         SEntityRecord& result);
std::vector<SEntityRecord> rectangularArrayEntities(const std::vector<SEntityRecord>& sources,
                                                    double column_spacing, double row_spacing,
                                                    int column_count, int row_count);
std::vector<SEntityRecord> polarArrayEntities(const std::vector<SEntityRecord>& sources,
                                              const SPoint2d& center, int item_count,
                                              double fill_angle);
std::vector<SEntityRecord> pathArrayEntities(const std::vector<SEntityRecord>& sources,
                                             const SEntityRecord& path,
                                             const SPoint2d& source_base_point, int item_count,
                                             bool align_to_path);

} // namespace smartGraphics
