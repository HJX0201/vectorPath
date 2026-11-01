#pragma once

#include "vp_entity.h"

#include <vector>

namespace Vp
{

// 按矩形阵列生成图元副本。
std::vector<VpEntityRecord>
associativeRectangularArrayEntities(const std::vector<VpEntityRecord>& sources, VpArrayId array_id,
                                    double column_spacing, double row_spacing, int column_count,
                                    int row_count);
// 按环形阵列生成图元副本。
std::vector<VpEntityRecord>
associativePolarArrayEntities(const std::vector<VpEntityRecord>& sources, VpArrayId array_id,
                              const VpPoint2d& center, int item_count, double fill_angle);
// 按路径阵列生成图元副本。
std::vector<VpEntityRecord> associativePathArrayEntities(const std::vector<VpEntityRecord>& sources,
                                                         VpArrayId array_id,
                                                         const VpEntityRecord& path,
                                                         const VpPoint2d& source_base_point,
                                                         int item_count, bool align_to_path);
// 重新计算关联阵列成员的几何。
bool regenerateAssociativeArray(const std::vector<VpEntityRecord>& document_entities,
                                const VpAssociativeArrayData& parameters,
                                std::vector<VpEntityRecord>& results,
                                std::vector<VpEntityId>& existing_ids);

} // namespace Vp
