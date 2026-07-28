#include "s_cad_main_window.h"
#include "s_cad_viewport.h"
#include "s_cad_workspace_widget.h"
#include "s_icon_provider.h"

#include <QAction>
#include <SARibbonPanel.h>

namespace smartGraphics
{

void SCadMainWindow::configureDimensionPanel(SARibbonPanel* annotation_panel)
{
    struct SDimensionActionDefinition
    {
        const char* text;
        SIconType icon_type;
        SToolMode tool_mode;
        const char* tool_tip;
        const char* command_id;
    };
    const SDimensionActionDefinition definitions[] = {
        {"对齐标注", SIconType::DimensionAligned, SToolMode::AlignedDimension,
         "沿两点真实方向标注距离（DAL）", "annotation.dimension_aligned"},
        {"角度标注", SIconType::DimensionAngular, SToolMode::AngularDimension,
         "标注两条射线之间的夹角（DAN）", "annotation.dimension_angular"},
        {"半径标注", SIconType::DimensionRadius, SToolMode::RadiusDimension,
         "标注圆或圆弧半径（DRA）", "annotation.dimension_radius"},
        {"直径标注", SIconType::DimensionDiameter, SToolMode::DiameterDimension,
         "标注圆或圆弧直径（DDI）", "annotation.dimension_diameter"},
        {"弧长标注", SIconType::DimensionArcLength, SToolMode::ArcLengthDimension,
         "标注圆弧长度（DAR）", "annotation.dimension_arc_length"},
        {"坐标标注", SIconType::DimensionOrdinate, SToolMode::OrdinateDimension,
         "标注相对原点的 X 或 Y 坐标（DOR）", "annotation.dimension_ordinate"},
    };
    for (const SDimensionActionDefinition& definition : definitions)
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
    QAction* style_action = createAction(tr("标注样式"), SIconType::DimensionStyle);
    registerShortcutAction(style_action, QStringLiteral("annotation.dimension_style"));
    style_action->setToolTip(tr("管理文字高度、箭头、比例和精度（DIMSTYLE）"));
    connect(style_action, &QAction::triggered, this, &SCadMainWindow::showDimensionStyleManager);
    annotation_panel->addLargeAction(style_action);
}

} // namespace smartGraphics
