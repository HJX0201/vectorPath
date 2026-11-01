#pragma once

class QPainter;
class QPointF;

namespace Vp
{

// 绘制文件图标，has_plus 表示带新建角标。
void drawFileIcon(QPainter& painter, bool has_plus);
// 绘制图标上的箭头。
void drawArrowHead(QPainter& painter, const QPointF& point, bool points_right);

} // namespace Vp
