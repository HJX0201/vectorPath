#include "vp_application_settings_migration.h"
#include "vp_cad_main_window.h"
#include "vp_chinese_ui_translator.h"
#include "vp_theme_manager.h"

#include <QApplication>
#include <QCoreApplication>
#include <QLibraryInfo>
#include <QResource>
#include <QSettings>
#include <QSurfaceFormat>
#include <QTranslator>

int main(int argument_count, char* argument_values[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    QSurfaceFormat surface_format;
    surface_format.setVersion(3, 3);
    surface_format.setProfile(QSurfaceFormat::CoreProfile);
    surface_format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(surface_format);

    QApplication application(argument_count, argument_values);
    Q_INIT_RESOURCE(vp_gui_resources);
    QSettings smart_cam_settings(QSettings::NativeFormat, QSettings::UserScope,
                                 QStringLiteral("smartCamLearning"), QStringLiteral("smartCam"));
    QSettings smart_cad_settings(QSettings::NativeFormat, QSettings::UserScope,
                                 QStringLiteral("smartCadLearning"),
                                 QStringLiteral("smartGraphics"));
    QSettings current_settings(QSettings::NativeFormat, QSettings::UserScope,
                               QStringLiteral("vectorPathLearning"), QStringLiteral("vectorPath"));
    Vp::migrateMissingApplicationSettings(smart_cam_settings, current_settings);
    Vp::migrateMissingApplicationSettings(smart_cad_settings, current_settings);
    QCoreApplication::setOrganizationName(QStringLiteral("vectorPathLearning"));
    QCoreApplication::setApplicationName(QStringLiteral("vectorPath"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.2.0-alpha.1"));

    QTranslator qt_translator;
    qt_translator.load(QStringLiteral("qt_zh_CN"),
                       QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    application.installTranslator(&qt_translator);
    Vp::VpChineseUiTranslator ui_translator;
    application.installTranslator(&ui_translator);

    Vp::VpThemeManager theme_manager;
    theme_manager.applyTheme(Vp::VpThemeMode::Dark);

    Vp::VpCadMainWindow main_window(theme_manager);
    main_window.showMaximized();
    return application.exec();
}
