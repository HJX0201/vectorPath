#include "s_cad_main_window.h"
#include "s_shortcut_dialog.h"
#include "s_shortcut_manager.h"
#include "s_theme_manager.h"

#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QKeySequenceEdit>
#include <QSettings>
#include <QTimer>
#include <QTreeWidget>
#include <QtTest>

using namespace smartGraphics;

class SShortcutManagerTest final : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void cleanup();
    void persistsAndRestoresDefaults();
    void rejectsShortcutConflicts();
    void exposesModernShortcutDialog();
};

void SShortcutManagerTest::initTestCase()
{
    Q_INIT_RESOURCE(s_gui_resources);
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, QDir::tempPath());
    QCoreApplication::setOrganizationName(QStringLiteral("smartCadTests"));
    QCoreApplication::setApplicationName(QStringLiteral("smartCadShortcutManagerTests"));
    QSettings().clear();
}

void SShortcutManagerTest::cleanup()
{
    QSettings().clear();
}

void SShortcutManagerTest::persistsAndRestoresDefaults()
{
    QAction first_action(QStringLiteral("直线"), this);
    QAction duplicate_action(QStringLiteral("直线工具栏"), this);
    SShortcutManager manager;
    const QKeySequence default_shortcut(QStringLiteral("Ctrl+Alt+L"));
    const QKeySequence custom_shortcut(QStringLiteral("Ctrl+Shift+L"));
    manager.registerAction(&first_action, QStringLiteral("draw.line"), default_shortcut);
    manager.registerAction(&duplicate_action, QStringLiteral("draw.line"), default_shortcut);
    QCOMPARE(first_action.shortcut(), default_shortcut);
    QVERIFY(duplicate_action.shortcut().isEmpty());

    QVERIFY(manager.setShortcut(QStringLiteral("draw.line"), custom_shortcut));
    QCOMPARE(first_action.shortcut(), custom_shortcut);
    QVERIFY(duplicate_action.shortcut().isEmpty());

    QAction restored_action(QStringLiteral("直线"), this);
    SShortcutManager restored_manager;
    restored_manager.registerAction(&restored_action, QStringLiteral("draw.line"),
                                    default_shortcut);
    QCOMPARE(restored_action.shortcut(), custom_shortcut);
    restored_manager.restoreDefaults();
    QCOMPARE(restored_action.shortcut(), default_shortcut);
    QVERIFY(!QSettings().contains(QStringLiteral("shortcuts/draw.line")));
}

void SShortcutManagerTest::rejectsShortcutConflicts()
{
    QAction first_action(QStringLiteral("打开"), this);
    QAction second_action(QStringLiteral("保存"), this);
    SShortcutManager manager;
    manager.registerAction(&first_action, QStringLiteral("file.open"),
                           QKeySequence(QStringLiteral("Ctrl+O")));
    manager.registerAction(&second_action, QStringLiteral("file.save"),
                           QKeySequence(QStringLiteral("Ctrl+S")));

    QString conflicting_id;
    QVERIFY(!manager.setShortcut(QStringLiteral("file.save"),
                                 QKeySequence(QStringLiteral("Ctrl+O")), &conflicting_id));
    QCOMPARE(conflicting_id, QStringLiteral("file.open"));
    QCOMPARE(second_action.shortcut(), QKeySequence(QStringLiteral("Ctrl+S")));
    QVERIFY(manager.clearShortcut(QStringLiteral("file.open")));
    QVERIFY(manager.setShortcut(QStringLiteral("file.save"),
                                QKeySequence(QStringLiteral("Ctrl+O"))));
}

void SShortcutManagerTest::exposesModernShortcutDialog()
{
    SThemeManager theme_manager;
    QVERIFY(theme_manager.applyTheme(SThemeMode::Dark));
    SCadMainWindow window(theme_manager);

    QAction* shortcut_action = nullptr;
    for (QAction* action : window.findChildren<QAction*>())
    {
        if (action->property("smartShortcutId").toString() == QLatin1String("ui.shortcuts"))
        {
            shortcut_action = action;
            break;
        }
    }
    QVERIFY(shortcut_action);

    bool dialog_verified = false;
    QTimer::singleShot(
        0, &window,
        [&dialog_verified]()
        {
            for (QWidget* widget : QApplication::topLevelWidgets())
            {
                auto* dialog = dynamic_cast<SShortcutDialog*>(widget);
                if (!dialog)
                {
                    continue;
                }
                QTreeWidget* tree =
                    dialog->findChild<QTreeWidget*>(QStringLiteral("smartShortcutTree"));
                QKeySequenceEdit* editor = dialog->findChild<QKeySequenceEdit*>(
                    QStringLiteral("smartShortcutSequenceEdit"));
                dialog_verified = tree && tree->topLevelItemCount() >= 60 && editor &&
                                  editor->isEnabled();
                dialog->accept();
                return;
            }
        });
    shortcut_action->trigger();
    QVERIFY(dialog_verified);

    QAction* new_action = nullptr;
    for (QAction* action : window.findChildren<QAction*>())
    {
        if (action->property("smartShortcutId").toString() == QLatin1String("file.new"))
        {
            new_action = action;
            break;
        }
    }
    QVERIFY(new_action);
    QCOMPARE(new_action->shortcut(), QKeySequence::New);
}

QTEST_MAIN(SShortcutManagerTest)

#include "s_shortcut_manager_test.moc"
