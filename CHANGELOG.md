# Changelog

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
