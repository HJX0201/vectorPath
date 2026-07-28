#include "s_cad_main_window.h"
#include "s_command_line_widget.h"
#include "s_dialog_service.h"
#include "s_plot_style_table.h"

#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSaveFile>
#include <QStandardPaths>
#include <QVBoxLayout>

namespace smartGraphics
{
namespace
{

QString plotStyleDirectoryPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
           QStringLiteral("/PlotStyles");
}

QStringList installedPlotStyleReferences()
{
    QStringList references;
    references.append(QStringLiteral("acad"));
    const QDir directory(plotStyleDirectoryPath());
    for (const QFileInfo& file_info :
         directory.entryInfoList({QStringLiteral("*.ctb"), QStringLiteral("*.stb")},
                                  QDir::Files | QDir::Readable, QDir::Name))
    {
        references.append(file_info.absoluteFilePath());
    }
    return references;
}

QString plotStyleDisplayName(const QString& reference)
{
    if (reference == QLatin1String("acad"))
    {
        return QObject::tr("acad（内置默认）");
    }
    return QFileInfo(reference).baseName();
}

void populatePlotStyleCombo(QComboBox& combo_box, const QString& current_reference)
{
    combo_box.clear();
    combo_box.addItem(QObject::tr("无"), QString());
    for (const QString& reference : installedPlotStyleReferences())
    {
        combo_box.addItem(plotStyleDisplayName(reference), reference);
    }
    const int current_index = combo_box.findData(current_reference);
    combo_box.setCurrentIndex(current_index < 0 ? 0 : current_index);
}

SResult<void> installPlotStyleFile(const QString& source_path)
{
    const SResult<SPlotStyleTable> validation = SPlotStyleTable::load(source_path);
    if (!validation)
    {
        return SResult<void>::failure(validation.errorMessage());
    }
    const QString directory_path = plotStyleDirectoryPath();
    if (!QDir().mkpath(directory_path))
    {
        return SResult<void>::failure(QObject::tr("无法创建打印样式目录。"));
    }
    QFile source_file(source_path);
    if (!source_file.open(QIODevice::ReadOnly))
    {
        return SResult<void>::failure(source_file.errorString());
    }
    QSaveFile target_file(QDir(directory_path).filePath(QFileInfo(source_path).fileName()));
    if (!target_file.open(QIODevice::WriteOnly) || target_file.write(source_file.readAll()) < 0 ||
        !target_file.commit())
    {
        return SResult<void>::failure(
            QObject::tr("无法安装打印样式表：%1").arg(target_file.errorString()));
    }
    return SResult<void>::success();
}

} // namespace

bool SCadMainWindow::executeOutputCommand(const QString& simplified_command)
{
    const QString command = simplified_command.toUpper();
    if (command == QLatin1String("PLOTSTYLE") || command == QLatin1String("STYLESMANAGER"))
    {
        showPlotStyleManager();
        return true;
    }
    return false;
}

void SCadMainWindow::showPlotStyleManager()
{
    SDialog dialog(this);
    dialog.setWindowTitle(tr("打印样式管理器"));
    dialog.resize(620, 420);
    auto* root_layout = new QVBoxLayout(&dialog);
    auto* list_widget = new QListWidget(&dialog);
    auto* details_label = new QLabel(&dialog);
    details_label->setWordWrap(true);
    details_label->setMinimumHeight(72);
    root_layout->addWidget(list_widget, 1);
    root_layout->addWidget(details_label);
    auto* button_row = new QHBoxLayout();
    auto* import_button = new QPushButton(tr("导入 CTB/STB…"), &dialog);
    auto* remove_button = new QPushButton(tr("删除"), &dialog);
    auto* close_button = new QPushButton(tr("关闭"), &dialog);
    button_row->addWidget(import_button);
    button_row->addWidget(remove_button);
    button_row->addStretch(1);
    button_row->addWidget(close_button);
    root_layout->addLayout(button_row);
    const auto refresh_list = [list_widget]()
    {
        list_widget->clear();
        for (const QString& reference : installedPlotStyleReferences())
        {
            auto* item = new QListWidgetItem(plotStyleDisplayName(reference), list_widget);
            item->setData(Qt::UserRole, reference);
            item->setData(Qt::UserRole + 1, QFileInfo(reference).isAbsolute());
        }
        if (list_widget->count() > 0)
        {
            list_widget->setCurrentRow(0);
        }
    };
    connect(list_widget, &QListWidget::currentItemChanged, &dialog,
            [details_label, remove_button](QListWidgetItem* current)
            {
                if (!current)
                {
                    details_label->clear();
                    remove_button->setEnabled(false);
                    return;
                }
                const QString reference = current->data(Qt::UserRole).toString();
                const SResult<SPlotStyleTable> result = SPlotStyleTable::fromReference(reference);
                remove_button->setEnabled(current->data(Qt::UserRole + 1).toBool());
                if (!result)
                {
                    details_label->setText(tr("读取失败：%1").arg(result.errorMessage()));
                    return;
                }
                const SPlotStyleTable& table = result.value();
                details_label->setText(
                    tr("类型：%1\n样式数量：%2\n说明：%3")
                        .arg(table.type() == SPlotStyleTableType::ColorDependent
                                 ? tr("颜色相关 CTB")
                                 : tr("命名样式 STB"))
                        .arg(table.styles().size())
                        .arg(table.description().isEmpty() ? tr("无") : table.description()));
            });
    connect(import_button, &QPushButton::clicked, &dialog,
            [this, &dialog, refresh_list]()
            {
                const QString file_path = QFileDialog::getOpenFileName(
                    &dialog, tr("导入打印样式表"), {}, tr("打印样式表 (*.ctb *.stb)"));
                if (file_path.isEmpty())
                {
                    return;
                }
                const SResult<void> result = installPlotStyleFile(file_path);
                if (!result)
                {
                    SDialogService::warning(&dialog, tr("打印样式管理器"),
                                            result.errorMessage());
                    return;
                }
                refresh_list();
                m_command_line->appendMessage(tr("打印样式已安装：%1").arg(file_path));
            });
    connect(remove_button, &QPushButton::clicked, &dialog,
            [list_widget, refresh_list, &dialog]()
            {
                QListWidgetItem* current = list_widget->currentItem();
                if (!current || !current->data(Qt::UserRole + 1).toBool())
                {
                    return;
                }
                const QString file_path = current->data(Qt::UserRole).toString();
                const QFileInfo file_info(file_path);
                if (file_info.absolutePath() == QDir(plotStyleDirectoryPath()).absolutePath() &&
                    QFile::remove(file_path))
                {
                    refresh_list();
                }
                else
                {
                    SDialogService::warning(&dialog, tr("打印样式管理器"),
                                            tr("无法删除所选打印样式表。"));
                }
            });
    connect(close_button, &QPushButton::clicked, &dialog, &QDialog::accept);
    refresh_list();
    dialog.exec();
}

} // namespace smartGraphics
