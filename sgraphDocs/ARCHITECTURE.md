# 代码架构

smartGraphics 使用分层 CMake 目标组织。依赖方向由基础数据、几何和文档逐步指向交互、
界面与应用入口，底层模块不依赖主窗口。

```text
sgraphCore
    ├─ sgraphGeometry
    │    └─ sgraphDocument
    │         ├─ sgraphCommands
    │         ├─ sgraphIo
    │         └─ sgraphRender
    │                └─ sgraphGui
    │                       └─ sgraphApp
    └─ sgraphThirdParty
```

## 模块职责

- `sgraphCore`：结果类型、通用基础设施。
- `sgraphGeometry`：二维几何、曲线、布尔运算、填充与刀路算法。
- `sgraphDocument`：实体模型、图层、样式、事务、撤销重做、恢复和原生持久化。
- `sgraphCommands`：命令目录、别名与坐标输入解析。
- `sgraphIo`：DXF/DWG/SVG/位图编解码及矢量导入。
- `sgraphRender`：OpenGL 视口、选择、捕捉、预览和交互式绘制编辑。
- `sgraphGui`：Ribbon、停靠工作区、主题、图标、对话框和主窗口行为。
- `sgraphApp`：应用启动、Qt 配置和中文界面翻译。
- `sgraphTests`：按模块组织的自动化单元与集成测试。
- `sgraphVectorBenchmark`：位图矢量化生成器、正确性验证、性能基准和历史结果。
- `sgraphBuildTools`：四种 Windows 构建入口、Qt 发现和运行库部署。
- `sgraphThirdParty`：原样引入的第三方源码与运行组件。

## 关键设计

- 命名空间统一为 `smartGraphics`。
- 自研 C++ 文件使用 `s_` 前缀，每个 `.h/.cpp` 不超过 800 行。
- 文档修改通过 `SDocumentTransaction` 提交，提供统一撤销/重做边界。
- 视口只持有文档引用，几何计算尽量保持为可独立测试的纯函数。
- 位图矢量化采用精确颜色和四邻域语义；水平游程可单线程或分片并行，最终统一闭合、
  规范化和验证。
- DWG 通过隔离的 LibreDWG 外部工具适配，不把 GPL 库静态或动态链接进自研程序。

## 兼容性边界

当前重点是二维 CAD/CAM。三维建模、三维编辑和三维可视化明确不在当前范围。功能完成度
以 `s_autocad_gap_matrix.yaml` 和 `s_smartcad_feature_inventory.yaml` 为准；仅有入口或
占位界面的功能不得视为完成。
