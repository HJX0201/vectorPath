# 使用说明

## 启动

构建完成后进入 `build/msbuild/<位数>/<配置>` 运行 `vectorPath.exe`。构建时已把 Qt 运行库、
Qt ADS 和 LibreDWG 工具部署到同一目录。

## 基本工作流

1. 新建或打开 `.vectorpath`、旧 `.smartcam`、`.smartcad`、DXF、DWG 文件。
2. 从 Ribbon 或命令行选择绘制、编辑命令。
3. 在视口拾取点，或直接输入绝对、相对、极坐标。
4. 用对象捕捉、栅格、正交、对象追踪提高精度。
5. 在图层面板和属性面板调整颜色、线宽与实体属性。
6. 保存原生文档，或导出 DXF/DWG。

## 常用命令

| 领域 | 命令 |
| --- | --- |
| 绘图 | `LINE/L`、`PLINE/PL`、`RECTANG/REC`、`CIRCLE/C`、`ARC/A`、`ELLIPSE/EL`、`SPLINE/SPL` |
| 编辑 | `MOVE/M`、`COPY/CO`、`ROTATE/RO`、`SCALE/SC`、`MIRROR/MI`、`OFFSET/O`、`TRIM/TR`、`EXTEND/EX`、`BREAK/BR`、`JOIN/J`、`EXPLODE/X`、`STRETCH/S`、`LENGTHEN/LEN`、`FILLET/F`、`CHAMFER/CHA`、`BLEND/BL`、`ALIGN/AL` |
| 阵列 | `ARRAYRECT/AR`、`ARRAYPOLAR/AP`、`ARRAYPATH/PA`、`ARRAYEDIT/AE` |
| 注释 | `TEXT/T`、`MTEXT/MT`、`MLEADER/ML`、`DIMLINEAR/DLI`、`HATCH/H`、`STYLE` |
| 图层 | `LAYER`、`LAYMRG`、`LAYDEL`、`LAYFRZ`、`LAYTHW`、`LAYISO`、`LAYERSTATE`、`LINETYPE`、`TRANSPARENCY` |
| 视图 | `ZOOM EXTENTS`、`PAN`、`REGEN`、`VIEW` |
| 文件 | `NEW`、`OPEN`、`SAVE`、`DXFOUT`、`DWGIN`、`DWGOUT` |
| 导入 | `IMPORTSVG`、`IMPORTBITMAP`、`SVGFILL`、`SVGDEDUP UPPER / LOWER` |
| 刀路 | `TPSORT`、`SIMULATE/SIM` |

命令行支持绝对 `X,Y`、相对直角 `@dx,dy` 和相对极坐标 `@距离<角度`；输入时视口实时
预览，Tab 补全命令与选项，Up/Down 浏览历史，`F1` 打开命令帮助。

## 默认快捷键

可在“管理 → 快捷键”或用 `CUI`、`SHORTCUTS`、`KEYBOARD` 修改，结果保存在当前用户配置。

| 快捷键 | 功能 | 快捷键 | 功能 |
| --- | --- | --- | --- |
| `Ctrl+N` / `Ctrl+O` / `Ctrl+S` | 新建 / 打开 / 保存 | `Ctrl+Shift+S` | 另存为 |
| `Ctrl+Z` / `Ctrl+Y` | 撤销 / 重做 | `Ctrl+P` | 打印 |
| `Ctrl+A` / `Delete` | 全选 / 删除 | `Ctrl+1` / `Ctrl+3` / `Ctrl+9` | 特性 / 工具选项板 / 命令行 |
| `Ctrl+K` / `Ctrl+L` | 命令搜索 / 聚焦命令行 | `Ctrl+,` / `Ctrl+.` | 仿真减速 / 加速 |
| `F1` | 帮助 | `F3` | 对象捕捉 |
| `F5` | 等轴测平面 | `F7` | 显示栅格 |
| `F8` | 正交 | `F9` | 栅格捕捉 |
| `F10` | 极轴追踪 | `F11` | 对象追踪 |
| `L` | 直线 | `Ctrl+Shift+T` | 生成加工刀路 |
| `F6` / `Shift+F6` / `Ctrl+F6` / `F12` | 仿真播放 / 暂停 / 停止 / 单步 | `Ctrl+Shift+H` | 显示或隐藏刀路 |

没有预设组合键的功能也会出现在设置界面；设置会阻止重复快捷键，“清除当前”只清选中项，
“恢复全部默认”删除全部用户覆盖。

## 界面与图标

`VpIconProvider` 用 Qt 矢量绘制生成主题感知图标，按当前设计令牌取前景色与强调色，高 DPI
版本由 Qt 生成。图标按域分组：文件与历史、绘图与修改、视图与主题、功能面板、布局与输出、
状态与扩展。所有已显示的 Ribbon 动作、停靠面板、空间标签和状态栏都必须使用专属图标；
新增功能先扩展 `VpIconType`，不要退回通用占位图。

## 位图矢量化

位图导入按像素精确颜色分区，使用四邻域连通规则，斜对角接触不会合并。导入对话框可设置
比例、填充方式和线程数；算法不做 K-means、Potrace 或曲线平滑。细节见
[BITMAP_VECTORIZATION.md](BITMAP_VECTORIZATION.md)。

## 文件兼容性

| 格式 | 说明 |
| --- | --- |
| `.vectorpath` | 默认原生格式，最完整地保存实体与设置（版本 24，兼容读取 1–23）。 |
| `.smartcam` / `.smartcad` | 上一代与旧版原生扩展名，继续支持打开和保存。 |
| DXF | 常用二维实体往返；复杂对象可能简化或写入兼容性报告。 |
| DWG | 由随附的 GNU LibreDWG 工具转换；高级或较新对象可能不完整。 |
| SVG | 支持常用几何、颜色、变换与填充规则；文本、滤镜等给出警告。 |

正式生产数据请保留原始文件和独立备份。

## 相关文档

[功能与路线](FEATURES.md) · [构建](BUILDING.md) · [测试](TESTING.md) ·
[性能](PERFORMANCE.md) · [架构](ARCHITECTURE.md)
