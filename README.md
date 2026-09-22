# vectorPath

vectorPath 是一个面向 Windows 的现代二维 CAD/CAM 学习与技术展示项目，使用
C++17、Qt 5.12.10、OpenGL、CMake 和 Ninja 开发。项目覆盖二维绘制与编辑、图层和标注、
DWG/DXF/SVG/位图导入导出、刀路排序与仿真，以及可撤销的文档事务。

> 当前版本：`0.2.0-alpha.1`。项目仍在持续开发，不宣称完整兼容 AutoCAD。

![vectorPath 位图矢量化结果界面](sgraphDocs/images/vp_bitmap_vectorization_ui.png)

> 位图经颜色分区和轮廓提取后转换为可编辑矢量实体，并按颜色组织到图层中。

## 主要能力

- 直线、圆、圆弧、椭圆、样条、多段线和常用规则图形。
- 修剪、延伸、打断、连接、倒角、圆角、偏移、阵列、拉伸、夹点编辑等二维操作。
- 图层、颜色、线宽、文字、标注、填充与可撤销/重做事务。
- 原生 `.vectorpath` 文档并兼容旧 `.smartcam`、`.smartcad` 文件、ASCII DXF、基于 GNU LibreDWG
  适配器的 DWG 读写。
- SVG 色块导入、像素精确的位图矢量化、偏移线填充与图层去重。
- 刀路排序、方向调整和交互式仿真。
- 深色、浅色和高对比度主题，Ribbon、停靠面板、快捷键与命令行。

## 位图矢量化：精确结果与大图优化

vectorPath 的位图导入不是模糊拟合，也不依赖 K-means、Potrace 或曲线平滑。算法按
像素精确比较颜色，使用四邻域判定色块，将结果转换为闭合 SVG 轮廓。它特别适合颜色边界
明确的图标、像素图、标牌和规则色块素材。

核心流程为：

`水平同色游程 → 相邻行严格重叠连接 → 并查集合并色块 → 生成有向边 → 共线压缩 → 闭合轮廓 → SVG`

游程采用半开区间 `[begin_x, end_x)`；相邻行只有同色游程真正重叠时才连接，端点接触不会
把斜对角像素错误合并。性能优化集中在数据表示和可控并行：游程提取与相邻行连接可并行，
ID 分配、并查集、轮廓压缩和 SVG 输出保持串行。小于 1,048,576 像素时，自动模式使用
单线程以避免线程调度开销。稳定 ID、规范化和排序保证单线程与多线程可以生成完全相同的
SVG 字节。

### 历史优化测量

基准程序与软件位图导入直接调用同一个 `bitmapToVectorResult()` 核心实现。以下为历史测量：
固定种子 `20260727`，在 64 位 Release 下使用 12 个自动线程，每种算法执行 3 次并取中位数。
原固定图片、逐文件 HTML 和 manifest 已从仓库移除；当前生成器使用合成图案，输入分布已有
变化，不能仅凭相同种子完全复现旧结果，也不将这些数值作为当前版本的验收结果。

| 测试集 | 正确性 | 四邻域 flood fill | 游程单线程 | 游程多线程 | 相对 flood fill | 相对单线程 | 峰值工作集 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1000 份相对均衡样本 | 1000/1000 | 19,618.17 ms | 7,015.54 ms | 4,879.50 ms | **4.02×** | **1.44×** | 359.3 MB |
| 5000 份大图压力样本 | 5000/5000 | 1,216,018.72 ms | 279,075.69 ms | 102,223.19 ms | **11.90×** | **2.73×** | 384.1 MB |

5000 份测试的逐文件加速比 P10/P50/P90 为 **2.88× / 13.70× / 27.80×**。其中 4010 张
是 4096×4096 图片，因此这组结果重点证明大图压力和长时间连续处理下的优化效果与稳定性，
不能据此承诺所有图片都获得相同加速；小图、棋盘格和高度离散色块可能受固定开销影响。
当前基准也没有设置单独的计时外预热轮次。

- [生成样本与运行基准](sgraphVectorBenchmark/README.md)
- [测试方法与结果说明](sgraphDocs/TEST_RESULTS.md)

## 快速构建

唯一构建入口为 `sgraphBuildTools/vp_build.py`。默认构建 64 位 Release 应用，测试与性能
基准默认关闭。桌面构建需本机安装 Qt 5.12.10 MSVC 套件、Visual Studio 2022 C++ 工具、
CMake、Ninja 和 Python 3；仓库不包含 Qt SDK。

```powershell
python sgraphBuildTools/vp_build.py
python sgraphBuildTools/vp_build.py --bits 32 --config Debug
```

脚本自动查找 Qt、激活 Visual Studio 环境并编译应用，将运行库部署到
`build/<位数>/<配置>`。需要测试或无 Qt 核心时显式开启：

```powershell
python sgraphBuildTools/vp_build.py --test
python sgraphBuildTools/vp_build.py --core --test
python sgraphBuildTools/vp_build.py --test --benchmarks
```

普通测试共用 `vectorPathCoreTests` 和 `vectorPathDesktopTests` 两个程序，38 个 CTest
套件仍各自启动独立进程。开启位图基准检查后共 40 项；无 Qt 模式仅运行 4 个核心套件。
`--benchmarks` 只开启基准构建，搭配 `--test` 才运行注册的基准检查。

Qt 不在常用路径时可传 `--qt-dir C:\Qt\Qt5.12.10\5.12.10\msvc2017_64`，或设置
`SGRAPH_QT64_DIR`、`SGRAPH_QT32_DIR`、`QTDIR`。完整参数和当前核心边界见
[构建说明](sgraphDocs/BUILDING.md)及[架构说明](sgraphDocs/ARCHITECTURE.md)。

## 文档

- [运行性能与内存优化计划](sgraphDocs/vp_performance_memory_plan.md)
- [完整代码架构与文件参考](sgraphDocs/CODEBASE_REFERENCE.md)
- [位图矢量化详尽源码与调用关系](sgraphDocs/BITMAP_VECTORIZATION_SOURCE_REFERENCE.md)
- [使用说明](sgraphDocs/USER_GUIDE.md)
- [简版架构说明](sgraphDocs/ARCHITECTURE.md)
- [完整编译工具链](sgraphDocs/BUILDING.md)
- [测试结果](sgraphDocs/TEST_RESULTS.md)
- [实现状态](sgraphDocs/vp_implementation_status.md)
- [默认快捷键](sgraphDocs/vp_default_shortcuts.md)
- [第三方组件与许可证](THIRD_PARTY_NOTICES.md)

## 仓库结构

所有自研模块目录使用 `sgraph` 前缀；第三方源码与工具统一放在
`sgraphThirdParty`。`build`、`dist`、基准样本、全部本地基准结果和本地 Qt SDK 均不会提交。
仓库保留基准运行入口、样本生成器和验证代码，图片、manifest 与 HTML 在运行时本地生成。

## 许可与状态

vectorPath 自研代码目前采用仓库内的
[`LICENSE.md`](LICENSE.md)；第三方组件继续适用各自许可证。
特别是 LibreDWG 为 GPLv3+，Qt 5.12.10 和 Qt Advanced Docking System 适用 LGPL
条款。发布二进制前请阅读 [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)。

欢迎通过 Issue 提交可复现的问题；安全问题请按 [`SECURITY.md`](SECURITY.md) 处理。
