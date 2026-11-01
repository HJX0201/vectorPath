#pragma once

#include "vp_entity.h"

#include <array>
#include <vector>

namespace Vp
{

// 三点确定圆弧。
bool calculateThreePointArc(const VpPoint2d& first_point, const VpPoint2d& second_point,
                            const VpPoint2d& third_point, VpPoint2d& center, double& radius,
                            double& start_angle, double& end_angle) noexcept;
// 求与两个图元相切的圆。
bool tangentCircleToTwoEntities(const VpEntityRecord& first_source,
                                const VpEntityRecord& second_source, const VpPoint2d& first_pick,
                                const VpPoint2d& second_pick, double radius,
                                VpCircleEntity& result);
// 求与三条直线相切的圆。
bool tangentCircleToThreeLines(const std::array<VpEntityRecord, 3>& sources,
                               const std::array<VpPoint2d, 3>& picks, VpCircleEntity& result);
// 求与三个图元相切的圆。
bool tangentCircleToThreeEntities(const std::array<VpEntityRecord, 3>& sources,
                                  const std::array<VpPoint2d, 3>& picks, VpCircleEntity& result);
// 由起点、终点和半径确定圆弧。
bool arcFromStartEndRadius(const VpPoint2d& start_point, const VpPoint2d& end_point, double radius,
                           double side_sign, VpArcEntity& result) noexcept;
// 由起点、终点和方向确定圆弧。
bool arcFromStartEndDirection(const VpPoint2d& start_point, const VpPoint2d& end_point,
                              double direction_angle, VpArcEntity& result) noexcept;
// 由起点、终点和包含角确定圆弧。
bool arcFromStartEndAngle(const VpPoint2d& start_point, const VpPoint2d& end_point,
                          double included_angle, VpArcEntity& result) noexcept;
// 由圆心、起点和包含角确定圆弧。
bool arcFromCenterStartAngle(const VpPoint2d& center, const VpPoint2d& start_point,
                             double included_angle, VpArcEntity& result) noexcept;
// 由凸度值确定圆弧。
bool bulgeArc(const VpPoint2d& start_point, const VpPoint2d& end_point, double bulge,
              VpArcEntity& result) noexcept;
// 由三点和凸度确定圆弧。
bool threePointBulge(const VpPoint2d& start_point, const VpPoint2d& point_on_arc,
                     const VpPoint2d& end_point, double& bulge) noexcept;
// 计算多段线某一段的轮廓边界。
std::vector<VpPoint2d> polylineSegmentOutline(const VpPolylineEntity& polyline,
                                              std::size_t segment_index,
                                              int arc_segment_count = 32);
// 计算点到圆心的角度。
double entityAngleDegrees(const VpPoint2d& center, const VpPoint2d& point) noexcept;
// 平移图元。
VpEntityRecord translatedEntity(const VpEntityRecord& source, double delta_x, double delta_y);
// 绕基点旋转图元。
VpEntityRecord rotatedEntity(const VpEntityRecord& source, const VpPoint2d& center, double angle);
// 按基点缩放图元。
VpEntityRecord scaledEntity(const VpEntityRecord& source, const VpPoint2d& base_point,
                            double scale_factor);
// 按轴镜像图元。
VpEntityRecord mirroredEntity(const VpEntityRecord& source, const VpPoint2d& axis_start,
                              const VpPoint2d& axis_end);
// 把图元对齐到两点。
VpEntityRecord alignedEntity(const VpEntityRecord& source, const VpPoint2d& first_source_point,
                             const VpPoint2d& first_target_point,
                             const VpPoint2d& second_source_point,
                             const VpPoint2d& second_target_point, bool scale_to_fit);
// 按过点偏移图元。
bool offsetEntity(const VpEntityRecord& source, const VpPoint2d& through_point,
                  VpEntityRecord& result);
// 用剪切边裁剪直线。
bool trimmedLineEntity(const VpEntityRecord& cutting_entity, const VpEntityRecord& target_entity,
                       const VpPoint2d& pick_point, VpEntityRecord& result);
// 计算裁剪后保留的图元片段。
std::vector<VpEntityRecord> trimmedEntityParts(const VpEntityRecord& cutting_entity,
                                               const VpEntityRecord& target_entity,
                                               const VpPoint2d& pick_point);
// 把直线延长到边界。
bool extendedLineEntity(const VpEntityRecord& boundary_entity, const VpEntityRecord& target_entity,
                        VpEntityRecord& result);
// 把图元延长到边界。
bool extendedEntity(const VpEntityRecord& boundary_entity, const VpEntityRecord& target_entity,
                    const VpPoint2d& pick_point, VpEntityRecord& result);
// 在两点处打断直线。
std::vector<VpEntityRecord> brokenLineEntities(const VpEntityRecord& source,
                                               const VpPoint2d& first_break_point,
                                               const VpPoint2d& second_break_point);
// 计算打断后保留的图元片段。
std::vector<VpEntityRecord> brokenEntityParts(const VpEntityRecord& source,
                                              const VpPoint2d& first_break_point,
                                              const VpPoint2d& second_break_point);
// 合并共线直线。
bool joinedLineEntity(const std::vector<VpEntityRecord>& sources, double tolerance,
                      VpEntityRecord& result);
// 合并相接的同类图元。
bool joinedEntity(const std::vector<VpEntityRecord>& sources, double tolerance,
                  VpEntityRecord& result);
// 设置多段线的闭合状态。
bool polylineClosedStateEntity(const VpEntityRecord& source, bool is_closed,
                               VpEntityRecord& result);
// 设置多段线的统一线宽。
bool polylineConstantWidthEntity(const VpEntityRecord& source, double width,
                                 VpEntityRecord& result);
// 反转多段线方向。
bool reversedPolylineEntity(const VpEntityRecord& source, VpEntityRecord& result);
// 把带凸度的多段线转为折线。
bool decurvedPolylineEntity(const VpEntityRecord& source, VpEntityRecord& result);
// 分解复合图元。
std::vector<VpEntityRecord> explodedEntityParts(const VpEntityRecord& source);
// 用拉伸框拉伸图元。
bool stretchedEntity(const VpEntityRecord& source, const VpPoint2d& first_corner,
                     const VpPoint2d& second_corner, double delta_x, double delta_y,
                     VpEntityRecord& result);
// 在拾取点处加长图元。
bool lengthenedEntity(const VpEntityRecord& source, const VpPoint2d& pick_point,
                      const VpPoint2d& destination, VpEntityRecord& result);
// 在两条直线间倒圆角。
bool filletedLineEntities(const VpEntityRecord& first_source, const VpEntityRecord& second_source,
                          const VpPoint2d& first_pick, const VpPoint2d& second_pick, double radius,
                          VpEntityRecord& first_result, VpEntityRecord& second_result,
                          VpEntityRecord& arc_result);
// 在两个图元间倒圆角。
bool filletedEntities(const VpEntityRecord& first_source, const VpEntityRecord& second_source,
                      const VpPoint2d& first_pick, const VpPoint2d& second_pick, double radius,
                      VpEntityRecord& first_result, VpEntityRecord& second_result,
                      VpEntityRecord& arc_result);
// 对多段线整体倒圆角。
bool filletedPolylineEntity(const VpEntityRecord& source, double radius, VpEntityRecord& result);
// 在两条直线间倒角。
bool chamferedLineEntities(const VpEntityRecord& first_source, const VpEntityRecord& second_source,
                           const VpPoint2d& first_pick, const VpPoint2d& second_pick,
                           double first_distance, double second_distance,
                           VpEntityRecord& first_result, VpEntityRecord& second_result,
                           VpEntityRecord& chamfer_result);
// 在两个图元间倒角。
bool chamferedEntities(const VpEntityRecord& first_source, const VpEntityRecord& second_source,
                       const VpPoint2d& first_pick, const VpPoint2d& second_pick,
                       double first_distance, double second_distance, VpEntityRecord& first_result,
                       VpEntityRecord& second_result, VpEntityRecord& chamfer_result);
// 对多段线整体倒角。
bool chamferedPolylineEntity(const VpEntityRecord& source, double first_distance,
                             double second_distance, VpEntityRecord& result);
// 用样条桥接两个图元。
bool blendedSplineEntity(const VpEntityRecord& first_source, const VpEntityRecord& second_source,
                         const VpPoint2d& first_pick, const VpPoint2d& second_pick,
                         VpEntityRecord& result);
// 生成矩形阵列图元。
std::vector<VpEntityRecord> rectangularArrayEntities(const std::vector<VpEntityRecord>& sources,
                                                     double column_spacing, double row_spacing,
                                                     int column_count, int row_count);
// 生成环形阵列图元。
std::vector<VpEntityRecord> polarArrayEntities(const std::vector<VpEntityRecord>& sources,
                                               const VpPoint2d& center, int item_count,
                                               double fill_angle);
// 生成路径阵列图元。
std::vector<VpEntityRecord> pathArrayEntities(const std::vector<VpEntityRecord>& sources,
                                              const VpEntityRecord& path,
                                              const VpPoint2d& source_base_point, int item_count,
                                              bool align_to_path);

} // namespace Vp
