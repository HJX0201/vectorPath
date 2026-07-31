#pragma once

class QSettings;

namespace smartCam
{

int migrateMissingApplicationSettings(QSettings& legacy_settings, QSettings& current_settings);

} // namespace smartCam
