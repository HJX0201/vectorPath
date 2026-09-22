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

class VpVectorImportDialog final : public QDialog
{
  public:
    explicit VpVectorImportDialog(QByteArray svg_data, QWidget* parent = nullptr);
    explicit VpVectorImportDialog(QImage bitmap, QWidget* parent = nullptr);
    explicit VpVectorImportDialog(VpSvgVectorData vector_data, QWidget* parent = nullptr);

    const VpVectorImportGeometry& importGeometry() const;
    VpVectorImportSettings vectorSettings() const;
    QByteArray generatedSvgData() const;

  private:
    void createInterface();
    void connectControls();
    void rebuildVectorData();
    void rebuildGeometry();
    void updatePreview();
    void chooseBackgroundColor();
    void useTopLeftBackground();
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
