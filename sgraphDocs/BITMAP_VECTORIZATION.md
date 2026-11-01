# 位图矢量化

> 适用版本：`v0.2.0-alpha.1`。范围：正式游程矢量化算法、四邻域 flood fill 基线、基准生成/
> 验证/报告、专项测试与维护约定。文中不包含逐函数源码讲解与历史重构记录。

## 1. 定位与边界

正式算法位于 `sgraphIo`，把输入 `QImage` 转成精确颜色的相连色块轮廓与 SVG。算法链路为：
**水平同色游程 → 相邻行重叠连接 → 并查集合并 → 有向边 → 共线压缩 → 闭合轮廓 → SVG**。

- 颜色使用 `QRgb` 精确比较（含 alpha），不量化、不聚类、不降色。
- 连通性为四邻域；仅在角点接触的同色像素分属不同组件。
- 适合：像素画、图标、标志、离散色块图、扫描后已规整颜色的图片。
- 非目标：K-means 等颜色聚类、Potrace、Bézier 拟合或曲线平滑、去噪、主动减少颜色。
- 不承诺所有图片都比逐像素方案快；内部多线程是同步等待，不等于 UI 后台异步。
- flood fill 只存在于 benchmark，用作正确性 oracle 与性能基线，不进入 `smartIo` 正式调用链。

## 2. 核心定义与不变量

**像素与背景**：输入先转 `QImage::Format_ARGB32`，逐像素用 `QRgb` 精确比较。`qAlpha==0`
视为背景；`ignore_background=true` 时颜色等于选定背景色也视为背景；背景统一映射为 `QRgb(0)`。

**四邻域**：像素 `(x,y)` 只与 `(x-1,y)`、`(x+1,y)`、`(x,y-1)`、`(x,y+1)` 相邻，`(x±1,y±1)`
不相邻。

**水平游程**：固定行内同色最大连续区间，用半开区间表示：

```text
R = (row, begin_x, end_x, color)      覆盖像素：begin_x <= x < end_x
```

半开区间让相邻区间共享端点而不重叠，长度、交集和边坐标都可直接用整数运算。

**相邻行连接条件**：游程 A、B 属于同一色块，当且仅当

```text
A.is_foreground && B.is_foreground
A.color == B.color
max(A.begin_x, B.begin_x) < min(A.end_x, B.end_x)
```

严格小于保证交集**至少覆盖一个像素列**（严格重叠）；等号表示只有网格端点接触，不建立 join。

**轮廓不变量**：

- 边坐标位于像素网格顶点，而不是像素中心；
- 每条边有方向，色块内部始终位于一致一侧；
- 每条边只属于一个 `block_id` 和一个颜色；
- 压缩只合并同组件、同色、同方向、同直线且首尾相接的边；
- 每条压缩边在闭环阶段只能使用一次；
- 成功轮廓至少三个顶点并闭合；相同输入与设置下排序稳定。

## 3. 处理流程

`bitmapToVectorResult()` 是正式算法总入口（`bitmapToSvgData()` 只是它的薄包装），按以下
阶段串行/并行推进：

1. **校验与规范化**：空图或非正尺寸失败；转 ARGB32；显式有效背景色优先，否则取左上角像素；
   `requestedWorkerCount()` 固定本次线程数。
2. **并行游程提取**：`rows[height]` 预分配，`parallelRanges()` 分片调用 `extractRowRuns()`，
   每个任务只写自己的行。
3. **稳定 `run_id`**：全部行任务完成后，串行按行号、行内 x 顺序给前景游程编号；该编号与线程
   完成顺序无关。没有前景游程则返回“位图中没有可转换的前景区域。”
4. **相邻行连接**：`connections` 大小为 `height+1`，索引 0 存顶边（左→右），`height` 存底边
   （右→左），中间索引由第二组 `parallelRanges()` 调用 `connectRows()` 产出 join 与水平边。
5. **并查集合并**：主线程按 connection 下标顺序、每个 joins 的顺序调用 `unite()`，再对全部
   前景游程 `find()` 统计唯一根，得到 `block_id`。
6. **有向边生成**：每个前景游程无条件生成左、右竖边（左边下→上，右边上→下）；水平边在连接
   阶段只带 `owner_id`，汇集后用 `find(owner_id)` 回填 `block_id`。
7. **共线压缩**：`compressSegments()` 归一化、排序、合并首尾相接的同直线边。
8. **闭环与规范化**：`stitchSegments()` 追踪闭环，`simplifyLoop()` 删除共线点，
   `canonicalizeLoop()` 旋转到固定首点，最后按 color、`block_id`、首点排序。
9. **SVG 输出**：`bitmapContoursToSvgData()` 按颜色聚合，输出 `M/L/Z` 路径。
10. **指标与返回**：记录各阶段耗时与计数，返回 `VpBitmapVectorResult`。

复杂度：设 P 像素、R 前景游程、J join、E 原始边、C 压缩边、V 顶点，扫描 O(P)、连接 O(R+J)、
并查集 O(Jα(R))、压缩 O(E log E)、闭环均摊 O(C+V)、SVG O(V+K log G)；额外空间 O(R+E+C+V)。

## 4. 数据结构

公共接口在 `sgraphIo/vp_bitmap_vectorizer.h`：

| 结构 | 关键字段 | 语义 |
| --- | --- | --- |
| `VpBitmapVectorSettings` | `ignore_background`（默认 true）、`background_color`、`worker_count` | 背景映射规则；`worker_count>0` 为显式线程数，0 为自动 |
| `VpBitmapVectorMetrics` | pixel/run/component/segment/contour_count、scan/connection/stitch/total_nanoseconds、estimated_working_bytes、svg_bytes、worker_count | 阶段规模与耗时；`estimated_working_bytes` 只估算核心工作容器，**不是进程峰值工作集** |
| `VpBitmapContour` | `color`、`block_id`、`points` | `color` 为原始 `QRgb`，`block_id` 为并查集根；`points` 为不重复首尾的闭合多边形，闭合由 SVG `Z` 表示 |
| `VpBitmapVectorResult` | `svg_data`、`contours`、`metrics` | 一次调用的完整产物；`contours` 供验证与导入复用 |

内部结构在 `vp_bitmap_vector_private.h`（命名空间 `bitmapVectorPrivate`）：

| 类型 | 字段与作用 |
| --- | --- |
| `VpBoundarySegment` | `start`、`end`、`color`、`owner_id`、`block_id`；供原始边、压缩边与闭环共用 |
| `VpColorRun` | `row`、`begin_x`/`end_x`、`color`、`run_id`、`is_foreground` |
| `VpRowConnection` | 一条行边界的 `joins` 与水平 boundary segments |
| `VpDisjointSet` | `parent` 加 8 位 `rank`，仅调用线程访问，无锁 |
| `VpNormalizedSegment` | block/color/direction/fixed/low/high，只在压缩函数内部存在 |

基准侧另有 case、算法摘要、case 结果、运行选项与验证结果结构，位于
`sgraphVectorBenchmark/src/vp_bitmap_benchmark_types.h`。

## 5. 关键函数职责

| 函数 | 位置 | 职责 |
| --- | --- | --- |
| `bitmapToVectorResult` | `vp_bitmap_run_vectorizer.cpp` | 总控：校验、游程、稳定 ID、连接、并查集、边生成、压缩、闭环、SVG、指标 |
| `bitmapToSvgData` | `vp_bitmap_vectorizer.cpp` | 薄包装：调用上者，成功时只移动 `svg_data`，失败原样传播错误 |
| `bitmapContoursToSvgData` | `vp_bitmap_vectorizer.cpp` | 规范轮廓 → UTF-8 SVG；按 `QMap` 颜色升序每种颜色一个 path，`fill-rule="evenodd"` 表达孔洞，少于 3 点的轮廓跳过 |
| `extractRowRuns` | `vp_bitmap_run_vectorizer.cpp` | 提取单行最大同色半开区间；写 `rows[row]`，无跨任务竞争 |
| `connectRows` | `vp_bitmap_run_vectorizer.cpp` | 双指针扫描相邻两行：严格重叠且同色则记 join，否则为前景侧生成水平有向边 |
| `requestedWorkerCount` / `parallelRanges` | `vp_bitmap_run_vectorizer.cpp` | 线程数决策；连续半开区间分片到局部 `QThreadPool`，返回前等待全部 future |
| `compressSegments` | `vp_bitmap_contour_stitcher.cpp` | 拒绝零长度；归一化后按全字段排序；重叠即失败；首尾相接的共线边合并；还原有向边 |
| `stitchSegments` | `vp_bitmap_contour_stitcher.cpp` | 每个 block 建 outgoing 表，按转向优先级追踪未用边；闭环后简化、规范化并稳定排序 |
| `simplifyLoop` / `canonicalizeLoop` | `vp_bitmap_contour_stitcher.cpp` | 用二维叉积删除共线点；旋转到字典序最小的 `(y,x)` 首点，不反转方向 |
| `bitmapToVectorFloodFill` | `vp_bitmap_flood_baseline.cpp` | 逐像素 BFS 基线，固定单线程；与正式算法共享压缩、闭环与 SVG 输出 |
| `validateBitmapBenchmarkCase` | benchmark validation | fail-fast 校验三算法一致性与回栅格 |
| `writeBitmapBenchmarkReport` | benchmark report | 聚合、分位数与 HTML 报告 |

闭环失败路径（错误文本直接传播，不静默丢弃）：闭合前重复使用线段、开放端点或无 outgoing、
同优先级候选并列、guard 超过本 block 边数仍未闭合、简化后少于三个顶点。

## 6. 并行与确定性

**线程数**：显式 `worker_count>0` 优先且至少为 1；自动模式下 `pixels < 1024*1024`（即**小于
1,048,576 像素自动单线程**）返回 1，否则取 `max(1, QThread::idealThreadCount())`。返回值可能
大于行数，实际任务数由 `parallelRanges()` 限制。

**分片**：`item_count<=0` 直接返回；`worker<=1` 同步执行整个区间；并行时
`task_count=min(item_count, worker*4)`、`chunk_size=ceil(item_count/task_count)`，提交连续
半开区间后逐个 `waitForFinished()`。该函数只提供执行机制，不保证 operation 自身线程安全；
两个实际调用点都只写互不重叠的 vector 元素。

**确定性来源**：

1. 每个并行任务只写固定下标，`rows`/`connections` 在任务前按最终大小分配；
2. `run_id` 在所有行任务完成后按行号、行内 x 顺序串行分配，与线程完成顺序无关；
3. join 按 connection 下标顺序串行 union，等秩时根选择固定；
4. 压缩对全字段排序；闭环使用固定转向优先级（右转 > 直行 > 左转 > 回头）并拒绝并列歧义；
5. 每个 loop 旋转到固定首点，contour 再按 color/`block_id`/首点稳定排序；
6. SVG 用 `QMap` 按颜色输出，配合整数坐标与固定数字格式。

因此**相同输入与设置下，单线程与多线程的 SVG 字节完全一致**；该结论由专项测试与基准验证
第 3 项双重锁定，任何并行分片、编号或规范化改动都必须重新证明它。

`bitmapToVectorResult()` 与 `bitmapToSvgData()` 均同步返回，调用线程被占满全程。GUI 若需异步，
必须在外层复制 `QImage` 与 settings 到后台任务，再把结果安全投递回 GUI 线程。

## 7. 正确性验证

基准对每个 case 跑三条路径：flood fill（oracle）、游程单线程（`worker=1`）、游程自动多线程
（N/auto），三者都用 `ignore_background=false`。验证顺序为 fail-fast：

1. 三算法 `component_count` 相同；
2. 三算法规范轮廓 SHA-256（`contourHash`）相同；
3. `serial.svg_data` 与 `parallel.svg_data` 字节相同；
4. 三份 SVG 都能被项目 SVG parser 重新解析；
5. flood / serial / parallel 轮廓回栅格后与源图逐像素相同。

第 5 步由 `rasterize()`（even-odd 扫描线填充）加 `compareImage()`（透明像素统一为
`(0,0,0,0)`，不匹配标红生成 difference 图）完成，独立于正式 SVG renderer。任一失败立即写
error 并保留最近 difference，全部通过才置 `passed=true`。

专项 Qt Test 位于 `VpSvgVectorImportTest`，编入 `vectorPathDesktopTests`，用
`vectorPathSvgVectorImportTests` 参数单独启动进程：

| 用例 | 锁定内容 |
| --- | --- |
| `vectorizesSolidBitmapAsOneRegion` | 300×300 单色 → 单游程/单组件/矩形闭环 |
| `keepsDiagonalPixelsAsSeparateContours` | 2×2 中两个对角红点 → 两个轮廓，四邻域端点不连接 |
| `vectorizesSplitMergeRunsWithSameParallelResult` | worker=1 与 worker=4 的组件、轮廓、SVG 字节一致 |
| `vectorizesHoleAsTwoClosedContours` | 挖孔矩形 → 一个组件、外环加内环两个闭合轮廓（even-odd） |

历史正确性结果为 1000/1000 与 5000/5000（失败 0）。

## 8. 性能测量与口径

**1000 文件相对均衡测试**

| 指标 | 结果 |
| --- | ---: |
| 正确性 | 1000/1000 |
| flood fill | 19,618.17 ms |
| 游程单线程 | 7,015.54 ms |
| 自动多线程 | 4,879.50 ms |
| 峰值工作集 | 359.3 MB |

**5000 文件压力测试**（seed 20260727、Release x64、12 线程、每算法 3 次取中位数）

| 指标 | 结果 |
| --- | ---: |
| 正确性 | 5000/5000，失败 0 |
| flood fill | 1,216,018.72 ms |
| 游程单线程 | 279,075.69 ms |
| 游程多线程 | 102,223.19 ms |
| 相对 flood fill | 11.90× |
| 相对单线程 | 2.73× |
| 逐文件 P10/P50/P90 | 2.88× / 13.70× / 27.80× |
| 峰值工作集 | 384.1 MB |

**测量口径**

- `measure()` 用 `QElapsedTimer` 包住完整 operation；N 次耗时排序后取 `times[size/2]`，只保留
  第一次完整 result，metrics 也来自第一次，中位数仅用于耗时。没有单独不计时的预热。
- 同一 case 的三算法执行顺序按 index 轮换，降低固定顺序偏向。
- `peakWorkingSet()` 读 Windows `GetProcessMemoryInfo` 的 `PeakWorkingSetSize`，是进程生命周期
  峰值，不是单 case 峰值；不要在报告里把它当成算法工作内存。
- 5000 组中 index 990 起固定 4096×4096，共 4010 张 4K，代表大图耐久压力而非均衡样本；
  以上为历史测量，新结果需在本地用保留的生成器、验证器和报告重新生成。

**运行与退出码**

```powershell
msbuild sgraphVectorBenchmark/vp_bitmap_benchmark.vcxproj /m /p:Configuration=Release /p:Platform=x64
build/msbuild/64/Release/smartBitmapVectorBenchmark.exe --cases 1000 --seed 20260727
```

| 退出码 | 含义 |
| --- | --- |
| 0 | 全部 case 通过 |
| 1 | 生成、报告等前置步骤失败 |
| 2 | 存在 case 验证失败 |
| 3 | 临时数据清理失败 |

**输出目录约定**：成功运行结束后只保留 `manifest.json` 与
`bitmap_vector_benchmark_report.html`；`cases/` 与 `failures/` 被 `removeTemporaryArtifacts()`
递归删除，整个结果目录由 Git 忽略。manifest 中的 `file` 只是运行期输入路径元数据，cases 已
清理，因此它不是长期 PNG 包。CI 用 `--smoke --cases 20` 运行，要求顶层条目排序后恰好为这两个
文件，且 manifest 的 `case_count` 与 `cases` 数量均为 20，任一不满足即失败。

## 9. 维护要点

| 改动点 | 必须同步 |
| --- | --- |
| 背景规则 | 正式与 baseline 两处 `effectiveColor`、专项测试、manifest 颜色口径 |
| 连通规则 | 半开区间与严格重叠定义、diagonal 用例、flood 参照实现 |
| 边方向 | 游程左右边、`appendBoundary`、`turnPriority`、孔洞与回栅格结果 |
| 并行分片 | 固定下标写入、稳定 `run_id`、future 生命周期与异常路径 |
| 轮廓规范化 | `contourHash`、SVG 字节一致性、历史基准可比性 |
| SVG 格式 | parser 验证、GUI 消费边界、字节确定性 |
| 生成器 | seed 可复现性与类别/尺寸说明 |
| 计时与报告 | 是否预热、保留第几次 metrics、中位数与分位数口径 |
| 输出清理 | CI 布局检查与 benchmark `README.md` |

## 10. 相关文档

- [完整代码架构与文件参考](CODEBASE_REFERENCE.md)
- [测试与验证](TESTING.md)
- [位图基准说明](../sgraphVectorBenchmark/README.md)
- [项目 README](../README.md)
