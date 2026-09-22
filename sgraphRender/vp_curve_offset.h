#pragma once

#include "vp_entity.h"

namespace Vp
{

bool offsetCurveEntity(const VpEntityRecord& source, const VpPoint2d& through_point,
                       VpEntityRecord& result);

} // namespace Vp
