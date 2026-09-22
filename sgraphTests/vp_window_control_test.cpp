#include "vp_cad_main_window.h"
#include "vp_cad_viewport.h"
#include "vp_icon_provider.h"
#include "vp_shortcut_dialog.h"
#include "vp_theme_manager.h"
#include "vp_vector_import_dialog.h"

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
#include <SARibbonBar.h>
#include <SARibbonCategory.h>
#include <SARibbonPanel.h>
#include <SARibbonQuickAccessBar.h>
#include <algorithm>

using namespace Vp;

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

class VpWindowControlTest final : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void usesNativeWindowFrame();
    void maximizesAndRestoresWithNativeFrame();
    void keepsRibbonBelowNativeTitleBar();
    void removesDuplicateRibbonPanels();
    void movesManagementToolsIntoHelp();
    void opensShortcutEditorFromHelp();
    void enablesRibbonDropDownActions();
    void simulationRibbonOrderAndControlState();
    void exposesVectorImportActionsAndIcons();
    void fillsWithPolylinesByDefaultWithoutOverlappingControls();
    void remembersEntityDisplayOptionsImmediately();
};

void VpWindowControlTest::initTestCase()
{
    Q_INIT_RESOURCE(vp_gui_resources);
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, QDir::tempPath());
    QCoreApplication::setOrganizationName(QStringLiteral("vectorPathTests"));
    QCoreApplication::setApplicationName(QStringLiteral("vectorPathWindowControlTests"));
    QSettings().clear();
}

void VpWindowControlTest::usesNativeWindowFrame()
{
    VpThemeManager theme_manager;
    QVERIFY(theme_manager.applyTheme(VpThemeMode::Dark));
    VpCadMainWindow window(theme_manager);
    window.showNormal();
    QCoreApplication::processEvents();

    const SARibbonMainWindowStyles styles = window.ribbonMainwindowStyle();
    QVERIFY(styles.testFlag(SARibbonMainWindowStyleFlag::UseNativeFrame));
    QVERIFY(styles.testFlag(SARibbonMainWindowStyleFlag::UseRibbonMenuBar));
    QVERIFY(!styles.testFlag(SARibbonMainWindowStyleFlag::UseRibbonFrame));
    QVERIFY(!(window.windowFlags() & Qt::FramelessWindowHint));
    QVERIFY(!window.windowButtonBar());
}

void VpWindowControlTest::maximizesAndRestoresWithNativeFrame()
{
    VpThemeManager theme_manager;
    QVERIFY(theme_manager.applyTheme(VpThemeMode::Dark));
    VpCadMainWindow window(theme_manager);
    window.showNormal();
    QCoreApplication::processEvents();

    window.showMaximized();
    QTRY_VERIFY(window.isMaximized());
    window.showNormal();
    QTRY_VERIFY(!window.isMaximized());
}

void VpWindowControlTest::keepsRibbonBelowNativeTitleBar()
{
    VpThemeManager theme_manager;
    QVERIFY(theme_manager.applyTheme(VpThemeMode::Dark));
    VpCadMainWindow window(theme_manager);
    window.showNormal();
    QCoreApplication::processEvents();

    QVERIFY(window.ribbonBar()->quickAccessBar()->isHidden());
    QVERIFY(!window.ribbonBar()->isTitleIconVisible());
    QVERIFY(!window.windowButtonBar());
}

void VpWindowControlTest::removesDuplicateRibbonPanels()
{
    VpThemeManager theme_manager;
    QVERIFY(theme_manager.applyTheme(VpThemeMode::Dark));
    VpCadMainWindow window(theme_manager);

    auto* draw_category = window.findChild<SARibbonCategory*>(QStringLiteral("smartDrawCategory"));
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

void VpWindowControlTest::movesManagementToolsIntoHelp()
{
    VpThemeManager theme_manager;
    QVERIFY(theme_manager.applyTheme(VpThemeMode::Dark));
    VpCadMainWindow window(theme_manager);

    auto* help_category = window.findChild<SARibbonCategory*>(QStringLiteral("smartHelpCategory"));
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

void VpWindowControlTest::opensShortcutEditorFromHelp()
{
    VpThemeManager theme_manager;
    QVERIFY(theme_manager.applyTheme(VpThemeMode::Dark));
    VpCadMainWindow window(theme_manager);
    QAction* shortcut_action =
        window.findChild<QAction*>(QStringLiteral("smartModifyShortcutsAction"));
    QVERIFY(shortcut_action);

    bool dialog_opened = false;
    QTimer::singleShot(0,
                       [&]()
                       {
                           for (QWidget* widget : QApplication::topLevelWidgets())
                           {
                               auto* shortcut_dialog = dynamic_cast<VpShortcutDialog*>(widget);
                               if (!shortcut_dialog)
                               {
                                   continue;
                               }
                               dialog_opened = true;
                               QMetaObject::invokeMethod(shortcut_dialog, "accept",
                                                         Qt::QueuedConnection);
                               break;
                           }
                       });
    shortcut_action->trigger();

    QVERIFY(dialog_opened);
}

void VpWindowControlTest::enablesRibbonDropDownActions()
{
    VpThemeManager theme_manager;
    QVERIFY(theme_manager.applyTheme(VpThemeMode::Dark));
    VpCadMainWindow window(theme_manager);

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

    VpCadViewport* viewport = window.findChild<VpCadViewport*>();
    QVERIFY(viewport);
    QCOMPARE(viewport->toolMode(), VpToolMode::StandardShape);
    QCOMPARE(viewport->standardShapeType(), VpStandardShapeType::Triangle);
}

void VpWindowControlTest::simulationRibbonOrderAndControlState()
{
    VpThemeManager theme_manager;
    QVERIFY(theme_manager.applyTheme(VpThemeMode::Dark));
    VpCadMainWindow window(theme_manager);
    const QList<SARibbonCategory*> categories = window.ribbonBar()->categoryPages();
    int annotation_index = -1;
    int simulation_index = -1;
    int view_index = -1;
    for (int index = 0; index < categories.size(); ++index)
    {
        const QString name = categories[index]->objectName();
        annotation_index =
            name == QLatin1String("smartAnnotationCategory") ? index : annotation_index;
        simulation_index =
            name == QLatin1String("smartSimulationCategory") ? index : simulation_index;
        view_index = name == QLatin1String("smartViewCategory") ? index : view_index;
    }
    QVERIFY(annotation_index >= 0);
    QCOMPARE(simulation_index, annotation_index + 1);
    QCOMPARE(view_index, simulation_index + 1);
    auto* simulation_category =
        window.findChild<SARibbonCategory*>(QStringLiteral("smartSimulationCategory"));
    QVERIFY(simulation_category->panelByName(QStringLiteral("仿真")));
    QVERIFY(simulation_category->panelByName(QStringLiteral("排序")));
    QAction* sort_action = window.findChild<QAction*>(QStringLiteral("smartSortApplyAction"));
    QVERIFY(sort_action);
    QVERIFY(sort_action->isEnabled());
    QVERIFY(!sort_action->icon().isNull());
}

void VpWindowControlTest::exposesVectorImportActionsAndIcons()
{
    VpThemeManager theme_manager;
    QVERIFY(theme_manager.applyTheme(VpThemeMode::Dark));
    VpCadMainWindow window(theme_manager);
    QAction* svg_action = window.findChild<QAction*>(QStringLiteral("smartAction_import_svg"));
    QAction* bitmap_action =
        window.findChild<QAction*>(QStringLiteral("smartAction_import_bitmap"));
    QAction* fill_action = window.findChild<QAction*>(QStringLiteral("smartAction_svg_fill"));
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

void VpWindowControlTest::fillsWithPolylinesByDefaultWithoutOverlappingControls()
{
    VpSvgVectorData data;
    data.source_width = 10.0;
    data.source_height = 10.0;
    VpVectorRegion region;
    region.contours = {
        {{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {0.0, 10.0}},
    };
    data.regions.push_back(std::move(region));

    VpVectorImportDialog dialog(std::move(data));
    dialog.show();
    QCoreApplication::processEvents();
    QVERIFY(!dialog.importGeometry().entities.empty());
    QVERIFY(std::all_of(dialog.importGeometry().entities.begin(),
                        dialog.importGeometry().entities.end(),
                        [](const VpColoredEntityGeometry& entity)
                        {
                            return entity.type == VpEntityType::Polyline;
                        }));
    const QList<QDoubleSpinBox*> spin_boxes = dialog.findChildren<QDoubleSpinBox*>();
    QCOMPARE(spin_boxes.size(), 3);
    QCOMPARE(std::count_if(spin_boxes.begin(), spin_boxes.end(),
                           [](const QDoubleSpinBox* spin_box)
                           {
                               return spin_box->isVisible();
                           }),
             2);
    QCOMPARE(std::count_if(spin_boxes.begin(), spin_boxes.end(),
                           [](const QDoubleSpinBox* spin_box)
                           {
                               return spin_box->isVisible() && spin_box->suffix().isEmpty();
                           }),
             1);
}

void VpWindowControlTest::remembersEntityDisplayOptionsImmediately()
{
    QSettings settings;
    settings.remove(QStringLiteral("display/nodesVisible"));
    settings.remove(QStringLiteral("display/directionsVisible"));
    settings.remove(QStringLiteral("display/sequenceVisible"));

    VpThemeManager theme_manager;
    QVERIFY(theme_manager.applyTheme(VpThemeMode::Dark));
    VpCadMainWindow window(theme_manager);
    VpCadViewport* viewport = window.findChild<VpCadViewport*>();
    QVERIFY(viewport);
    QVERIFY(!viewport->isNodeDisplayVisible());
    QVERIFY(!viewport->isDirectionDisplayVisible());
    QVERIFY(!viewport->isSequenceDisplayVisible());

    viewport->setNodeDisplayVisible(true);
    viewport->setDirectionDisplayVisible(true);
    viewport->setSequenceDisplayVisible(true);
    QSettings persisted_settings;
    persisted_settings.sync();
    QCOMPARE(persisted_settings.value(QStringLiteral("display/nodesVisible")).toBool(), true);
    QCOMPARE(persisted_settings.value(QStringLiteral("display/directionsVisible")).toBool(), true);
    QCOMPARE(persisted_settings.value(QStringLiteral("display/sequenceVisible")).toBool(), true);
    settings.remove(QStringLiteral("display/nodesVisible"));
    settings.remove(QStringLiteral("display/directionsVisible"));
    settings.remove(QStringLiteral("display/sequenceVisible"));
}

QTEST_MAIN(VpWindowControlTest)

#include "vp_window_control_test.moc"
