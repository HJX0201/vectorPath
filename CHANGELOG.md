# Changelog

## 0.3.0-alpha.1 - 2026-07-31

- 产品由 smartCam 完整改名为 vectorPath，统一窗口标题、C++ 命名空间、CMake 项目、
  `vectorPath.exe`、运行清单和发布包名称。
- 新建原生文档默认使用 `.vectorpath`，继续兼容 `.smartcam`、`.smartcad`、
  `SMCAD001` magic、版本 24 清单和旧恢复/打印样式标识。
- 首次启动按优先级迁移 `smartCamLearning/smartCam` 和
  `smartCadLearning/smartGraphics` 中缺失的设置，不覆盖已有 vectorPath 设置。
- CMake 测试开关改为 `VECTORPATH_BUILD_TESTS`，旧 `SMARTCAM_BUILD_TESTS` 和
  `SMARTCAD_BUILD_TESTS` 保留一个版本的弃用兼容映射。
- 增加 `.smartcam` 向后兼容和两代旧设置迁移优先级测试；发布 Windows x64
  `v0.3.0-alpha.1` 预发布包。

## 0.2.0-alpha.1 - 2026-07-31

- 产品由 smartGraphics 完整改名为 smartCam，统一窗口标题、C++ 命名空间、CMake 项目、
  `smartCam.exe`、运行清单和发布包名称。
- 新建原生文档默认使用 `.smartcam`，继续兼容 `.smartcad`、`SMCAD001` magic、版本 24
  清单和旧恢复/打印样式标识。
- 首次启动时将 `smartCadLearning/smartGraphics` 中缺失的设置迁移到
  `smartCamLearning/smartCam`，不覆盖已有新设置。
- CMake 测试开关改为 `SMARTCAM_BUILD_TESTS`，旧 `SMARTCAD_BUILD_TESTS` 保留一个版本的
  弃用兼容映射。
- 增加设置迁移正常、幂等、空设置和写失败测试；64 位 Release 更新为 CTest 34/34 通过。

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
