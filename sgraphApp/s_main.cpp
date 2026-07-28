#include "s_cad_main_window.h"
#include "s_chinese_ui_translator.h"
#include "s_theme_manager.h"

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
    Q_INIT_RESOURCE(s_gui_resources);
    QCoreApplication::setOrganizationName(QStringLiteral("smartCadLearning"));
    QCoreApplication::setApplicationName(QStringLiteral("smartGraphics"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    QTranslator qt_translator;
    qt_translator.load(QStringLiteral("qt_zh_CN"),
                       QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    application.installTranslator(&qt_translator);
    smartGraphics::SChineseUiTranslator ui_translator;
    application.installTranslator(&ui_translator);

    smartGraphics::SThemeManager theme_manager;
    theme_manager.applyTheme(smartGraphics::SThemeMode::Dark);

    smartGraphics::SCadMainWindow main_window(theme_manager);
    main_window.showMaximized();
    return application.exec();
}
