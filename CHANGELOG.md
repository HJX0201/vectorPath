# Changelog

## Unreleased

- 构建入口迁移为原生 `vectorPath.sln` 和模块 `.vcxproj`，支持 Win32/x64、Debug/Release；
  自研 CMake 入口移除，遗留版本保留在提交 `12fb231`。
- Qt/MSBuild 负责 moc/rcc；保留按需构建的两个测试程序和基准，输出统一到 `build/msbuild`。
- 四种桌面配置各通过 40 项检查，无 Qt 核心 Debug/Release 各通过 4 项检查。

## 0.2.0-alpha.1 - 2026-07-31

- 产品由 smartGraphics 完整改名为 vectorPath（开发过程中曾使用 smartCam 中间名称），
  统一窗口标题、C++ 命名空间、CMake 项目、
  `vectorPath.exe`、运行清单和发布包名称。
- 新建原生文档默认使用 `.vectorpath`，继续兼容 `.smartcam`、`.smartcad`、
  `SMCAD001` magic、版本 24 清单和旧恢复/打印样式标识。
- 首次启动按优先级迁移 `smartCamLearning/smartCam` 和
  `smartCadLearning/smartGraphics` 中缺失的设置，不覆盖已有 vectorPath 设置。
- CMake 测试开关改为 `VECTORPATH_BUILD_TESTS`，旧 `SMARTCAM_BUILD_TESTS` 和
  `SMARTCAD_BUILD_TESTS` 保留一个版本的弃用兼容映射。
- 增加设置迁移正常、幂等、空设置、写失败、`.smartcam` 向后兼容和
  两代旧设置迁移优先级测试；发布 Windows x64 `v0.2.0-alpha.1` 预发布包。

## 0.1.0-alpha.1 - 2026-07-27

- 整理为 GitHub 可发布的 smartGraphics 项目结构。
- 自研目录统一使用 `sgraph` 前缀，C++ 命名空间统一为 `smartGraphics`。
- 主程序输出统一为 `smartGraphics.exe`。
- 增加 32/64 位 Debug/Release 四套自动构建、Qt 查找和运行库部署入口。
- 第三方组件集中到 `sgraphThirdParty`，Qt SDK 不进入仓库。
- 独立位图矢量化基准模块，按日期序号保留 1000 与 5000 文件自包含报告和清单；运行中
  支持断点续生成，成功后自动清理批量 PNG。
- 主窗口切换为 Windows 原生标题栏，由系统绘制最小化、最大化/还原和关闭按钮，同时
  保留现有 Ribbon 菜单与工作区布局。
- 修复对象捕捉模式过滤和 SVG 数字解析问题。
- 32/64 位 Debug/Release 四套配置曾完成 32 项 CTest；新增基准输出结构验收后，64 位
  Release 已更新为 33/33 通过。
