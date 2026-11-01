#pragma once

class QSettings;

namespace Vp
{

// 把旧版设置迁移到当前设置，返回迁移项数。
int migrateMissingApplicationSettings(QSettings& legacy_settings, QSettings& current_settings);

} // namespace Vp
