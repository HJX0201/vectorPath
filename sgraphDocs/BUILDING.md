# 编译 vectorPath

## 一个入口选择构建内容

在仓库根目录运行 `python sgraphBuildTools/vp_build.py`。默认构建 64 位 Release 应用；
测试和性能基准默认关闭。所有架构、配置与核心模式共用此入口。

```powershell
python sgraphBuildTools/vp_build.py
python sgraphBuildTools/vp_build.py --bits 32 --config Debug
python sgraphBuildTools/vp_build.py --test --run
python sgraphBuildTools/vp_build.py --core --test --config Debug
python sgraphBuildTools/vp_build.py --test --benchmarks
```

| 参数 | 行为 |
| --- | --- |
| `--bits 32|64` | 目标架构，默认 64。 |
| `--config Debug|Release` | 构建配置，默认 Release。 |
| `--core` | 仅构建无 Qt 核心，跳过应用、Qt 查找和运行库部署。 |
| `--test` | 构建并运行所选模式的 CTest，默认关闭。 |
| `--benchmarks` | 构建可选性能基准，默认关闭；搭配 `--test` 运行已注册的基准检查。 |
| `--jobs auto|N` | 并行编译与测试数量，默认 auto。 |
| `--qt-dir <目录>` | 指定桌面 Qt 套件根目录。 |
| `--clean` | 清理本次模式、架构和配置对应的输出目录后重新构建。 |
| `--run` | 构建完成后启动应用；与 `--test` 同用时先通过测试。 |
| `--package` | 生成便携 ZIP 和 SHA-256 文件。 |

`--core` 不能与 `--run` 或 `--package` 同用。输出目录分别为：

- 桌面：`build/<32或64>/<Debug或Release>`，主程序为 `vectorPath.exe`。
- 核心：`build/core/<32或64>/<Debug或Release>`。

构建失败或已开启的测试失败均返回错误。应用开发和交付验证应显式使用 `--test`；修改位图
算法或基准输出时使用 `--test --benchmarks`。只有 `--benchmarks` 时不会自动运行完整性能测量。

## 工具要求

- Windows 10/11。
- Visual Studio 2022，安装“使用 C++ 的桌面开发”和 x86/x64 MSVC 工具。
- CMake 3.16 或更高版本、Ninja、Python 3.10 或更高版本。
- 桌面模式另需 Qt 5.12.10 MSVC 2017 套件：32 位用 `msvc2017`，64 位用 `msvc2017_64`。

Qt SDK 不放入 GitHub 仓库。脚本只使用本机已安装 Qt，不下载或修改 Qt。
`--core` 模式无需安装 Qt SDK，也不构建 Ribbon、Qt ADS、文档、命令、IO、界面和应用。

## 当前无 Qt 范围

纯核心包括 `smartCore`、`smartGeometryCore` 与 Clipper2。基础结果、ID 集合、点、六种
曲线实体、标准形状、样条/椭圆、布尔、多边形算法，以及栅格、正交和追踪状态已脱离 Qt。
桌面直接复用同一份实现。

Qt 点/文本适配与其余实体算法位于桌面 `smartGeometry` 的显式源列表；设置迁移位于
`smartGui`。文本和颜色实体、Document、命令及完整编辑交互尚未完成纯 C++ 迁移。

## 测试项目与性能基准

普通测试只生成两个可执行程序：`vectorPathCoreTests` 和 `vectorPathDesktopTests`。
CTest 仍保留 38 个套件名称，以参数选择套件并为每项创建独立进程；没有将多个 Qt
应用实例放进同一测试进程。`--core --test` 只构建核心测试程序并运行 4 个套件。

桌面模式同时使用 `--test --benchmarks` 时增加位图 smoke 和输出布局两项，共 40 项。
`--core --benchmarks` 仅增加 ID 集合性能程序，不需要 Qt，也不会执行位图基准。
正式位图测量仍可通过 `sgraphVectorBenchmark/vp_run_benchmark.py` 发起；它内部调用统一
构建入口开启基准，使用固定样例和独立结果目录。

## 桌面 Qt 自动查找

查找顺序为 `--qt-dir`、`SGRAPH_QT32_DIR` / `SGRAPH_QT64_DIR`、`QTDIR`、本地
`.toolchain/Qt`、`C:\Qt` / 用户目录 / LocalAppData 常见位置，最后是 PATH 中的 qmake。
脚本验证版本、架构、Debug/Release DLL、CMake 配置和 windeployqt。

```powershell
python sgraphBuildTools/vp_build.py --qt-dir C:\Qt\Qt5.12.10\5.12.10\msvc2017_64 --test
```

桌面完成后生成 `runtime_manifest.json`，记录运行文件的大小、架构、来源和 SHA-256。
windeployqt 复制 Qt DLL、插件和 MSVC 运行库；Qt SDK 本身不复制进仓库。
64 位 Release 包名为 `vectorPath-0.2.0-alpha.1-windows-x64.zip`。

## 手动 CMake

推荐使用统一 Python 入口。手动调试需先打开对应架构的 VS Developer PowerShell。
桌面示例：

```powershell
cmake -S . -B build/64/Release -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_PREFIX_PATH=C:/Qt/Qt5.12.10/5.12.10/msvc2017_64 `
  -DVECTORPATH_BUILD_DESKTOP=ON `
  -DVECTORPATH_BUILD_TESTS=ON `
  -DVECTORPATH_BUILD_BENCHMARKS=OFF
cmake --build build/64/Release --parallel
ctest --test-dir build/64/Release --output-on-failure
```

无 Qt 核心示例：

```powershell
cmake -S . -B build/core/64/Release -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DVECTORPATH_BUILD_DESKTOP=OFF `
  -DVECTORPATH_BUILD_TESTS=ON `
  -DVECTORPATH_BUILD_BENCHMARKS=OFF `
  -DCMAKE_DISABLE_FIND_PACKAGE_Qt5=ON
cmake --build build/core/64/Release --parallel
ctest --test-dir build/core/64/Release --output-on-failure
```

`VECTORPATH_BUILD_DESKTOP` 默认 ON，`VECTORPATH_BUILD_TESTS` 和
`VECTORPATH_BUILD_BENCHMARKS` 默认 OFF。旧 `SMARTCAM_BUILD_TESTS`、`SMARTCAD_BUILD_TESTS`
仍映射到新测试开关并给出弃用提示，仅承担一个版本的兼容责任。

## 第三方与常见问题

构建使用仓库内的 Clipper2、SARibbon 2.9.0、Qt Advanced Docking System 4.4.1 和
GNU LibreDWG 0.14 Windows 工具，无需配置时联网下载。纯核心只使用其中的 Clipper2。

- 找不到 Qt 或架构不匹配：安装对应架构的 5.12.10 套件，或传 `--qt-dir`。
- 找不到编译器：在 Visual Studio Installer 中补装 C++ 桌面工具。
- 应用缺 DLL：重新运行桌面构建入口，由 windeployqt 完成部署。
- 未生成测试或基准项目：按需增加 `--test` 或 `--benchmarks`。
