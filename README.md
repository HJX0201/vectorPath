# smartGraphics

smartGraphics 是一个面向 Windows 的现代二维 CAD/CAM 学习与技术展示项目，使用
C++17、Qt 5.12.10、OpenGL、CMake 和 Ninja 开发。项目覆盖二维绘制与编辑、图层和标注、
DWG/DXF/SVG/位图导入导出、刀路排序与仿真，以及可撤销的文档事务。

> 当前版本：`0.1.0-alpha.1`。项目仍在持续开发，不宣称完整兼容 AutoCAD。

## 主要能力

- 直线、圆、圆弧、椭圆、样条、多段线和常用规则图形。
- 修剪、延伸、打断、连接、倒角、圆角、偏移、阵列、拉伸、夹点编辑等二维操作。
- 图层、颜色、线宽、文字、标注、填充与可撤销/重做事务。
- 原生 `.smartcad` 文档、ASCII DXF、基于 GNU LibreDWG 适配器的 DWG 读写。
- SVG 色块导入、像素精确的位图矢量化、偏移线填充与图层去重。
- 刀路排序、方向调整和交互式仿真。
- 深色、浅色和高对比度主题，Ribbon、停靠面板、快捷键与命令行。

## 位图矢量化测试

固定种子 `20260727` 的 5000 文件 Release 基准全部通过正确性验证：

| 算法 | 累计耗时 |
| --- | ---: |
| 四邻域 flood fill | 1,216,018.72 ms |
| 水平游程单线程 | 279,075.69 ms |
| 水平游程 12 线程 | 102,223.19 ms |

多线程游程法相对 flood fill 为 **11.90×**，相对单线程游程法为 **2.73×**；测试峰值工作集
384.1 MB。该批次中 4010 张为 4096×4096，结果主要反映大图压力/耐久场景，不代表每种图片
都能取得相同加速。完整自包含报告位于
[`sgraphVectorBenchmark/results/5000-case-20260727/bitmap_vector_benchmark_report.html`](sgraphVectorBenchmark/results/5000-case-20260727/bitmap_vector_benchmark_report.html)。

## 快速构建

仓库**不包含 Qt SDK**。请先安装 Qt 5.12.10 MSVC 套件、Visual Studio 2022 C++ 工具、
CMake、Ninja 和 Python 3，然后运行：

```powershell
python sgraphBuildTools/s_build_64_release.py --clean
```

脚本会自动查找 Qt、激活 Visual Studio 编译环境、编译和运行测试，并使用所找到 Qt 的
`windeployqt` 把运行所需 DLL 和插件复制到 `build/64/Release`。32 位入口：

```powershell
python sgraphBuildTools/s_build_32_release.py --clean
```

如果 Qt 不在常用路径，可显式指定：

```powershell
python sgraphBuildTools/s_build_64_release.py --qt-dir C:\Qt\Qt5.12.10\5.12.10\msvc2017_64
```

也可设置 `SGRAPH_QT64_DIR`、`SGRAPH_QT32_DIR` 或 `QTDIR`。详细说明见
[`sgraphDocs/BUILDING.md`](sgraphDocs/BUILDING.md)。

## 文档

- [使用说明](sgraphDocs/USER_GUIDE.md)
- [代码架构](sgraphDocs/ARCHITECTURE.md)
- [完整编译工具链](sgraphDocs/BUILDING.md)
- [测试结果](sgraphDocs/TEST_RESULTS.md)
- [项目面试 100 问与参考回答](sgraphDocs/s_interview_100_questions.md)
- [实现状态](sgraphDocs/s_implementation_status.md)
- [默认快捷键](sgraphDocs/s_default_shortcuts.md)
- [第三方组件与许可证](THIRD_PARTY_NOTICES.md)

## 仓库结构

所有自研模块目录使用 `sgraph` 前缀；第三方源码与工具统一放在
`sgraphThirdParty`。`build`、`dist`、自动生成的基准图片和本地 Qt SDK 均不会提交。

## 许可与状态

smartGraphics 自研代码目前采用仓库内的
[`LICENSE.md`](LICENSE.md)；第三方组件继续适用各自许可证。
特别是 LibreDWG 为 GPLv3+，Qt 5.12.10 和 Qt Advanced Docking System 适用 LGPL
条款。发布二进制前请阅读 [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)。

欢迎通过 Issue 提交可复现的问题；安全问题请按 [`SECURITY.md`](SECURITY.md) 处理。
