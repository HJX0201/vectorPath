#include "vp_cad_document.h"
#include "vp_cad_main_window.h"
#include "vp_dialog_service.h"
#include "vp_document_transaction.h"
#include "vp_drawing_settings.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

namespace Vp
{

void VpCadMainWindow::showUnitsDialog()
{
    VpDialog dialog(this);
    dialog.setObjectName(QStringLiteral("smartUnitsDialog"));
    dialog.setWindowTitle(tr("图形单位"));
    dialog.setMinimumWidth(420);

    auto* layout = new QVBoxLayout(&dialog);
    auto* description =
        new QLabel(tr("几何始终使用双精度无单位世界坐标；插入单位用于交换、附着和显示。"), &dialog);
    description->setWordWrap(true);
    layout->addWidget(description);

    auto* form = new QFormLayout();
    auto* unit_combo = new QComboBox(&dialog);
    unit_combo->addItem(tr("无单位"), static_cast<int>(VpInsertionUnit::Unitless));
    unit_combo->addItem(tr("毫米 (mm)"), static_cast<int>(VpInsertionUnit::Millimeters));
    unit_combo->addItem(tr("厘米 (cm)"), static_cast<int>(VpInsertionUnit::Centimeters));
    unit_combo->addItem(tr("米 (m)"), static_cast<int>(VpInsertionUnit::Meters));
    unit_combo->addItem(tr("英寸 (in)"), static_cast<int>(VpInsertionUnit::Inches));
    unit_combo->addItem(tr("英尺 (ft)"), static_cast<int>(VpInsertionUnit::Feet));

    auto* angle_combo = new QComboBox(&dialog);
    angle_combo->addItem(tr("十进制度数"), static_cast<int>(VpAngleFormat::DecimalDegrees));
    angle_combo->addItem(tr("度/分/秒"), static_cast<int>(VpAngleFormat::DegreesMinutesSeconds));
    angle_combo->addItem(tr("百分度"), static_cast<int>(VpAngleFormat::Gradians));
    angle_combo->addItem(tr("弧度"), static_cast<int>(VpAngleFormat::Radians));

    auto* linear_precision = new QSpinBox(&dialog);
    linear_precision->setRange(0, 8);
    auto* angular_precision = new QSpinBox(&dialog);
    angular_precision->setRange(0, 8);
    const VpDrawingSettings current_settings = m_document->drawingSettings();
    unit_combo->setCurrentIndex(
        unit_combo->findData(static_cast<int>(current_settings.insertion_unit)));
    angle_combo->setCurrentIndex(
        angle_combo->findData(static_cast<int>(current_settings.angle_format)));
    linear_precision->setValue(current_settings.linear_precision);
    angular_precision->setValue(current_settings.angular_precision);
    form->addRow(tr("插入单位"), unit_combo);
    form->addRow(tr("角度格式"), angle_combo);
    form->addRow(tr("线性精度"), linear_precision);
    form->addRow(tr("角度精度"), angular_precision);
    layout->addLayout(form);

    auto* preview = new QLabel(&dialog);
    preview->setObjectName(QStringLiteral("smartUnitsPreview"));
    layout->addWidget(preview);
    const auto settings_from_controls = [=]()
    {
        VpDrawingSettings settings;
        settings.insertion_unit = static_cast<VpInsertionUnit>(unit_combo->currentData().toInt());
        settings.angle_format = static_cast<VpAngleFormat>(angle_combo->currentData().toInt());
        settings.linear_precision = linear_precision->value();
        settings.angular_precision = angular_precision->value();
        return settings;
    };
    const auto refresh_preview = [=]()
    {
        const VpDrawingSettings settings = settings_from_controls();
        preview->setText(
            tr("预览：长度 %1    角度 %2")
                .arg(formatLinearValue(1234.56789, settings), formatAngleValue(37.5125, settings)));
    };
    connect(unit_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), &dialog,
            refresh_preview);
    connect(angle_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), &dialog,
            refresh_preview);
    connect(linear_precision, QOverload<int>::of(&QSpinBox::valueChanged), &dialog,
            refresh_preview);
    connect(angular_precision, QOverload<int>::of(&QSpinBox::valueChanged), &dialog,
            refresh_preview);
    refresh_preview();

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    auto transaction = m_document->beginTransaction(tr("更改图形单位"));
    if (transaction->setDrawingSettings(settings_from_controls()))
    {
        transaction->commit();
    }
}

} // namespace Vp
