# 使用说明

## 启动

构建成功后进入对应输出目录并运行 `smartGraphics.exe`。构建脚本已把 Qt、QtADS 和
LibreDWG 所需运行文件放在同一目录树中。

## 基本工作流

1. 新建或打开 `.smartcad`、DXF、DWG 文件。
2. 通过 Ribbon 或命令行选择绘制/编辑功能。
3. 在视口单击输入点，也可输入绝对、相对或极坐标。
4. 使用对象捕捉、栅格、正交和对象追踪提高精度。
5. 在图层和特性面板调整颜色、线宽与实体属性。
6. 保存原生文档，或导出 DXF/DWG。

## 常用命令

- `LINE` / `L`：直线。
- `CIRCLE` / `C`：圆。
- `PLINE`：多段线。
- `UNDO` / `U`、`REDO`：撤销与重做。
- `OPEN`、`SAVE`、`NEW`：文档操作。
- `ZOOM EXTENTS`：缩放至全部实体。
- `DXFOUT`、`DWGIN`、`DWGOUT` / `EXPORTDWG`：交换格式。
- `IMPORTSVG` / `SVGIMPORT`：导入 SVG。
- `IMPORTBITMAP` / `BITMAPIMPORT`：位图精确矢量化。
- `SVGFILL`：对导入色块生成单线或多边形偏移填充。
- `SVGDEDUP UPPER|LOWER`：按图层优先级去除重叠。
- `SIMULATE` / `SIM`：刀路仿真。
- `TPSORT ...`：刀路排序。

完整快捷键见 `s_default_shortcuts.md`。

## 位图矢量化

位图导入按像素精确颜色分区，使用四邻域连通规则；斜对角接触不会被错误合并。导入对话框
可控制比例、填充方式和线程数量。算法不执行 K-means、Potrace 或曲线平滑。

## 文件兼容性

- `.smartcad`：项目原生格式，最完整地保存自研实体和设置。
- DXF：支持常用二维实体；复杂对象可能被简化或报告。
- DWG：由附带的 GNU LibreDWG 工具转换；高级或较新对象可能不完整。
- SVG：支持常用几何、颜色、变换和填充规则；文本、滤镜等会给出警告。

正式生产数据请保留原始文件和独立备份。
