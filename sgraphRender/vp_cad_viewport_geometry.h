#pragma once

#include "vp_entity.h"

#include <array>
#include <vector>

namespace Vp
{

bool calculateThreePointArc(const VpPoint2d& first_point, const VpPoint2d& second_point,
                            const VpPoint2d& third_point, VpPoint2d& center, double& radius,
                            double& start_angle, double& end_angle) noexcept;
bool tangentCircleToTwoEntities(const VpEntityRecord& first_source,
                                const VpEntityRecord& second_source, const VpPoint2d& first_pick,
                                const VpPoint2d& second_pick, double radius,
                                VpCircleEntity& result);
bool tangentCircleToThreeLines(const std::array<VpEntityRecord, 3>& sources,
                               const std::array<VpPoint2d, 3>& picks, VpCircleEntity& result);
bool tangentCircleToThreeEntities(const std::array<VpEntityRecord, 3>& sources,
                                  const std::array<VpPoint2d, 3>& picks, VpCircleEntity& result);
bool arcFromStartEndRadius(const VpPoint2d& start_point, const VpPoint2d& end_point, double radius,
                           double side_sign, VpArcEntity& result) noexcept;
bool arcFromStartEndDirection(const VpPoint2d& start_point, const VpPoint2d& end_point,
                              double direction_angle, VpArcEntity& result) noexcept;
bool arcFromStartEndAngle(const VpPoint2d& start_point, const VpPoint2d& end_point,
                          double included_angle, VpArcEntity& result) noexcept;
bool arcFromCenterStartAngle(const VpPoint2d& center, const VpPoint2d& start_point,
                             double included_angle, VpArcEntity& result) noexcept;
bool bulgeArc(const VpPoint2d& start_point, const VpPoint2d& end_point, double bulge,
              VpArcEntity& result) noexcept;
bool threePointBulge(const VpPoint2d& start_point, const VpPoint2d& point_on_arc,
                     const VpPoint2d& end_point, double& bulge) noexcept;
std::vector<VpPoint2d> polylineSegmentOutline(const VpPolylineEntity& polyline,
                                              std::size_t segment_index,
                                              int arc_segment_count = 32);
double entityAngleDegrees(const VpPoint2d& center, const VpPoint2d& point) noexcept;
VpEntityRecord translatedEntity(const VpEntityRecord& source, double delta_x, double delta_y);
VpEntityRecord rotatedEntity(const VpEntityRecord& source, const VpPoint2d& center, double angle);
VpEntityRecord scaledEntity(const VpEntityRecord& source, const VpPoint2d& base_point,
                            double scale_factor);
VpEntityRecord mirroredEntity(const VpEntityRecord& source, const VpPoint2d& axis_start,
                              const VpPoint2d& axis_end);
VpEntityRecord alignedEntity(const VpEntityRecord& source, const VpPoint2d& first_source_point,
                             const VpPoint2d& first_target_point,
                             const VpPoint2d& second_source_point,
                             const VpPoint2d& second_target_point, bool scale_to_fit);
bool offsetEntity(const VpEntityRecord& source, const VpPoint2d& through_point,
                  VpEntityRecord& result);
bool trimmedLineEntity(const VpEntityRecord& cutting_entity, const VpEntityRecord& target_entity,
                       const VpPoint2d& pick_point, VpEntityRecord& result);
std::vector<VpEntityRecord> trimmedEntityParts(const VpEntityRecord& cutting_entity,
                                               const VpEntityRecord& target_entity,
                                               const VpPoint2d& pick_point);
bool extendedLineEntity(const VpEntityRecord& boundary_entity, const VpEntityRecord& target_entity,
                        VpEntityRecord& result);
bool extendedEntity(const VpEntityRecord& boundary_entity, const VpEntityRecord& target_entity,
                    const VpPoint2d& pick_point, VpEntityRecord& result);
std::vector<VpEntityRecord> brokenLineEntities(const VpEntityRecord& source,
                                               const VpPoint2d& first_break_point,
                                               const VpPoint2d& second_break_point);
std::vector<VpEntityRecord> brokenEntityParts(const VpEntityRecord& source,
                                              const VpPoint2d& first_break_point,
                                              const VpPoint2d& second_break_point);
bool joinedLineEntity(const std::vector<VpEntityRecord>& sources, double tolerance,
                      VpEntityRecord& result);
bool joinedEntity(const std::vector<VpEntityRecord>& sources, double tolerance,
                  VpEntityRecord& result);
bool polylineClosedStateEntity(const VpEntityRecord& source, bool is_closed,
                               VpEntityRecord& result);
bool polylineConstantWidthEntity(const VpEntityRecord& source, double width,
                                 VpEntityRecord& result);
bool reversedPolylineEntity(const VpEntityRecord& source, VpEntityRecord& result);
bool decurvedPolylineEntity(const VpEntityRecord& source, VpEntityRecord& result);
std::vector<VpEntityRecord> explodedEntityParts(const VpEntityRecord& source);
bool stretchedEntity(const VpEntityRecord& source, const VpPoint2d& first_corner,
                     const VpPoint2d& second_corner, double delta_x, double delta_y,
                     VpEntityRecord& result);
bool lengthenedEntity(const VpEntityRecord& source, const VpPoint2d& pick_point,
                      const VpPoint2d& destination, VpEntityRecord& result);
bool filletedLineEntities(const VpEntityRecord& first_source, const VpEntityRecord& second_source,
                          const VpPoint2d& first_pick, const VpPoint2d& second_pick, double radius,
                          VpEntityRecord& first_result, VpEntityRecord& second_result,
                          VpEntityRecord& arc_result);
bool filletedEntities(const VpEntityRecord& first_source, const VpEntityRecord& second_source,
                      const VpPoint2d& first_pick, const VpPoint2d& second_pick, double radius,
                      VpEntityRecord& first_result, VpEntityRecord& second_result,
                      VpEntityRecord& arc_result);
bool filletedPolylineEntity(const VpEntityRecord& source, double radius, VpEntityRecord& result);
bool chamferedLineEntities(const VpEntityRecord& first_source, const VpEntityRecord& second_source,
                           const VpPoint2d& first_pick, const VpPoint2d& second_pick,
                           double first_distance, double second_distance,
                           VpEntityRecord& first_result, VpEntityRecord& second_result,
                           VpEntityRecord& chamfer_result);
bool chamferedEntities(const VpEntityRecord& first_source, const VpEntityRecord& second_source,
                       const VpPoint2d& first_pick, const VpPoint2d& second_pick,
                       double first_distance, double second_distance, VpEntityRecord& first_result,
                       VpEntityRecord& second_result, VpEntityRecord& chamfer_result);
bool chamferedPolylineEntity(const VpEntityRecord& source, double first_distance,
                             double second_distance, VpEntityRecord& result);
bool blendedSplineEntity(const VpEntityRecord& first_source, const VpEntityRecord& second_source,
                         const VpPoint2d& first_pick, const VpPoint2d& second_pick,
                         VpEntityRecord& result);
std::vector<VpEntityRecord> rectangularArrayEntities(const std::vector<VpEntityRecord>& sources,
                                                     double column_spacing, double row_spacing,
                                                     int column_count, int row_count);
std::vector<VpEntityRecord> polarArrayEntities(const std::vector<VpEntityRecord>& sources,
                                               const VpPoint2d& center, int item_count,
                                               double fill_angle);
std::vector<VpEntityRecord> pathArrayEntities(const std::vector<VpEntityRecord>& sources,
                                              const VpEntityRecord& path,
                                              const VpPoint2d& source_base_point, int item_count,
                                              bool align_to_path);

} // namespace Vp
