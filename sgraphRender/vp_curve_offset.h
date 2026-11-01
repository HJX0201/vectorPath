#pragma once

#include "vp_entity.h"

namespace Vp
{

// 按过点偏移曲线图元。
bool offsetCurveEntity(const VpEntityRecord& source, const VpPoint2d& through_point,
                       VpEntityRecord& result);

} // namespace Vp
