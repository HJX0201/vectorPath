#pragma once

#include "vp_toolpath.h"

#include <QDialog>

class QCheckBox;

namespace Vp
{

class VpToolpathSortDialog final : public QDialog
{
  public:
    explicit VpToolpathSortDialog(const VpToolpathSortOptions& initial_options,
                                  QWidget* parent = nullptr);

    VpToolpathSortOptions options() const;

  private:
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
