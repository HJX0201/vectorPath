#pragma once

class QSettings;

namespace Vp
{

int migrateMissingApplicationSettings(QSettings& legacy_settings, QSettings& current_settings);

} // namespace Vp
