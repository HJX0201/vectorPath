#include "s_cad_main_window.h"
#include "s_cad_viewport.h"
#include "s_shortcut_dialog.h"
#include "s_theme_manager.h"
#include "s_vector_import_dialog.h"

#include <SARibbonBar.h>
#include <SARibbonCategory.h>
#include <SARibbonPanel.h>
#include <SARibbonQuickAccessBar.h>
#include <SARibbonSystemButtonBar.h>
#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QDoubleSpinBox>
#include <QImage>
#include <QMenu>
#include <QSettings>
#include <QTimer>
#include <QToolButton>
#include <QtTest>
#include <algorithm>

using namespace smartGraphics;

namespace
{

QRect visibleIconBounds(const QIcon& icon, const QSize& size)
{
    const QImage image = icon.pixmap(size).toImage().convertToFormat(QImage::Format_ARGB32);
    QRect bounds;
    for (int y = 0; y < image.height(); ++y)
    {
        for (int x = 0; x < image.width(); ++x)
        {
            if (qAlpha(image.pixel(x, y)) > 0)
            {
                bounds = bounds.united(QRect(x, y, 1, 1));
            }
        }
    }
    return bounds;
}

} // namespace

class SWindowControlTest final : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void showsWindowControlIcons();
    void switchesMaximizeAndRestoreIcon();
    void keepsOnlyWindowControlsInTitleBar();
    void removesDuplicateRibbonPanels();
    void movesManagementToolsIntoHelp();
    void opensShortcutEditorFromHelp();
    void enablesRibbonDropDownActions();
    void simulationRibbonOrderAndControlState();
    void exposesVectorImportActionsAndIcons();
    void fillsWithPolylinesByDefaultWithoutOverlappingControls();
    void remembersEntityDisplayOptionsImmediately();
};

void SWindowControlTest::initTestCase()
{
    Q_INIT_RESOURCE(s_gui_resources);
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, QDir::tempPath());
    QCoreApplication::setOrganizationName(QStringLiteral("smartCadTests"));
    QCoreApplication::setApplicationName(QStringLiteral("smartCadWindowControlTests"));
    QSettings().clear();
}

void SWindowControlTest::showsWindowControlIcons()
{
    SThemeManager theme_manager;
    QVERIFY(theme_manager.applyTheme(SThemeMode::Dark));
    SCadMainWindow window(theme_manager);
    window.showNormal();
    QCoreApplication::processEvents();

    SARibbonSystemButtonBar* button_bar = window.windowButtonBar();
    QVERIFY(button_bar);
    QVERIFY(button_bar->minimizeButton());
    QVERIFY(button_bar->maximizeButton());
    QVERIFY(button_bar->closeButton());
    QCOMPARE(button_bar->iconSize(), QSize(24, 24));
    QVERIFY(!button_bar->minimizeButton()->icon().isNull());
    QVERIFY(!button_bar->maximizeButton()->icon().isNull());
    QVERIFY(!button_bar->closeButton()->icon().isNull());
    const QRect close_icon_bounds =
        visibleIconBounds(button_bar->closeButton()->icon(), QSize(24, 24));
    QVERIFY(close_icon_bounds.width() >= 11);
    QVERIFY(close_icon_bounds.height() >= 11);
    QCOMPARE(button_bar->minimizeButton()->accessibleName(), QStringLiteral("最小化窗口"));
    QCOMPARE(button_bar->maximizeButton()->accessibleName(), QStringLiteral("最大化窗口"));
    QCOMPARE(button_bar->closeButton()->accessibleName(), QStringLiteral("关闭窗口"));
}

void SWindowControlTest::switchesMaximizeAndRestoreIcon()
{
    SThemeManager theme_manager;
    QVERIFY(theme_manager.applyTheme(SThemeMode::Dark));
    SCadMainWindow window(theme_manager);
    window.showNormal();
    QCoreApplication::processEvents();

    QAbstractButton* maximize_button = window.windowButtonBar()->maximizeButton();
    QVERIFY(maximize_button);
    const qint64 maximize_icon_key = maximize_button->icon().cacheKey();

    window.setWindowState(window.windowState() | Qt::WindowMaximized);
    QTRY_VERIFY(window.isMaximized());
    QTRY_COMPARE(maximize_button->accessibleName(), QStringLiteral("还原窗口"));
    QVERIFY(maximize_button->icon().cacheKey() != maximize_icon_key);
}

void SWindowControlTest::keepsOnlyWindowControlsInTitleBar()
{
    SThemeManager theme_manager;
    QVERIFY(theme_manager.applyTheme(SThemeMode::Dark));
    SCadMainWindow window(theme_manager);
    window.showNormal();
    QCoreApplication::processEvents();

    QVERIFY(window.ribbonBar()->quickAccessBar()->isHidden());
    QVERIFY(window.windowButtonBar()->minimizeButton());
    QVERIFY(window.windowButtonBar()->maximizeButton());
    QVERIFY(window.windowButtonBar()->closeButton());
}

void SWindowControlTest::removesDuplicateRibbonPanels()
{
    SThemeManager theme_manager;
    QVERIFY(theme_manager.applyTheme(SThemeMode::Dark));
    SCadMainWindow window(theme_manager);

    auto* draw_category =
        window.findChild<SARibbonCategory*>(QStringLiteral("smartDrawCategory"));
    auto* modify_category =
        window.findChild<SARibbonCategory*>(QStringLiteral("smartModifyCategory"));
    QVERIFY(draw_category);
    QVERIFY(modify_category);
    QVERIFY(!draw_category->panelByName(QStringLiteral("常用修改")));
    QVERIFY(!draw_category->panelByName(QStringLiteral("注释")));
    QVERIFY(!modify_category->panelByName(QStringLiteral("历史")));
    QVERIFY(modify_category->panelByName(QStringLiteral("布尔运算")));
    QVERIFY(!window.findChild<SARibbonCategory*>(QStringLiteral("smartLayerCategory")));
    QVERIFY(window.findChild<QAction*>(QStringLiteral("smartAction_boolean_union")));
    QVERIFY(window.findChild<QAction*>(QStringLiteral("smartAction_boolean_intersection")));
    QVERIFY(window.findChild<QAction*>(QStringLiteral("smartAction_boolean_difference")));
    QVERIFY(window.findChild<QAction*>(QStringLiteral("smartAction_boolean_xor")));
    QVERIFY(window.findChild<QAction*>(QStringLiteral("smartAction_boolean_complement")));
}

void SWindowControlTest::movesManagementToolsIntoHelp()
{
    SThemeManager theme_manager;
    QVERIFY(theme_manager.applyTheme(SThemeMode::Dark));
    SCadMainWindow window(theme_manager);

    auto* help_category =
        window.findChild<SARibbonCategory*>(QStringLiteral("smartHelpCategory"));
    QVERIFY(help_category);
    QVERIFY(help_category->panelByName(QStringLiteral("主题")));
    QVERIFY(help_category->panelByName(QStringLiteral("图形维护")));
    QVERIFY(help_category->panelByName(QStringLiteral("扩展")));
    QVERIFY(help_category->panelByName(QStringLiteral("帮助")));

    QAction* shortcut_action =
        window.findChild<QAction*>(QStringLiteral("smartModifyShortcutsAction"));
    QVERIFY(shortcut_action);
    QCOMPARE(shortcut_action->text(), QStringLiteral("修改快捷键"));
    QVERIFY(shortcut_action->isEnabled());
}

void SWindowControlTest::opensShortcutEditorFromHelp()
{
    SThemeManager theme_manager;
    QVERIFY(theme_manager.applyTheme(SThemeMode::Dark));
    SCadMainWindow window(theme_manager);
    QAction* shortcut_action =
        window.findChild<QAction*>(QStringLiteral("smartModifyShortcutsAction"));
    QVERIFY(shortcut_action);

    bool dialog_opened = false;
    QTimer::singleShot(0, [&]()
    {
        for (QWidget* widget : QApplication::topLevelWidgets())
        {
            auto* shortcut_dialog = dynamic_cast<SShortcutDialog*>(widget);
            if (!shortcut_dialog)
            {
                continue;
            }
            dialog_opened = true;
            QMetaObject::invokeMethod(shortcut_dialog, "accept", Qt::QueuedConnection);
            break;
        }
    });
    shortcut_action->trigger();

    QVERIFY(dialog_opened);
}

void SWindowControlTest::enablesRibbonDropDownActions()
{
    SThemeManager theme_manager;
    QVERIFY(theme_manager.applyTheme(SThemeMode::Dark));
    SCadMainWindow window(theme_manager);

    const QStringList action_names{
        QStringLiteral("smartAction_draw_circle"),
        QStringLiteral("smartAction_draw_arc"),
        QStringLiteral("smartGripAction"),
        QStringLiteral("smartBooleanAction"),
    };
    for (const QString& action_name : action_names)
    {
        QAction* action = window.findChild<QAction*>(action_name);
        QVERIFY2(action, qPrintable(action_name));
        QVERIFY2(action->menu(), qPrintable(action_name));
        bool found_button = false;
        for (QToolButton* button : window.findChildren<QToolButton*>())
        {
            if (button->defaultAction() != action)
            {
                continue;
            }
            found_button = true;
            QCOMPARE(button->popupMode(), QToolButton::MenuButtonPopup);
        }
        QVERIFY2(found_button, qPrintable(action_name));
    }

    QAction* triangle_action =
        window.findChild<QAction*>(QStringLiteral("smartAction_shape_triangle"));
    QVERIFY2(triangle_action, "Shape triangle action not found");
    triangle_action->trigger();

    SCadViewport* viewport = window.findChild<SCadViewport*>();
    QVERIFY(viewport);
    QCOMPARE(viewport->toolMode(), SToolMode::StandardShape);
    QCOMPARE(viewport->standardShapeType(), SStandardShapeType::Triangle);
}

void SWindowControlTest::simulationRibbonOrderAndControlState()
{
    SThemeManager theme_manager;
    QVERIFY(theme_manager.applyTheme(SThemeMode::Dark));
    SCadMainWindow window(theme_manager);
    const QList<SARibbonCategory*> categories = window.ribbonBar()->categoryPages();
    int annotation_index = -1;
    int simulation_index = -1;
    int view_index = -1;
    for (int index = 0; index < categories.size(); ++index)
    {
        const QString name = categories[index]->objectName();
        annotation_index = name == QLatin1String("smartAnnotationCategory") ? index
                                                                            : annotation_index;
        simulation_index = name == QLatin1String("smartSimulationCategory") ? index
                                                                             : simulation_index;
        view_index = name == QLatin1String("smartViewCategory") ? index : view_index;
    }
    QVERIFY(annotation_index >= 0);
    QCOMPARE(simulation_index, annotation_index + 1);
    QCOMPARE(view_index, simulation_index + 1);
    auto* simulation_category =
        window.findChild<SARibbonCategory*>(QStringLiteral("smartSimulationCategory"));
    QVERIFY(simulation_category->panelByName(QStringLiteral("仿真")));
    QVERIFY(simulation_category->panelByName(QStringLiteral("排序")));
    QAction* sort_action =
        window.findChild<QAction*>(QStringLiteral("smartSortApplyAction"));
    QVERIFY(sort_action);
    QVERIFY(sort_action->isEnabled());
    QVERIFY(!sort_action->icon().isNull());
}

void SWindowControlTest::exposesVectorImportActionsAndIcons()
{
    SThemeManager theme_manager;
    QVERIFY(theme_manager.applyTheme(SThemeMode::Dark));
    SCadMainWindow window(theme_manager);
    QAction* svg_action =
        window.findChild<QAction*>(QStringLiteral("smartAction_import_svg"));
    QAction* bitmap_action =
        window.findChild<QAction*>(QStringLiteral("smartAction_import_bitmap"));
    QAction* fill_action =
        window.findChild<QAction*>(QStringLiteral("smartAction_svg_fill"));
    QAction* deduplicate_action =
        window.findChild<QAction*>(QStringLiteral("smartAction_svg_deduplicate"));
    QVERIFY(svg_action);
    QVERIFY(bitmap_action);
    QVERIFY(fill_action);
    QVERIFY(deduplicate_action);
    QVERIFY(deduplicate_action->menu());
    QCOMPARE(deduplicate_action->menu()->actions().size(), 2);
    QVERIFY(!svg_action->icon().isNull());
    QVERIFY(!bitmap_action->icon().isNull());
    QVERIFY(!fill_action->icon().isNull());
    QVERIFY(!deduplicate_action->icon().isNull());
    QVERIFY(visibleIconBounds(svg_action->icon(), QSize(32, 32)).isValid());
    QVERIFY(visibleIconBounds(bitmap_action->icon(), QSize(32, 32)).isValid());
    QVERIFY(visibleIconBounds(fill_action->icon(), QSize(32, 32)).isValid());
    QVERIFY(visibleIconBounds(deduplicate_action->icon(), QSize(32, 32)).isValid());
}

void SWindowControlTest::fillsWithPolylinesByDefaultWithoutOverlappingControls()
{
    SSvgVectorData data;
    data.source_width = 10.0;
    data.source_height = 10.0;
    SVectorRegion region;
    region.contours = {
        {{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {0.0, 10.0}},
    };
    data.regions.push_back(std::move(region));

    SVectorImportDialog dialog(std::move(data));
    dialog.show();
    QCoreApplication::processEvents();
    QVERIFY(!dialog.importGeometry().entities.empty());
    QVERIFY(std::all_of(
        dialog.importGeometry().entities.begin(),
        dialog.importGeometry().entities.end(),
        [](const SColoredEntityGeometry& entity)
        {
            return entity.type == SEntityType::Polyline;
        }));
    const QList<QDoubleSpinBox*> spin_boxes =
        dialog.findChildren<QDoubleSpinBox*>();
    QCOMPARE(spin_boxes.size(), 3);
    QCOMPARE(std::count_if(
                 spin_boxes.begin(), spin_boxes.end(),
                 [](const QDoubleSpinBox* spin_box)
                 {
                     return spin_box->isVisible();
                 }),
             2);
    QCOMPARE(std::count_if(
                 spin_boxes.begin(), spin_boxes.end(),
                 [](const QDoubleSpinBox* spin_box)
                 {
                     return spin_box->isVisible() &&
                            spin_box->suffix().isEmpty();
                 }),
             1);
}

void SWindowControlTest::remembersEntityDisplayOptionsImmediately()
{
    QSettings settings;
    settings.remove(QStringLiteral("display/nodesVisible"));
    settings.remove(QStringLiteral("display/directionsVisible"));
    settings.remove(QStringLiteral("display/sequenceVisible"));

    SThemeManager theme_manager;
    QVERIFY(theme_manager.applyTheme(SThemeMode::Dark));
    SCadMainWindow window(theme_manager);
    SCadViewport* viewport = window.findChild<SCadViewport*>();
    QVERIFY(viewport);
    QVERIFY(!viewport->isNodeDisplayVisible());
    QVERIFY(!viewport->isDirectionDisplayVisible());
    QVERIFY(!viewport->isSequenceDisplayVisible());

    viewport->setNodeDisplayVisible(true);
    viewport->setDirectionDisplayVisible(true);
    viewport->setSequenceDisplayVisible(true);
    QSettings persisted_settings;
    persisted_settings.sync();
    QCOMPARE(persisted_settings.value(QStringLiteral("display/nodesVisible")).toBool(),
             true);
    QCOMPARE(
        persisted_settings.value(QStringLiteral("display/directionsVisible")).toBool(),
        true);
    QCOMPARE(persisted_settings.value(QStringLiteral("display/sequenceVisible")).toBool(),
             true);
    settings.remove(QStringLiteral("display/nodesVisible"));
    settings.remove(QStringLiteral("display/directionsVisible"));
    settings.remove(QStringLiteral("display/sequenceVisible"));
}

QTEST_MAIN(SWindowControlTest)

#include "s_window_control_test.moc"
