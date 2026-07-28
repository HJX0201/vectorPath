#include "s_cad_main_window.h"
#include "s_cad_viewport.h"
#include "s_command_line_widget.h"
#include "s_theme_manager.h"

#include <DockWidget.h>
#include <SARibbonToolButton.h>
#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QSplitter>
#include <QToolBar>
#include <QtTest>

using namespace smartGraphics;

class SWorkspacePersistenceTest final : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void cleanup();
    void supportsResizableFixedPanels();
    void usesReferenceUiProportions();
};

void SWorkspacePersistenceTest::initTestCase()
{
    Q_INIT_RESOURCE(s_gui_resources);
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, QDir::tempPath());
    QCoreApplication::setOrganizationName(QStringLiteral("smartCadTests"));
    QCoreApplication::setApplicationName(QStringLiteral("smartCadWorkspacePersistenceTests"));
    QSettings().clear();
}

void SWorkspacePersistenceTest::cleanup()
{
    QSettings().clear();
}

void SWorkspacePersistenceTest::supportsResizableFixedPanels()
{
    SThemeManager theme_manager;
    QVERIFY(theme_manager.applyTheme(SThemeMode::Dark));
    SCadMainWindow window(theme_manager);
    window.resize(1400, 900);
    window.show();
    QCoreApplication::processEvents();

    const QList<QSplitter*> splitters = window.findChildren<QSplitter*>();
    QVERIFY(!splitters.isEmpty());
    for (QSplitter* splitter : splitters)
    {
        QCOMPARE(splitter->childrenCollapsible(), false);
        QVERIFY(splitter->handleWidth() >= 5);
        for (int handle_index = 1; handle_index < splitter->count(); ++handle_index)
        {
            QVERIFY(splitter->handle(handle_index)->isEnabled());
        }
    }

    SCommandLineWidget* command_line = window.findChild<SCommandLineWidget*>();
    QVERIFY(command_line);
    QCOMPARE(command_line->maximumHeight(), QWIDGETSIZE_MAX);
    QVERIFY(window.close());
    QVERIFY(!QSettings().value(QStringLiteral("workspace/dockState")).toByteArray().isEmpty());
}

void SWorkspacePersistenceTest::usesReferenceUiProportions()
{
    SThemeManager theme_manager;
    QVERIFY(theme_manager.applyTheme(SThemeMode::Dark));
    SCadMainWindow window(theme_manager);
    window.resize(1600, 900);
    window.show();
    QCoreApplication::processEvents();

    QToolBar* drawing_toolbar = window.findChild<QToolBar*>(QStringLiteral("smartDrawingToolBar"));
    QVERIFY(drawing_toolbar);
    QCOMPARE(drawing_toolbar->width(), 64);

    ads::CDockWidget* layer_dock =
        window.findChild<ads::CDockWidget*>(QStringLiteral("smartLayerDock"));
    ads::CDockWidget* property_dock =
        window.findChild<ads::CDockWidget*>(QStringLiteral("smartPropertiesDock"));
    ads::CDockWidget* command_dock =
        window.findChild<ads::CDockWidget*>(QStringLiteral("smartCommandDock"));
    QVERIFY(layer_dock);
    QVERIFY(property_dock);
    QVERIFY(command_dock);
    property_dock->toggleView(true);
    QCoreApplication::processEvents();
    QTRY_VERIFY(layer_dock->width() >= 280);
    const double inspector_ratio = static_cast<double>(layer_dock->width()) / window.width();
    QVERIFY(inspector_ratio >= 0.17 && inspector_ratio <= 0.23);
    const double property_ratio = static_cast<double>(property_dock->height()) /
                                  (layer_dock->height() + property_dock->height());
    const QString property_ratio_message =
        QStringLiteral("property ratio=%1, property=%2, layer=%3")
            .arg(property_ratio)
            .arg(property_dock->height())
            .arg(layer_dock->height());
    QVERIFY2(property_ratio >= 0.38 && property_ratio <= 0.62,
             qPrintable(property_ratio_message));
    const double command_ratio = static_cast<double>(command_dock->height()) / window.height();
    const QString command_ratio_message =
        QStringLiteral("command ratio=%1, dock=%2, window=%3")
            .arg(command_ratio)
            .arg(command_dock->height())
            .arg(window.height());
    QVERIFY2(command_ratio >= 0.10 && command_ratio <= 0.15,
             qPrintable(command_ratio_message));

    int large_button_count = 0;
    for (SARibbonToolButton* button : window.findChildren<SARibbonToolButton*>())
    {
        if (button->buttonType() == SARibbonToolButton::LargeButton)
        {
            ++large_button_count;
        }
    }
    QVERIFY(large_button_count >= 60);
}

QTEST_MAIN(SWorkspacePersistenceTest)

#include "s_workspace_persistence_test.moc"
