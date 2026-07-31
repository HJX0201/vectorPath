# 编译 vectorPath

## 支持矩阵

| 架构 | 配置 | 入口 | 输出目录 |
| --- | --- | --- | --- |
| 32 位 | Debug | `s_build_32_debug.py` | `build/32/Debug` |
| 32 位 | Release | `s_build_32_release.py` | `build/32/Release` |
| 64 位 | Debug | `s_build_64_debug.py` | `build/64/Debug` |
| 64 位 | Release | `s_build_64_release.py` | `build/64/Release` |

四个入口均位于 `sgraphBuildTools`。生成的主程序统一名为 `vectorPath.exe`。

## 必需工具

- Windows 10/11。
- Visual Studio 2022，安装“使用 C++ 的桌面开发”和 x86/x64 MSVC 工具。
- CMake 3.16 或更高版本。
- Ninja。
- Python 3.10 或更高版本。
- Qt 5.12.10 MSVC 2017 套件：
  - 32 位：`msvc2017`
  - 64 位：`msvc2017_64`

Qt SDK 体积较大且受其许可证约束，因此不放入 GitHub 仓库。构建脚本只使用本机已安装
Qt，不会下载或修改 Qt。

## Qt 自动查找

查找顺序如下：

1. 命令参数 `--qt-dir`。
2. `SGRAPH_QT32_DIR` 或 `SGRAPH_QT64_DIR`。
3. `QTDIR`。
4. 仓库外部/本地的 `.toolchain/Qt`。
5. `C:\Qt`、用户目录和 LocalAppData 中的常见安装位置。
6. `PATH` 中的 `qmake`。

脚本会验证 Qt 版本、架构、Debug/Release DLL、CMake 配置和 `windeployqt`。未找到时会
列出已检查位置及解决方式。

## 构建示例

```powershell
python sgraphBuildTools/s_build_64_release.py --clean --jobs auto
python sgraphBuildTools/s_build_32_debug.py --clean --jobs 8
```

常用参数：

- `--qt-dir <目录>`：指定 Qt 套件根目录。
- `--clean`：清理当前架构和配置的输出目录。
- `--jobs auto|N`：并行编译数量。
- `--run`：通过测试后启动应用。
- `--package`：生成便携 ZIP 和 SHA-256 文件；64 位 Release 包名为
  `vectorPath-0.3.0-alpha.1-windows-x64.zip`。
- `--no-test`：仅在明确不需要测试时使用。

构建完成后脚本运行 CTest，并生成 `runtime_manifest.json`，记录运行文件的大小、架构、
来源和 SHA-256。`windeployqt` 会自动复制 Qt DLL、平台插件、图像插件和 MSVC
运行库；Qt SDK 本身不会复制进仓库。

## 第三方目录

构建所需的非 Qt 第三方源码或运行组件统一位于 `sgraphThirdParty`：

- Clipper2
- SARibbon 2.9.0
- Qt Advanced Docking System 4.4.1
- GNU LibreDWG 0.14 Windows 工具

无需在配置阶段联网下载依赖。

## 手动 CMake

推荐使用 Python 入口。需要手动调试时，可先打开对应的 VS Developer PowerShell：

```powershell
cmake -S . -B build/64/Release -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_PREFIX_PATH=C:/Qt/Qt5.12.10/5.12.10/msvc2017_64
cmake --build build/64/Release --parallel
ctest --test-dir build/64/Release --output-on-failure
```

新项目应使用 `VECTORPATH_BUILD_TESTS` 控制测试构建。旧的 `SMARTCAM_BUILD_TESTS`、
`SMARTCAD_BUILD_TESTS` 仍会映射到新变量并显示弃用提示，仅用于一个版本的构建兼容。

## 常见问题

- “找不到 Qt”：安装正确架构的 5.12.10 套件，或使用 `--qt-dir`。
- “Qt 架构不匹配”：32 位必须使用 `msvc2017`，64 位必须使用 `msvc2017_64`。
- “找不到编译器”：在 Visual Studio Installer 中补装 C++ 桌面工具。
- 程序启动缺 DLL：不要手工挑 DLL；重新运行构建脚本，让 `windeployqt` 完成部署。
