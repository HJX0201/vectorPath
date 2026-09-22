#pragma once

#include <QtTest>

// Each suite stays in its own translation unit and retains its own generated moc.
#define VECTORPATH_TEST_ENTRY(test_class, entry_point)                                             \
    int entry_point(int argc, char* argv[])                                                        \
    {                                                                                              \
        test_class test;                                                                           \
        QTEST_SET_MAIN_SOURCE_PATH                                                                 \
        return QTest::qExec(&test, argc, argv);                                                    \
    }
