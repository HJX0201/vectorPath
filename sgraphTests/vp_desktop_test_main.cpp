#include "vp_test_runner.h"

#include <QApplication>
#include <QCoreApplication>
#include <QGuiApplication>
#include <cstring>
#include <iostream>
#include <vector>

QT_BEGIN_NAMESPACE
QTEST_ADD_GPU_BLACKLIST_SUPPORT_DEFS
QT_END_NAMESPACE

#define VECTORPATH_DESKTOP_TEST(suite_name, entry_point, application_mode)                         \
    int entry_point(int argc, char* argv[]);
#include "vp_desktop_test_registry.inc"
#undef VECTORPATH_DESKTOP_TEST

namespace Vp
{
namespace
{

enum class VpTestApplicationMode
{
    AppLess,
    Core,
    Gui,
    Widgets
};

struct VpDesktopTestSuite
{
    const char* name;
    int (*run)(int argc, char* argv[]);
    VpTestApplicationMode application_mode;
};

const VpDesktopTestSuite kSuites[] = {
#define VECTORPATH_DESKTOP_TEST(suite_name, entry_point, application_mode)                         \
    {#suite_name, &entry_point, VpTestApplicationMode::application_mode},
#include "vp_desktop_test_registry.inc"
#undef VECTORPATH_DESKTOP_TEST
};

void initializeApplication(QCoreApplication& application, const VpDesktopTestSuite& suite)
{
    application.setAttribute(Qt::AA_Use96Dpi, true);
    // Preserve the default settings identity from the former per-suite executable.
    application.setApplicationName(QString::fromLatin1(suite.name));
}

int runSuite(const VpDesktopTestSuite& suite, int argc, char* argv[])
{
    switch (suite.application_mode)
    {
    case VpTestApplicationMode::AppLess:
        return suite.run(argc, argv);
    case VpTestApplicationMode::Core:
    {
        QCoreApplication application(argc, argv);
        initializeApplication(application, suite);
        return suite.run(argc, argv);
    }
    case VpTestApplicationMode::Gui:
    {
        QGuiApplication application(argc, argv);
        initializeApplication(application, suite);
        QTEST_ADD_GPU_BLACKLIST_SUPPORT
        return suite.run(argc, argv);
    }
    case VpTestApplicationMode::Widgets:
    {
        QApplication application(argc, argv);
        initializeApplication(application, suite);
        QTEST_DISABLE_KEYPAD_NAVIGATION
        QTEST_ADD_GPU_BLACKLIST_SUPPORT
        return suite.run(argc, argv);
    }
    }
    return 2;
}

void listSuites()
{
    for (const VpDesktopTestSuite& suite : kSuites)
    {
        std::cout << suite.name << '\n';
    }
}

} // namespace
} // namespace Vp

int main(int argc, char* argv[])
{
    if (argc == 2 && std::strcmp(argv[1], "--list-suites") == 0)
    {
        Vp::listSuites();
        return 0;
    }
    if (argc < 2)
    {
        std::cerr << "Usage: vectorPathDesktopTests <suite> [QtTest arguments]\n"
                     "       vectorPathDesktopTests --list-suites\n";
        return 2;
    }
    for (const Vp::VpDesktopTestSuite& suite : Vp::kSuites)
    {
        if (std::strcmp(argv[1], suite.name) != 0)
        {
            continue;
        }
        // Qt may rewrite argc/argv. The suite name replaces the old executable name;
        // all subsequent QtTest options and test-function selectors pass through.
        std::vector<char*> arguments(argv + 1, argv + argc);
        arguments.push_back(nullptr);
        return Vp::runSuite(suite, argc - 1, arguments.data());
    }
    std::cerr << "Unknown test suite: " << argv[1] << '\n';
    return 2;
}
