# 编译 vectorPath

## 入口

唯一的构建入口是根目录的 `vectorPath.sln`：用 Visual Studio 2022 打开即可生成、调试和
运行，也可以在命令行直接调用 MSBuild。仓库不包含 CMake、Ninja 或 Python 构建脚本；
第三方源码自带的 CMake 文件只是上游原件，不参与构建。最后的 CMake 入口保留在 Git 提交
`12fb231`。

## 工具要求

- Windows 10/11。
- Visual Studio 2022，安装“使用 C++ 的桌面开发”、v143 工具集和 Windows SDK，包含 x86/x64 工具。
- 桌面模式需 Qt 5.12.10 MSVC 2017 套件：32 位用 `msvc2017`，64 位用 `msvc2017_64`。
- Visual Studio 中安装官方 [Qt VS Tools](https://doc.qt.io/qtvstools/qtvstools-how-to-install.html)。
  项目使用 Qt/MSBuild 的 `QtMoc`、`QtRcc` 项；可在 Qt 项目设置及文件属性中编辑相关选项，
  生成源码由 Qt/MSBuild 加入编译。[官方构建说明](https://doc.qt.io/qtvstools/qtvstools-explanation-building.html)

Qt SDK 不放入仓库，构建只使用本机已安装的 SDK。纯核心模式不需要 Qt SDK 或 Qt/MSBuild，
也不构建 Ribbon、Qt ADS、文档、命令、IO、界面和应用。

## 解决方案结构

解决方案包含 16 个原生 `.vcxproj`，按磁盘目录分组显示：

- 默认“生成解决方案”构建应用及其依赖共 12 个项目。
- 三个测试/基准项目（`vp_core_tests`、`vp_desktop_tests`、`vp_id_collection_benchmark`）
  和位图基准项目保留在解决方案中，但不参与默认生成；在 VS 中右键单独生成，或用下面的
  MSBuild 命令构建。
- 单元测试和性能基准共用 `vp_common.props`、`vp_defaults.props`、`vp_build.targets`
  三个公共属性文件，工程里只包含源文件列表和模块级配置。

## 命令行构建

在 VS 开发者 PowerShell 中执行：

```powershell
msbuild vectorPath.sln /m /p:Configuration=Release /p:Platform=x64
msbuild vectorPath.sln /m /p:Configuration=Debug /p:Platform=Win32
```

输出目录：

- 桌面：`build/msbuild/<32或64>/<Debug或Release>`，主程序为 `vectorPath.exe`。
- 核心：`build/msbuild/core/<32或64>/<Debug或Release>`。

纯核心只构建 `smartCore`、`smartGeometryCore` 与 Clipper2，不查找 Qt：

```powershell
msbuild sgraphGeometry/vp_geometry_core.vcxproj /m /p:Configuration=Release /p:Platform=x64 /p:VpCoreOnly=true
```

每个项目默认 `/MP2` 并行编译，可用 `/p:VpCompilerProcesses=N` 覆盖；项目间并行由
`/m` 控制。

## Qt 查找与运行部署

`vp_defaults.props` 按 `VpQtDir`、`SGRAPH_QT32_DIR` / `SGRAPH_QT64_DIR`、`QTDIR`、
仓库根的 `.toolchain/Qt/5.12.10`、`C:\Qt\Qt5.12.10\5.12.10\*` 等常见位置查找 Qt，
并在构建前校验版本必须为 5.12.10。Qt/MSBuild 通过 `QtMsBuild` 环境变量或
`%LOCALAPPDATA%/QtMsBuild` 查找；安装 Qt VS Tools 后重启 Visual Studio 使环境生效。
个人路径写进已忽略的 `vp_local.props`，不要提交到工程文件。

应用项目（`vp_app`）生成后由 `VpDeployRuntime` 目标部署 Qt DLL、插件、`qt_zh_CN.qm`
翻译、Qt ADS 与 LibreDWG 工具，并调用 `windeployqt`。测试和基准项目不打部署步骤，
运行测试时按下节设置环境变量。

## 测试

先生成测试程序，再按套件运行：

```powershell
msbuild sgraphTests/vp_core_tests.vcxproj /m /p:Configuration=Release /p:Platform=x64
msbuild sgraphTests/vp_desktop_tests.vcxproj /m /p:Configuration=Release /p:Platform=x64

$output = "build/msbuild/64/Release"
& "$output/vectorPathCoreTests.exe" --list-suites
& "$output/vectorPathCoreTests.exe" <套件名>
& "$output/vectorPathDesktopTests.exe" --list-suites
& "$output/vectorPathDesktopTests.exe" <套件名>
```

- 核心测试 4 个套件，桌面测试 38 个套件，两类程序都支持 `--list-suites` 列出名称。
- 桌面套件按注册表分别以 AppLess、Core、Gui 或 Widgets 初始化；Widgets 套件需要离屏
  平台。运行时设置：

  ```powershell
  $env:QT_QPA_PLATFORM = "offscreen"
  $env:QT_QPA_PLATFORM_PLUGIN_PATH = "<Qt>/plugins/platforms"
  $env:PATH = "<Qt>/bin;" + $env:PATH
  ```

- DWG 相关套件需要 LibreDWG 工具；未部署到程序目录时用
  `$env:VECTORPATH_LIBREDWG_DIR = "<仓库>/sgraphThirdParty/libredwg-0.14-win64"` 指定。
- 运行全部套件可以循环：

  ```powershell
  $exe = "$output/vectorPathDesktopTests.exe"
  & $exe --list-suites | ForEach-Object { & $exe $_; if ($LASTEXITCODE -ne 0) { throw "套件 $_ 失败" } }
  ```

## 性能基准

```powershell
msbuild sgraphTests/vp_id_collection_benchmark.vcxproj /m /p:Configuration=Release /p:Platform=x64
msbuild sgraphVectorBenchmark/vp_bitmap_benchmark.vcxproj /m /p:Configuration=Release /p:Platform=x64

& "build/msbuild/64/Release/vectorPathIdCollectionBenchmark.exe"
& "build/msbuild/64/Release/smartBitmapVectorBenchmark.exe" --smoke --cases 20 --output build/benchmark-smoke
```

位图基准支持 `--cases`、`--seed`、`--threads`、`--repetitions`、`--output` 和
`--smoke`。输出目录只保留 `manifest.json` 与 `bitmap_vector_benchmark_report.html`，
样本、清单和报告仅本地保存，整个结果目录由 Git 忽略。样本生成与结果说明见
[sgraphVectorBenchmark/README.md](../sgraphVectorBenchmark/README.md)。

## CI

`.github/workflows/windows-ci.yml` 用 MSBuild 直接构建解决方案：core 任务构建无 Qt
核心及其测试并运行 4 个套件；build 任务在 x86/x64 Release 上构建解决方案、测试程序和
基准程序，运行全部套件，再跑一次 20 份样本的位图 smoke 并校验输出目录布局。

桌面任务从 Qt 官方下载固定版本
[Qt/MSBuild 3.5.0 ZIP](https://download.qt.io/official_releases/vsaddin/3.5.0/qt-vsaddin-msbuild-3.5.0.zip.mirrorlist)，
核验 SHA-256 后解压到临时目录，无需安装完整 Visual Studio 扩展：

```text
e6398999d7e2bacf2cb93839ef24e7a1be851dfc7d6e3dc7dc3e57c8efdb4ed8
```

## 第三方

第三方原生工程统一位于 `sgraphThirdParty/vp_*.vcxproj`，引用原样保留的 Clipper2、
SARibbon 2.9.0 与 Qt ADS 4.4.1 源码。GNU LibreDWG 0.14 仍作为外部 Windows 工具部署。
构建不修改 vendor 源码，也不在构建时联网下载依赖。

## 常见问题

- Qt 或架构不匹配：安装对应架构的 5.12.10 套件，或传 `/p:VpQtDir=<路径>`。
- 找不到 Qt/MSBuild：安装 Qt VS Tools，或将官方 ZIP 解压后设置 `QtMsBuild`。
- 找不到编译器：在 Visual Studio Installer 中补装 C++ 桌面工具和 v143。
- 应用缺 DLL：重新生成应用，确认 `VpDeployRuntime` 完成部署。
- 未生成测试或基准：它们在解决方案中默认不生成，右键单独生成或用上面的 MSBuild 命令。
