# 编译 vectorPath

## Visual Studio 主入口

使用 Visual Studio 2022 打开根目录的 `vectorPath.sln`，选择 `Debug/Release` 和
`Win32/x64`，将 `vectorPath` 设为启动项目后生成、运行。各模块使用原生 `.vcxproj`，
源码和项目属性可以直接在 Visual Studio 中维护，不需要先生成解决方案。

默认“生成解决方案”只构建应用及依赖。两个测试项目和性能项目保留在解决方案中，默认
不勾选生成；可右键单独生成，或通过下述脚本构建并运行所需测试。

- Visual Studio 构建不需要 CMake、Ninja 或 Python。
- 命令行批量构建、测试和打包使用 Python 3.10 或更高版本。
- 自研 CMake 入口已移除，最后版本保留在 Git 提交 `12fb231`。第三方原有 CMake 文件
  保留为上游源码的一部分，当前构建不调用它们。

## 工具要求

- Windows 10/11。
- Visual Studio 2022，安装“使用 C++ 的桌面开发”、v143 工具集和 Windows SDK，包含 x86/x64 工具。
- 桌面模式需 Qt 5.12.10 MSVC 2017 套件：32 位用 `msvc2017`，64 位用 `msvc2017_64`。
- Visual Studio 中安装官方 [Qt VS Tools](https://doc.qt.io/qtvstools/qtvstools-how-to-install.html)。
  项目使用 Qt/MSBuild 的 `QtMoc`、`QtRcc` 项；可以在 Qt 项目设置及文件属性中编辑相关
  选项，生成源码由 Qt/MSBuild 加入编译。[官方构建说明](https://doc.qt.io/qtvstools/qtvstools-explanation-building.html)

Qt SDK 不放入仓库，构建只使用本机已安装的 SDK。纯核心模式不需要 Qt SDK 或 Qt/MSBuild，
也不构建 Ribbon、Qt ADS、文档、命令、IO、界面和应用。

Qt 目录可通过公共属性 `VpQtDir`、`SGRAPH_QT32_DIR` / `SGRAPH_QT64_DIR`、`QTDIR`
或公共属性文件支持的常见安装路径指定。Qt/MSBuild 通过 `QtMsBuild` 环境变量或
`%LOCALAPPDATA%/QtMsBuild` 查找；安装 Qt VS Tools 后重启 Visual Studio 使环境生效。
不应将个人 Qt 路径写死到已提交的工程文件。
也可在仓库根目录的 `vp_local.props` 中设置 `VpQtDir`；该文件已忽略，不提交个人路径。

## 命令行与批量验证

统一入口为 `sgraphBuildTools/vp_build.py`，直接调用 MSBuild。默认构建 64 位 Release 应用，
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
| `--bits 32|64` | 目标架构，默认 64；分别对应 Win32/x64。 |
| `--config Debug|Release` | 构建配置，默认 Release。 |
| `--core` | 仅构建无 Qt 核心，跳过 Qt、Qt/MSBuild 查找和运行库部署。 |
| `--test` | 构建并逐套件运行所选模式的测试，默认关闭。 |
| `--benchmarks` | 构建可选性能基准；搭配 `--test` 运行位图 smoke 和输出布局检查。 |
| `--jobs auto|N` | MSBuild 项目并行数及独立测试进程数，默认 auto。 |
| `--qt-dir <目录>` | 指定桌面 Qt 套件根目录。 |
| `--clean` | 清理本次模式、架构和配置对应的输出目录后重新构建。 |
| `--run` | 构建完成后启动应用；与 `--test` 同用时先通过测试。 |
| `--package` | 生成便携 ZIP 和 SHA-256 文件。 |

`--core` 不能与 `--run` 或 `--package` 同用。输出目录为：

- 桌面：`build/msbuild/<32或64>/<Debug或Release>`，主程序为 `vectorPath.exe`。
- 核心：`build/msbuild/core/<32或64>/<Debug或Release>`。

构建失败或已开启的测试失败均返回错误。开发和交付验证应显式使用 `--test`；修改位图
算法或基准输出时使用 `--test --benchmarks`。单独 `--benchmarks` 不运行完整性能测量。

Python 驱动将每个项目的编译进程数设为 `VpCompilerProcesses=1`，由 `--jobs` 控制项目
并行，避免项目并行与编译器 `/MP` 相乘。直接在 Visual Studio 中构建时，每项目默认
使用 `/MP2`；可通过 `VpCompilerProcesses` 覆盖。

## 当前无 Qt 范围

纯核心包括 `smartCore`、`smartGeometryCore` 与 Clipper2。基础结果、ID 集合、点、六种
曲线实体、标准形状、样条/椭圆、布尔、多边形算法，以及栅格、正交和追踪状态已脱离 Qt。
桌面复用同一份实现。Qt 点/文本适配及其余实体算法位于桌面 `smartGeometry` 的显式源列表。
文本和颜色实体、Document、命令及完整编辑交互尚未完成纯 C++ 迁移。

## 测试项目与性能基准

普通测试只生成 `vectorPathCoreTests` 和 `vectorPathDesktopTests` 两个可执行程序。
`sgraphBuildTools/vp_test.py` 保留 38 个套件，每个以参数启动独立进程；桌面套件仍分别采用
AppLess、Core、Gui 或 Widgets 初始化，Widgets 使用 offscreen 平台。`--core --test`
仅构建核心测试程序并运行 4 个套件。

桌面测试进程显式使用所选 Qt SDK 的 `plugins/platforms` 目录，保证 Widgets 测试能找到
offscreen 插件，避免已部署到程序目录的 Qt DLL 改变插件查找位置。该设置仅用于测试进程，
不修改应用正常运行的环境。

`--test --benchmarks` 在桌面模式增加位图 smoke 和输出布局两项，共 40 项。
`--core --benchmarks` 仅增加 ID 集合性能程序，不需要 Qt，也不运行位图基准。
正式位图测量入口为 `sgraphVectorBenchmark/vp_run_benchmark.py`；它调用同一个 MSBuild
驱动，按固定种子生成合成样本。样本、manifest 和 HTML 仅本地保存，整个结果目录由 Git 忽略。

## Qt 查找与运行部署

Python 驱动按 `--qt-dir`、`SGRAPH_QT32_DIR` / `SGRAPH_QT64_DIR`、`QTDIR`、本地
`.toolchain/Qt`、常见安装位置和 PATH 中的 qmake 查找 Qt，验证版本、架构、目标配置的
DLL 与部署工具。核心模式跳过这些检查。

```powershell
python sgraphBuildTools/vp_build.py --qt-dir C:/Qt/Qt5.12.10/5.12.10/msvc2017_64 --test
```

应用构建后部署 Qt DLL、插件、翻译、Qt ADS 与 LibreDWG 工具。Python 驱动额外生成
`runtime_manifest.json`，记录文件大小、架构、来源和 SHA-256；`--package` 生成便携 ZIP。
64 位 Release 包名为 `vectorPath-0.2.0-alpha.1-windows-x64.zip`。

## 直接调用 MSBuild

在 VS Developer PowerShell 中可直接构建原生解决方案：

```powershell
msbuild vectorPath.sln /m /p:Configuration=Release /p:Platform=x64
msbuild vectorPath.sln /m /p:Configuration=Debug /p:Platform=Win32
```

桌面 Qt 不在自动查找范围内时增加 `/p:VpQtDir=C:/Qt/Qt5.12.10/5.12.10/msvc2017_64`。
测试的批量执行和核心模式建议使用统一 Python 入口，避免遗漏逐套件环境与配置。

## CI 与第三方

Windows CI 保留无 Qt 核心 x64 Debug/Release 和桌面 x86/x64 Release；桌面运行 40 项，
核心运行 4 项。桌面任务从 Qt 官方下载固定版本
[Qt/MSBuild 3.5.0 ZIP](https://download.qt.io/official_releases/vsaddin/3.5.0/qt-vsaddin-msbuild-3.5.0.zip.mirrorlist)，
核验 SHA-256 后解压到临时目录，无需安装完整 Visual Studio 扩展：

```text
e6398999d7e2bacf2cb93839ef24e7a1be851dfc7d6e3dc7dc3e57c8efdb4ed8
```

第三方原生工程统一位于 `sgraphThirdParty/vp_*.vcxproj`，引用原样保留的 Clipper2、
SARibbon 2.9.0 与 Qt ADS 4.4.1 源码。GNU LibreDWG 0.14 仍作为外部 Windows 工具部署。
构建不修改 vendor 源码，不在运行构建时联网下载依赖；CI 安装工具链属于准备步骤。

- Qt 或架构不匹配：安装对应架构的 5.12.10 套件，或传 `--qt-dir` / `VpQtDir`。
- 找不到 Qt/MSBuild：安装 Qt VS Tools，或将官方 ZIP 解压后设置 `QtMsBuild`。
- 找不到编译器：在 Visual Studio Installer 中补装 C++ 桌面工具和 v143。
- 应用缺 DLL：重新生成应用，完成部署步骤。
- 未生成测试或基准：在 VS 中单独生成相应项目，或增加 `--test` / `--benchmarks`。
