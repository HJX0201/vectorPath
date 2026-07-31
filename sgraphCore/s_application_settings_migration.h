#pragma once

class QSettings;

namespace vectorPath
{

int migrateMissingApplicationSettings(QSettings& legacy_settings, QSettings& current_settings);

} // namespace vectorPath
