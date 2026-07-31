#pragma once

#include "s_bitmap_vectorizer.h"
#include "s_svg_vector_data.h"
#include "s_vector_fill.h"

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

namespace vectorPath
{

class SVectorImportDialog final : public QDialog
{
  public:
    explicit SVectorImportDialog(QByteArray svg_data, QWidget* parent = nullptr);
    explicit SVectorImportDialog(QImage bitmap, QWidget* parent = nullptr);
    explicit SVectorImportDialog(SSvgVectorData vector_data,
                                 QWidget* parent = nullptr);

    const SVectorImportGeometry& importGeometry() const;
    SVectorImportSettings vectorSettings() const;
    QByteArray generatedSvgData() const;

  private:
    void createInterface();
    void connectControls();
    void rebuildVectorData();
    void rebuildGeometry();
    void updatePreview();
    void chooseBackgroundColor();
    void useTopLeftBackground();
    SVectorImportSettings settings() const;

    QByteArray m_svg_data;
    QImage m_bitmap;
    SSvgVectorData m_vector_data;
    std::optional<SVectorImportGeometry> m_geometry;
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

} // namespace vectorPath
