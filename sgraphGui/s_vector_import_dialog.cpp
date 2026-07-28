#include "s_vector_import_dialog.h"

#include "s_svg_parser.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <variant>

namespace smartGraphics
{
namespace
{

QRectF entityBounds(const SColoredEntityGeometry& entity)
{
    if (entity.type == SEntityType::Line)
    {
        const SLineEntity& line = std::get<SLineEntity>(entity.geometry);
        return QRectF(line.start_point.toPointF(), line.end_point.toPointF()).normalized();
    }
    if (entity.type == SEntityType::Polyline)
    {
        const SPolylineEntity& polyline = std::get<SPolylineEntity>(entity.geometry);
        if (polyline.vertices.empty())
        {
            return {};
        }
        QRectF bounds(polyline.vertices.front().toPointF(), QSizeF());
        for (const SPoint2d& point : polyline.vertices)
        {
            bounds |= QRectF(point.toPointF(), QSizeF());
        }
        return bounds;
    }
    if (entity.type == SEntityType::Hatch)
    {
        const SHatchEntity& hatch = std::get<SHatchEntity>(entity.geometry);
        if (hatch.boundary.empty())
        {
            return {};
        }
        QRectF bounds(hatch.boundary.front().toPointF(), QSizeF());
        for (const SPoint2d& point : hatch.boundary)
        {
            bounds |= QRectF(point.toPointF(), QSizeF());
        }
        return bounds;
    }
    return {};
}

QRectF geometryBounds(const SVectorImportGeometry& geometry)
{
    QRectF result;
    bool has_bounds = false;
    for (const SColoredEntityGeometry& entity : geometry.entities)
    {
        const QRectF bounds = entityBounds(entity);
        if (!bounds.isValid() && bounds.isNull())
        {
            continue;
        }
        result = has_bounds ? result.united(bounds) : bounds;
        has_bounds = true;
    }
    return result;
}

void drawEntity(QPainter& painter, const SColoredEntityGeometry& entity,
                const QRectF& world_bounds, const QRectF& target)
{
    const double scale = std::min(target.width() / std::max(world_bounds.width(), 1.0e-6),
                                  target.height() / std::max(world_bounds.height(), 1.0e-6));
    const auto map_point = [&](const SPoint2d& point)
    {
        return QPointF(target.center().x() +
                           (point.x - world_bounds.center().x()) * scale,
                       target.center().y() -
                           (point.y - world_bounds.center().y()) * scale);
    };
    QColor color = entity.color;
    color.setAlpha(std::max(72, color.alpha()));
    painter.setPen(QPen(color, entity.type == SEntityType::Line ? 1.0 : 1.4));
    if (entity.type == SEntityType::Line)
    {
        const SLineEntity& line = std::get<SLineEntity>(entity.geometry);
        painter.drawLine(map_point(line.start_point), map_point(line.end_point));
    }
    else if (entity.type == SEntityType::Polyline)
    {
        const SPolylineEntity& polyline = std::get<SPolylineEntity>(entity.geometry);
        QPolygonF polygon;
        for (const SPoint2d& point : polyline.vertices)
        {
            polygon.append(map_point(point));
        }
        painter.drawPolyline(polygon);
        if (polyline.is_closed && polygon.size() > 2)
        {
            painter.drawLine(polygon.back(), polygon.front());
        }
    }
    else if (entity.type == SEntityType::Hatch)
    {
        const SHatchEntity& hatch = std::get<SHatchEntity>(entity.geometry);
        QPainterPath path;
        path.setFillRule(Qt::OddEvenFill);
        const auto add_loop = [&](const std::vector<SPoint2d>& loop)
        {
            if (loop.empty())
            {
                return;
            }
            path.moveTo(map_point(loop.front()));
            for (std::size_t index = 1; index < loop.size(); ++index)
            {
                path.lineTo(map_point(loop[index]));
            }
            path.closeSubpath();
        };
        add_loop(hatch.boundary);
        for (const std::vector<SPoint2d>& island : hatch.island_boundaries)
        {
            add_loop(island);
        }
        QColor fill = color;
        fill.setAlpha(std::min(fill.alpha(), 190));
        painter.fillPath(path, fill);
        painter.drawPath(path);
    }
}

} // namespace

SVectorImportDialog::SVectorImportDialog(QByteArray svg_data, QWidget* parent)
    : QDialog(parent), m_svg_data(std::move(svg_data))
{
    setWindowTitle(tr("SVG 矢量导入设置"));
    createInterface();
    connectControls();
    rebuildVectorData();
}

SVectorImportDialog::SVectorImportDialog(QImage bitmap, QWidget* parent)
    : QDialog(parent), m_bitmap(std::move(bitmap))
{
    setWindowTitle(tr("位图矢量化与 SVG 导入设置"));
    if (!m_bitmap.isNull())
    {
        m_selected_background = QColor::fromRgba(m_bitmap.pixel(0, 0));
    }
    createInterface();
    connectControls();
    rebuildVectorData();
}

SVectorImportDialog::SVectorImportDialog(SSvgVectorData vector_data, QWidget* parent)
    : QDialog(parent), m_vector_data(std::move(vector_data)), m_is_fill_only(true)
{
    setWindowTitle(tr("SVG 填充设置"));
    createInterface();
    connectControls();
    rebuildGeometry();
}

const SVectorImportGeometry& SVectorImportDialog::importGeometry() const
{
    return *m_geometry;
}

SVectorImportSettings SVectorImportDialog::vectorSettings() const
{
    return settings();
}

QByteArray SVectorImportDialog::generatedSvgData() const
{
    return m_svg_data;
}

void SVectorImportDialog::createInterface()
{
    resize(820, 620);
    auto* root_layout = new QVBoxLayout(this);
    auto* settings_group =
        new QGroupBox(m_is_fill_only ? tr("SVG 填充") : tr("SVG 导入"), this);
    auto* form = new QFormLayout(settings_group);
    m_fill_mode = new QComboBox(settings_group);
    m_fill_mode->addItem(tr("单线填充"));
    m_fill_mode->addItem(tr("多边形偏移线填充"));
    m_fill_mode->setVisible(m_is_fill_only);
    if (m_is_fill_only)
    {
        form->addRow(tr("填充方式："), m_fill_mode);
    }
    else
    {
        m_fill_mode->addItem(tr("不填充"));
        m_fill_mode->setCurrentIndex(2);
    }
    m_spacing = new QDoubleSpinBox(settings_group);
    m_spacing->setRange(0.001, 1000000.0);
    m_spacing->setDecimals(3);
    m_spacing->setValue(1.0);
    m_spacing->setVisible(m_is_fill_only);
    if (m_is_fill_only)
    {
        form->addRow(tr("填充间距："), m_spacing);
    }
    m_angle = new QDoubleSpinBox(settings_group);
    m_angle->setRange(-360.0, 360.0);
    m_angle->setDecimals(2);
    m_angle->setSuffix(QStringLiteral("°"));
    m_angle->setVisible(m_is_fill_only);
    if (m_is_fill_only)
    {
        form->addRow(tr("单线角度："), m_angle);
    }
    m_scale = new QDoubleSpinBox(settings_group);
    m_scale->setRange(0.000001, 1000000.0);
    m_scale->setDecimals(6);
    m_scale->setValue(1.0);
    m_scale->setVisible(!m_is_fill_only);
    if (!m_is_fill_only)
    {
        form->addRow(tr("导入比例："), m_scale);
    }
    m_preserve_outlines = new QCheckBox(tr("保留原始轮廓"), settings_group);
    m_preserve_outlines->setChecked(!m_is_fill_only);
    m_preserve_outlines->setVisible(!m_is_fill_only);
    if (!m_is_fill_only)
    {
        form->addRow({}, m_preserve_outlines);
    }
    root_layout->addWidget(settings_group);

    if (!m_bitmap.isNull())
    {
        auto* bitmap_group = new QGroupBox(tr("位图背景"), this);
        auto* background_layout = new QHBoxLayout(bitmap_group);
        m_ignore_background = new QCheckBox(tr("忽略指定背景色"), bitmap_group);
        m_ignore_background->setChecked(true);
        m_background_color = new QPushButton(tr("选择背景色"), bitmap_group);
        m_top_left_color = new QPushButton(tr("使用左上角颜色"), bitmap_group);
        background_layout->addWidget(m_ignore_background);
        background_layout->addWidget(m_background_color);
        background_layout->addWidget(m_top_left_color);
        background_layout->addStretch();
        root_layout->addWidget(bitmap_group);
    }

    m_preview = new QLabel(this);
    m_preview->setMinimumSize(640, 300);
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setFrameShape(QFrame::StyledPanel);
    root_layout->addWidget(m_preview, 1);
    m_summary = new QLabel(this);
    m_summary->setWordWrap(true);
    root_layout->addWidget(m_summary);
    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root_layout->addWidget(m_buttons);
}

void SVectorImportDialog::connectControls()
{
    const auto rebuild = [this]()
    {
        rebuildGeometry();
    };
    connect(m_fill_mode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, rebuild);
    connect(m_spacing, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, rebuild);
    connect(m_angle, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, rebuild);
    connect(m_scale, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, rebuild);
    connect(m_preserve_outlines, &QCheckBox::toggled, this, rebuild);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    if (m_ignore_background)
    {
        connect(m_ignore_background, &QCheckBox::toggled, this,
                &SVectorImportDialog::rebuildVectorData);
        connect(m_background_color, &QPushButton::clicked, this,
                &SVectorImportDialog::chooseBackgroundColor);
        connect(m_top_left_color, &QPushButton::clicked, this,
                &SVectorImportDialog::useTopLeftBackground);
    }
}

void SVectorImportDialog::rebuildVectorData()
{
    if (!m_bitmap.isNull())
    {
        SBitmapVectorSettings bitmap_settings;
        bitmap_settings.ignore_background = m_ignore_background->isChecked();
        bitmap_settings.background_color = m_selected_background;
        const SResult<QByteArray> vector_result =
            bitmapToSvgData(m_bitmap, bitmap_settings);
        if (!vector_result)
        {
            m_geometry.reset();
            m_summary->setText(vector_result.errorMessage());
            m_buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
            return;
        }
        m_svg_data = vector_result.value();
    }
    const SResult<SSvgVectorData> parse_result = parseSvgVectorData(m_svg_data);
    if (!parse_result)
    {
        m_geometry.reset();
        m_summary->setText(parse_result.errorMessage());
        m_buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
        return;
    }
    m_vector_data = parse_result.value();
    rebuildGeometry();
}

void SVectorImportDialog::rebuildGeometry()
{
    m_spacing->setEnabled(m_fill_mode->currentIndex() != 2);
    m_angle->setEnabled(m_fill_mode->currentIndex() == 0);
    const SResult<SVectorImportGeometry> geometry_result =
        createVectorImportGeometry(m_vector_data, settings());
    if (!geometry_result)
    {
        m_geometry.reset();
        m_summary->setText(geometry_result.errorMessage());
        m_buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
        return;
    }
    m_geometry = geometry_result.value();
    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(!m_geometry->entities.empty());
    updatePreview();
}

void SVectorImportDialog::updatePreview()
{
    if (!m_geometry)
    {
        return;
    }
    QImage preview_image(720, 340, QImage::Format_ARGB32_Premultiplied);
    preview_image.fill(QColor(24, 29, 36));
    QPainter painter(&preview_image);
    painter.setRenderHint(QPainter::Antialiasing);
    const QRectF target = QRectF(preview_image.rect()).adjusted(18, 18, -18, -18);
    const QRectF bounds = geometryBounds(*m_geometry);
    for (const SColoredEntityGeometry& entity : m_geometry->entities)
    {
        drawEntity(painter, entity, bounds, target);
    }
    m_preview->setPixmap(QPixmap::fromImage(preview_image).scaled(
        m_preview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    QString summary =
        tr("尺寸：%1 × %2　区域：%3　预计原生实体：%4")
            .arg(m_geometry->width, 0, 'f', 3)
            .arg(m_geometry->height, 0, 'f', 3)
            .arg(m_geometry->region_count)
            .arg(m_geometry->entities.size());
    if (!m_geometry->warnings.isEmpty())
    {
        summary += tr("\n兼容性：%1").arg(m_geometry->warnings.join(QStringLiteral("；")));
    }
    m_summary->setText(summary);
}

void SVectorImportDialog::chooseBackgroundColor()
{
    const QColor selected =
        QColorDialog::getColor(m_selected_background, this, tr("选择要忽略的背景色"),
                               QColorDialog::ShowAlphaChannel);
    if (selected.isValid())
    {
        m_selected_background = selected;
        rebuildVectorData();
    }
}

void SVectorImportDialog::useTopLeftBackground()
{
    if (!m_bitmap.isNull())
    {
        m_selected_background = QColor::fromRgba(m_bitmap.pixel(0, 0));
        rebuildVectorData();
    }
}

SVectorImportSettings SVectorImportDialog::settings() const
{
    SVectorImportSettings result;
    if (m_fill_mode->currentIndex() == 0)
    {
        result.fill_mode = SImportFillMode::SingleLine;
    }
    else if (m_fill_mode->currentIndex() == 1)
    {
        result.fill_mode = SImportFillMode::PolygonOffset;
    }
    else if (m_fill_mode->currentIndex() == 2)
    {
        result.fill_mode = SImportFillMode::None;
    }
    result.fill_spacing = m_spacing->value();
    result.fill_angle_degrees = m_angle->value();
    result.scale = m_scale->value();
    result.preserve_outlines = m_preserve_outlines->isChecked();
    result.include_color_blocks = !m_is_fill_only;
    return result;
}

} // namespace smartGraphics
