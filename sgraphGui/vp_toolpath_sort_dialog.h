#pragma once

#include "vp_toolpath.h"

#include <QDialog>

class QCheckBox;

namespace Vp
{

// 设置刀具路径排序方式的对话框。
class VpToolpathSortDialog final : public QDialog
{
  public:
    // 构造排序设置对话框。
    explicit VpToolpathSortDialog(const VpToolpathSortOptions& initial_options,
                                  QWidget* parent = nullptr);

    // 返回对话框中选择的排序选项。
    VpToolpathSortOptions options() const;

  private:
    // 按模式启用或禁用相关控件。
    void updateControlStates();

    QCheckBox* m_row_scan_check = nullptr;
    QCheckBox* m_shortest_check = nullptr;
    QCheckBox* m_left_to_right_check = nullptr;
    QCheckBox* m_right_to_left_check = nullptr;
    QCheckBox* m_top_to_bottom_check = nullptr;
    QCheckBox* m_bottom_to_top_check = nullptr;
    QCheckBox* m_reverse_check = nullptr;
};

} // namespace Vp
