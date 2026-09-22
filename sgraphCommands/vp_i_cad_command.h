#pragma once

#include "vp_geometry_types.h"

#include <QString>

namespace Vp
{

class VpCadDocument;

struct VpCommandContext
{
    VpCadDocument* document = nullptr;
    VpPoint2d cursor_position;
};

class VpICadCommand
{
  public:
    virtual ~VpICadCommand() = default;
    virtual QString id() const = 0;
    virtual QString displayName() const = 0;
    virtual void begin(const VpCommandContext& command_context) = 0;
    virtual void acceptPoint(const VpPoint2d& world_point) = 0;
    virtual void cancel() = 0;
};

} // namespace Vp
