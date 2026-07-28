#pragma once

#include "s_geometry_types.h"

#include <QString>

namespace smartGraphics
{

class SCadDocument;

struct SCommandContext
{
    SCadDocument* document = nullptr;
    SPoint2d cursor_position;
};

class SICadCommand
{
  public:
    virtual ~SICadCommand() = default;
    virtual QString id() const = 0;
    virtual QString displayName() const = 0;
    virtual void begin(const SCommandContext& command_context) = 0;
    virtual void acceptPoint(const SPoint2d& world_point) = 0;
    virtual void cancel() = 0;
};

} // namespace smartGraphics
