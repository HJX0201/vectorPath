#pragma once

#include "s_result.h"

#include <QPainterPath>
#include <QString>
#include <vector>

namespace vectorPath
{

struct SSvgSubpath
{
    QPainterPath path;
    bool is_closed = false;
};

SResult<std::vector<SSvgSubpath>> parseSvgPathData(const QString& path_data);

} // namespace vectorPath
