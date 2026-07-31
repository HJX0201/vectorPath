#pragma once

#include "s_entity.h"

#include <vector>

namespace smartCam
{

std::vector<SEntityRecord>
associativeRectangularArrayEntities(const std::vector<SEntityRecord>& sources, SArrayId array_id,
                                    double column_spacing, double row_spacing, int column_count,
                                    int row_count);
std::vector<SEntityRecord> associativePolarArrayEntities(const std::vector<SEntityRecord>& sources,
                                                         SArrayId array_id, const SPoint2d& center,
                                                         int item_count, double fill_angle);
std::vector<SEntityRecord> associativePathArrayEntities(const std::vector<SEntityRecord>& sources,
                                                        SArrayId array_id,
                                                        const SEntityRecord& path,
                                                        const SPoint2d& source_base_point,
                                                        int item_count, bool align_to_path);
bool regenerateAssociativeArray(const std::vector<SEntityRecord>& document_entities,
                                const SAssociativeArrayData& parameters,
                                std::vector<SEntityRecord>& results,
                                std::vector<SEntityId>& existing_ids);

} // namespace smartCam
