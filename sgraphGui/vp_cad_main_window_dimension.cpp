#include "vp_cad_main_window.h"
#include "vp_cad_viewport.h"
#include "vp_cad_workspace_widget.h"
#include "vp_icon_provider.h"

#include <QAction>
#include <SARibbonPanel.h>

namespace Vp
{

void VpCadMainWindow::configureDimensionPanel(SARibbonPanel* annotation_panel)
{
    struct VpDimensionActionDefinition
    {
        const char* text;
        VpIconType icon_type;
        VpToolMode tool_mode;
        const char* tool_tip;
        const char* command_id;
    };
    const VpDimensionActionDefinition definitions[] = {
        {"对齐标注", VpIconType::DimensionAligned, VpToolMode::AlignedDimension,
         "沿两点真实方向标注距离（DAL）", "annotation.dimension_aligned"},
        {"角度标注", VpIconType::DimensionAngular, VpToolMode::AngularDimension,
         "标注两条射线之间的夹角（DAN）", "annotation.dimension_angular"},
        {"半径标注", VpIconType::DimensionRadius, VpToolMode::RadiusDimension,
         "标注圆或圆弧半径（DRA）", "annotation.dimension_radius"},
        {"直径标注", VpIconType::DimensionDiameter, VpToolMode::DiameterDimension,
         "标注圆或圆弧直径（DDI）", "annotation.dimension_diameter"},
        {"弧长标注", VpIconType::DimensionArcLength, VpToolMode::ArcLengthDimension,
         "标注圆弧长度（DAR）", "annotation.dimension_arc_length"},
        {"坐标标注", VpIconType::DimensionOrdinate, VpToolMode::OrdinateDimension,
         "标注相对原点的 X 或 Y 坐标（DOR）", "annotation.dimension_ordinate"},
    };
    for (const VpDimensionActionDefinition& definition : definitions)
    {
        QAction* action = createAction(tr(definition.text), definition.icon_type);
        action->setToolTip(tr(definition.tool_tip));
        registerShortcutAction(action, QString::fromLatin1(definition.command_id));
        connect(action, &QAction::triggered, this,
                [this, tool_mode = definition.tool_mode]()
                {
                    m_workspace->viewport()->setToolMode(tool_mode);
                });
        annotation_panel->addLargeAction(action);
    }
    QAction* style_action = createAction(tr("标注样式"), VpIconType::DimensionStyle);
    registerShortcutAction(style_action, QStringLiteral("annotation.dimension_style"));
    style_action->setToolTip(tr("管理文字高度、箭头、比例和精度（DIMSTYLE）"));
    connect(style_action, &QAction::triggered, this, &VpCadMainWindow::showDimensionStyleManager);
    annotation_panel->addLargeAction(style_action);
}

} // namespace Vp
