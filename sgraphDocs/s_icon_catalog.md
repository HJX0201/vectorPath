# vectorPath 图标目录

`SIconProvider` 使用 Qt 矢量绘制生成主题感知图标。图标会根据当前设计令牌自动使用前景色和强调色，并由 Qt 自动生成高 DPI 版本。

## 文件与历史

- Application、NewFile、OpenFile、Save、SaveAs、ExportDxf、ExportDwg、Recovery、Audit
- Undo、Redo

## 绘图与修改

- DrawLine、DrawCircle、DrawPolyline、DrawArc、DrawRectangle
- Select、Move、Copy、Rotate、Scale、Mirror、Trim、Extend、Break、Join、Explode、Stretch、Fillet、Chamfer、ArrayRect、ArrayPolar、ArrayPath、Offset
- Dimension、Text、Hatch

## 视图和主题

- ZoomExtents、Pan、Viewport
- ThemeDark、ThemeLight、ThemeHighContrast
- ModelSpace、LayoutSpace

## 功能面板

- Layers、Properties、ExternalReference
- ToolPalette、DesignCenter、SheetSet
- CompatibilityReport、CommandLine

## 布局与输出

- ModelSpace、LayoutSpace、PageSetup、PlotStyle、PlotPreview、Print、ExportPdf、Publish

## 状态与扩展

- Snap、Grid、Ortho、Polar、Tracking、Lineweight
- Settings、Plugin、Script、Help

所有已显示的 Ribbon 动作、停靠面板、空间标签和状态栏功能都必须使用上述专属图标。新增功能时应先扩展 `SIconType`，禁止退回通用占位图标。
