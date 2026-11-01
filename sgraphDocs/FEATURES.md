# 功能与路线

本文记录已形成闭环的能力、已知差距和后续顺序。逐项证据与稳定功能 ID 保存在
`vp_smartcad_feature_inventory.yaml`、`vp_autocad_2026_feature_catalog.yaml` 和
`vp_autocad_gap_matrix.yaml`，AutoCAD 基线资料见 [AUTOCAD_BASELINE.md](AUTOCAD_BASELINE.md)。

## 完成判定

功能只有同时满足以下条件，才能在差距矩阵标记为 `complete`：

1. Ribbon 或属性页有专属、主题感知图标和可访问名称。
2. 命令行提供完整命令名和常用别名，关键流程不依赖鼠标。
3. 交互命令提供动态几何预览、取消处理和明确提示。
4. 所有写操作走 `VpDocumentTransaction`，可撤销重做。
5. 原生格式无损保存；交换格式不支持时生成兼容性报告。
6. 有自动测试，自研 `.h/.cpp` 均不超过 800 行。

Ribbon 占位按钮、空停靠面板和只有接口声明的功能只能记为 `stub`。

## 已实现

### 绘图与构造

- `LINE/L`、`PLINE/PL`、`RECTANG/REC`、`CIRCLE/C`、`ARC/A`、`ELLIPSE/EL`、`SPLINE/SPL`、`TEXT/T`、`MTEXT/MT`、`MLEADER/ML`、`DIMLINEAR/DLI`、`HATCH/H`。
- 多段线：绘制中用 `ARC/A`、`LINE/L` 切换弧段与直线段，三点确定真实 bulge；`WIDTH/W 起 止` 支持每段渐变宽度，世界坐标填充。
- 圆：圆心-半径、圆心-直径、2P、3P、`TTR 半径`（与两条直线/圆/圆弧相切）、`TTT`（三个对象，解析广义阿波罗尼斯解）。
- 圆弧：3P、CSE、SCE、SCA、CSA、SEA、SED、SER，角度/方向/半径可拾取或用 `ARC <方式> 数值` 输入。
- 全部构造方式都有阶段提示、动态预览、`U` 放弃点和单事务撤销。

### 编辑与修改

- `MOVE/M`、`COPY/CO`、`ROTATE/RO`、`SCALE/SC`（参考缩放）、`MIRROR/MI`（保留原对象）、`ALIGN/AL`（两组点完成平移旋转缩放）。
- `OFFSET/O`：直线、圆、圆弧、开放/闭合多段线、三次样条；bulge 弧按 5° 细分，尖角受限斜接或斜切，并清理自交。
- `TRIM/TR`：直线、圆、圆弧与含正负 bulge 的多段线之间裁剪，一次可产生两段结果。
- `EXTEND/EX`：直线、圆弧和开放多段线端部按拾取端延伸，支持有限曲线边界。
- `BREAK/BR`：两点打断直线、圆、圆弧。
- `JOIN/J`：任意顺序合并直线、同圆连续圆弧（闭环自动转整圆）、含 bulge 多段线。
- `EXPLODE/X`：多段线、填充边界、标注分解。
- `STRETCH/S`：交叉窗口局部拉伸，圆/圆弧/文字整体移动。
- `LENGTHEN/LEN`：按拾取端拉长或缩短，端弧保持圆心与圆弧编码。
- `FILLET/F`、`CHAMFER/CHA`：直线/圆/圆弧两两组合，支持 `FILLET R`、`CHAMFER D`；对多段线可整条处理。
- `BLEND/BL`：在直线、圆弧、样条端部间生成切向连续样条。
- 所有修改命令都有落图前预览，预览与提交共用同一几何求解。

### 阵列与关联

- `ARRAYRECT/AR`、`ARRAYPOLAR/AP`、`ARRAYPATH/PA` 创建关联阵列，成员保存稳定阵列 ID、源几何与完整参数。
- `ARRAYEDIT/AE`：`RECT/POLAR/PATH` 参数重生成、增删成员，单事务提交。
- 点选任一成员选中整组，悬停显示整组预览；原生格式 v16 无损保存，DXF 展开并报告关联降级。

### 夹点与选择

- 夹点：直线端点/中点、圆心/半径、圆弧中心/端点/半径、多段线顶点、文字插入点、标注控制点、填充边界顶点。
- 拖动预览、松开单事务提交；`Space` 循环拉伸/移动/旋转/缩放/镜像，`GRIP_*` 命令可直接选模式。
- 点选、窗口框选、交叉框选、Shift 追加；单选/多选用实体属性目录树展示常规属性、坐标、长度、角度、半径、面积与顶点。

### 注释、文字与标注

- `MTEXT/MT` 富文本（粗体/斜体/下划线）、边界宽度、字高、左右/居中/两端对齐；`TEXTALIGN` 调整单行文字对齐。
- `STYLE` 文字样式：字体族、固定高度、宽度因子、倾斜角、粗斜体、当前样式，可撤销。
- `MLEADER/ML` 三点创建引线，全程预览。
- `FILLET` 多段线弧段、`HATCH` 边界与 `DIMLINEAR` 支持原生保存和 DXF 往返；原生格式 v17 保存三类注释对象。

### 图层、样式与单位

- 图层：新建、受保护删除、当前层切换、显隐、锁定、冻结/解冻、隔离、颜色、线宽、线型、透明度、打印控制。
- 图层重命名、合并、连同内容删除，与实体归属、当前图层一起作为单条历史记录。
- 命名图层状态：保存、覆盖、恢复、删除（`LAYERSTATE SAVE|RESTORE|DELETE`）。
- 命令：`LAYER RENAME`、`LAYMRG`、`LAYDEL`、`LAYFRZ`、`LAYTHW`、`LAYISO`、`LINETYPE`、`TRANSPARENCY`。
- 图形单位：六种插入单位、十进制度数/度分秒/百分度/弧度、0–8 位显示精度；`UNITS`、`ANGLEFORMAT`、`LUPREC`、`AUPREC`。

### 辅助绘图与捕捉

- 捕捉：端点、中点、圆心、象限点、交点、垂足、切点、最近点，带类型标记与中文提示；另有外观交点、延伸捕捉、平行捕捉。
- 对象追踪：从已捕获特征点生成水平/垂直引导与多点交汇，`F11`、`OTRACK ON|OFF`。
- 极轴追踪 `F10`：1–90° 增量、贯穿视口引导线与角度标签，`POLAR ON|OFF|ANGLE`。
- 栅格捕捉 `F9`：可配置间距，`SNAP ON|OFF|数值`；栅格支持任意角度旋转与 Left/Top/Right 等轴测平面，`F5`、`GRID ROTATION`、`ISOPLANE`。
- 正交 `F8` 与极轴追踪互斥；线宽显示、平移、`ZOOM EXTENTS/PREVIOUS`、20 级视图历史与命名视图。

### 文件、文档与恢复

- 新建、打开、保存、另存为、撤销、重做、原子保存；`.vectorpath` 原生格式版本 24，兼容读取 1–23 与旧扩展名 `.smartcam`、`.smartcad`。
- DXF R2018：LINE、CIRCLE、ARC、LWPOLYLINE（42/40/41 组码）、TEXT、DIMENSION、HATCH 往返。
- DWG：GNU LibreDWG 0.14 外部进程隔离转换，基础二维导入、R2000 导出、版本降级报告与往返测试。
- 恢复：五分钟自动保存、最多五份恢复副本、命名/未命名恢复管理器；`AUDIT CHECK|FIX` 修复无效几何。
- SVG 导入（轮廓、区域、颜色层）、位图精确矢量化、`SVGFILL` 填充、`SVGDEDUP UPPER|LOWER` 去重。

### 命令与界面

- Ribbon、停靠面板、深色/浅色/高对比度主题、主题感知图标（见 [USER_GUIDE.md](USER_GUIDE.md)）。
- 命令行：绝对 `X,Y`、相对 `@dx,dy`、极坐标 `@距离<角度`；实时预览与事务提交。
- 命令别名、`.scr` 顺序执行、`F1` 帮助；不区分大小写的弹出建议、Tab 补全、Up/Down 历史。
- 刀路：`TPSORT` 排序、`SIMULATE/SIM` 仿真、方向调整与轨迹显示。

## 已知差距

- 只有模型空间，没有图纸空间布局、布局视口、比例锁定或视口图层替代；`space_name` 仅为兼容字段。
- 输出仅有 CTB/STB 解析与颜色/线宽映射、`PLOTSTYLE`/`STYLESMANAGER` 文件管理；无页面设置、打印预览、打印机/PDF 输出和批量发布。
- 表格、完整 DXF TABLE/LAYOUT 映射、代理对象原样保留未实现。
- 外部参照、PDF/图像附着、SHX 与字体修复未实现。
- AutoLISP、完整 SCR、插件 ABI、公开 SDK、CUI 编辑器未实现。
- 套索、栏选、多边形选择和选择循环未实现。
- 块功能已按用户要求移除。
- DWG 代理对象、布局和未映射字段不能原样往返。

## 后续顺序

1. 布局与布局视口基础。
2. 表格与完整 DXF TABLE/LAYOUT 映射。
3. 外部参照、PDF/图像附着、字体修复。
4. AutoLISP、SCR、插件 ABI、SDK、CUI。
5. P2：图形比较、Trace、标记导入、电子传递、图纸集、数据提取。
6. P3 三维升级保持暂停：`ACAD.3D.MODEL`、`ACAD.3D.EDIT`、`ACAD.3D.VISUALIZE` 固定 `out_of_scope`。

## 更新规则

- 每实现一项，按功能 ID 更新 `vp_autocad_gap_matrix.yaml`，并把源码证据加入
  `vp_smartcad_feature_inventory.yaml`。
- Autodesk 大版本升级只新增或变更对应功能 ID，不重写稳定 ID。
- 状态只由可运行代码、持久化能力和自动测试决定，不以 Ribbon 占位判断进度。
