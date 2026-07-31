#include "s_dialog_service.h"

#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#include <QtTest>

using namespace vectorPath;

class SDialogSizePolicyTest final : public QObject
{
    Q_OBJECT

  private slots:
    void givesCustomDialogsAUsableMinimum();
    void givesInputDialogsAUsableMinimum();
    void givesMessageDialogsAUsableMinimum();
    void preservesLargerExplicitMinimum();
};

void SDialogSizePolicyTest::givesCustomDialogsAUsableMinimum()
{
    SDialog dialog;

    QCOMPARE(dialog.minimumWidth(), 520);
    QCOMPARE(dialog.minimumHeight(), 320);
}

void SDialogSizePolicyTest::givesInputDialogsAUsableMinimum()
{
    bool dialog_found = false;
    int minimum_width = 0;
    int minimum_height = 0;
    QTimer::singleShot(0, [&]()
    {
        for (QWidget* widget : QApplication::topLevelWidgets())
        {
            auto* input_dialog = qobject_cast<QDialog*>(widget);
            if (!input_dialog || input_dialog->windowTitle() != QStringLiteral("创建文字"))
            {
                continue;
            }
            dialog_found = true;
            minimum_width = input_dialog->minimumWidth();
            minimum_height = input_dialog->minimumHeight();
            auto* text_edit =
                input_dialog->findChild<QLineEdit*>(QStringLiteral("s_dialog_text_input"));
            QVERIFY(text_edit);
            text_edit->setText(QStringLiteral("测试文字"));
            QMetaObject::invokeMethod(input_dialog, "accept", Qt::QueuedConnection);
            break;
        }
    });

    bool is_accepted = false;
    const QString text =
        SDialogService::getText(nullptr, QStringLiteral("创建文字"),
                                QStringLiteral("文字内容："), QLineEdit::Normal,
                                QString{}, &is_accepted);

    QVERIFY(dialog_found);
    QCOMPARE(minimum_width, 520);
    QCOMPARE(minimum_height, 220);
    QVERIFY(is_accepted);
    QCOMPARE(text, QStringLiteral("测试文字"));
}

void SDialogSizePolicyTest::givesMessageDialogsAUsableMinimum()
{
    bool dialog_found = false;
    int minimum_width = 0;
    int minimum_height = 0;
    QTimer::singleShot(0, [&]()
    {
        for (QWidget* widget : QApplication::topLevelWidgets())
        {
            auto* message_box = qobject_cast<QDialog*>(widget);
            if (!message_box || message_box->windowTitle() != QStringLiteral("提示"))
            {
                continue;
            }
            dialog_found = true;
            minimum_width = message_box->minimumWidth();
            minimum_height = message_box->minimumHeight();
            auto* button_box = message_box->findChild<QDialogButtonBox*>();
            QVERIFY(button_box);
            QPushButton* ok_button = button_box->button(QDialogButtonBox::Ok);
            QVERIFY(ok_button);
            QMetaObject::invokeMethod(ok_button, "click", Qt::QueuedConnection);
            break;
        }
    });

    const QMessageBox::StandardButton result =
        SDialogService::information(nullptr, QStringLiteral("提示"),
                                    QStringLiteral("操作已经完成。"));

    QVERIFY(dialog_found);
    QCOMPARE(minimum_width, 520);
    QCOMPARE(minimum_height, 240);
    QCOMPARE(result, QMessageBox::Ok);
}

void SDialogSizePolicyTest::preservesLargerExplicitMinimum()
{
    SDialog dialog;
    dialog.setMinimumSize(640, 420);

    QCOMPARE(dialog.minimumWidth(), 640);
    QCOMPARE(dialog.minimumHeight(), 420);
}

QTEST_MAIN(SDialogSizePolicyTest)

#include "s_dialog_size_policy_test.moc"
