#include "s_toolpath_sort_dialog.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace smartCam
{

SToolpathSortDialog::SToolpathSortDialog(const SToolpathSortOptions& initial_options,
                                         QWidget* parent)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("smartToolpathSortDialog"));
    setWindowTitle(tr("刀路排序"));
    setMinimumSize(420, 260);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);
    auto* description = new QLabel(
        tr("设置当前选择集或当前空间内可加工实体的排序方式。实体将按图层依次处理。"),
        this);
    description->setWordWrap(true);
    layout->addWidget(description);

    auto* options_layout = new QGridLayout();
    options_layout->setHorizontalSpacing(18);
    options_layout->setVerticalSpacing(10);
    m_row_scan_check = new QCheckBox(tr("二维逐行扫描"), this);
    m_shortest_check = new QCheckBox(tr("始终最短"), this);
    m_left_to_right_check = new QCheckBox(tr("从左到右"), this);
    m_right_to_left_check = new QCheckBox(tr("从右到左"), this);
    m_top_to_bottom_check = new QCheckBox(tr("从上到下"), this);
    m_bottom_to_top_check = new QCheckBox(tr("从下到上"), this);
    m_row_scan_check->setObjectName(QStringLiteral("smartSortRowScanCheck"));
    m_shortest_check->setObjectName(QStringLiteral("smartSortShortestCheck"));
    m_left_to_right_check->setObjectName(QStringLiteral("smartSortLeftToRightCheck"));
    m_right_to_left_check->setObjectName(QStringLiteral("smartSortRightToLeftCheck"));
    m_top_to_bottom_check->setObjectName(QStringLiteral("smartSortTopToBottomCheck"));
    m_bottom_to_top_check->setObjectName(QStringLiteral("smartSortBottomToTopCheck"));
    m_reverse_check = new QCheckBox(tr("允许改变实体方向"), this);
    m_reverse_check->setObjectName(QStringLiteral("smartSortReverseCheck"));
    auto* method_group = new QButtonGroup(this);
    method_group->setExclusive(true);
    method_group->addButton(m_row_scan_check);
    method_group->addButton(m_shortest_check);
    auto* horizontal_group = new QButtonGroup(this);
    horizontal_group->setExclusive(true);
    horizontal_group->addButton(m_left_to_right_check);
    horizontal_group->addButton(m_right_to_left_check);
    auto* vertical_group = new QButtonGroup(this);
    vertical_group->setExclusive(true);
    vertical_group->addButton(m_top_to_bottom_check);
    vertical_group->addButton(m_bottom_to_top_check);
    options_layout->addWidget(new QLabel(tr("排序方式："), this), 0, 0);
    options_layout->addWidget(m_row_scan_check, 0, 1);
    options_layout->addWidget(m_shortest_check, 0, 2);
    options_layout->addWidget(new QLabel(tr("水平方向："), this), 1, 0);
    options_layout->addWidget(m_left_to_right_check, 1, 1);
    options_layout->addWidget(m_right_to_left_check, 1, 2);
    options_layout->addWidget(new QLabel(tr("垂直方向："), this), 2, 0);
    options_layout->addWidget(m_top_to_bottom_check, 2, 1);
    options_layout->addWidget(m_bottom_to_top_check, 2, 2);
    options_layout->addWidget(m_reverse_check, 3, 1, 1, 2);
    layout->addLayout(options_layout);

    m_row_scan_check->setChecked(initial_options.mode == SToolpathSortMode::RowScan);
    m_shortest_check->setChecked(initial_options.mode == SToolpathSortMode::Shortest);
    m_left_to_right_check->setChecked(
        initial_options.horizontal == SToolpathHorizontalDirection::LeftToRight);
    m_right_to_left_check->setChecked(
        initial_options.horizontal == SToolpathHorizontalDirection::RightToLeft);
    m_top_to_bottom_check->setChecked(
        initial_options.vertical == SToolpathVerticalDirection::TopToBottom);
    m_bottom_to_top_check->setChecked(
        initial_options.vertical == SToolpathVerticalDirection::BottomToTop);
    m_reverse_check->setChecked(initial_options.allow_reverse);

    auto* button_box = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    QPushButton* sort_button = button_box->addButton(tr("排序"), QDialogButtonBox::AcceptRole);
    sort_button->setObjectName(QStringLiteral("smartSortDialogApply"));
    sort_button->setDefault(true);
    layout->addWidget(button_box);
    connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_shortest_check, &QCheckBox::toggled, this,
            [this]()
            {
                updateControlStates();
            });
    updateControlStates();
}

SToolpathSortOptions SToolpathSortDialog::options() const
{
    SToolpathSortOptions result;
    result.mode =
        m_shortest_check->isChecked() ? SToolpathSortMode::Shortest : SToolpathSortMode::RowScan;
    result.horizontal = m_right_to_left_check->isChecked()
                            ? SToolpathHorizontalDirection::RightToLeft
                            : SToolpathHorizontalDirection::LeftToRight;
    result.vertical = m_bottom_to_top_check->isChecked()
                          ? SToolpathVerticalDirection::BottomToTop
                          : SToolpathVerticalDirection::TopToBottom;
    result.allow_reverse = result.mode == SToolpathSortMode::Shortest &&
                           m_reverse_check->isChecked();
    return result;
}

void SToolpathSortDialog::updateControlStates()
{
    const bool is_shortest = m_shortest_check->isChecked();
    m_left_to_right_check->setEnabled(!is_shortest);
    m_right_to_left_check->setEnabled(!is_shortest);
    m_top_to_bottom_check->setEnabled(!is_shortest);
    m_bottom_to_top_check->setEnabled(!is_shortest);
    m_reverse_check->setEnabled(is_shortest);
    if (!is_shortest)
    {
        m_reverse_check->setChecked(false);
    }
}

} // namespace smartCam
