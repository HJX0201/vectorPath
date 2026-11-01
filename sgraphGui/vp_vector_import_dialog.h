#pragma once

#include "vp_bitmap_vectorizer.h"
#include "vp_svg_vector_data.h"
#include "vp_vector_fill.h"

#include <QByteArray>
#include <QDialog>
#include <QImage>
#include <optional>

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;

namespace Vp
{

// 导入前预览并设置矢量填充方式的对话框。
class VpVectorImportDialog final : public QDialog
{
  public:
    // 用 SVG 数据构造导入对话框。
    explicit VpVectorImportDialog(QByteArray svg_data, QWidget* parent = nullptr);
    // 用位图构造导入对话框。
    explicit VpVectorImportDialog(QImage bitmap, QWidget* parent = nullptr);
    // 用已解析的矢量数据构造导入对话框。
    explicit VpVectorImportDialog(VpSvgVectorData vector_data, QWidget* parent = nullptr);

    // 返回生成的导入几何。
    const VpVectorImportGeometry& importGeometry() const;
    // 返回当前的矢量导入设置。
    VpVectorImportSettings vectorSettings() const;
    // 返回生成的 SVG 数据。
    QByteArray generatedSvgData() const;

  private:
    // 创建界面控件。
    void createInterface();
    // 连接控件信号。
    void connectControls();
    // 按当前设置重建矢量数据。
    void rebuildVectorData();
    // 按当前设置重建导入几何。
    void rebuildGeometry();
    // 刷新预览。
    void updatePreview();
    // 选择位图背景色。
    void chooseBackgroundColor();
    // 用左上角像素作为背景色。
    void useTopLeftBackground();
    // 汇总界面上的导入设置。
    VpVectorImportSettings settings() const;

    QByteArray m_svg_data;
    QImage m_bitmap;
    VpSvgVectorData m_vector_data;
    std::optional<VpVectorImportGeometry> m_geometry;
    QComboBox* m_fill_mode = nullptr;
    QDoubleSpinBox* m_spacing = nullptr;
    QDoubleSpinBox* m_angle = nullptr;
    QDoubleSpinBox* m_scale = nullptr;
    QCheckBox* m_preserve_outlines = nullptr;
    QCheckBox* m_ignore_background = nullptr;
    QPushButton* m_background_color = nullptr;
    QPushButton* m_top_left_color = nullptr;
    QLabel* m_preview = nullptr;
    QLabel* m_summary = nullptr;
    QDialogButtonBox* m_buttons = nullptr;
    QColor m_selected_background;
    bool m_is_fill_only = false;
};

} // namespace Vp
