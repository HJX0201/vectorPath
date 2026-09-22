#include "vp_application_settings_migration.h"

#include <QSettings>

namespace Vp
{

int migrateMissingApplicationSettings(QSettings& legacy_settings, QSettings& current_settings)
{
    legacy_settings.sync();
    current_settings.sync();
    if (legacy_settings.status() != QSettings::NoError ||
        current_settings.status() != QSettings::NoError)
    {
        return -1;
    }

    int migrated_count = 0;
    const QStringList legacy_keys = legacy_settings.allKeys();
    for (const QString& key : legacy_keys)
    {
        if (current_settings.contains(key))
        {
            continue;
        }
        current_settings.setValue(key, legacy_settings.value(key));
        ++migrated_count;
    }
    current_settings.sync();
    if (current_settings.status() != QSettings::NoError)
    {
        return -1;
    }
    return migrated_count;
}

} // namespace Vp
