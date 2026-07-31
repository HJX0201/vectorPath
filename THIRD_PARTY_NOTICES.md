# 第三方组件与许可证

第三方源码和运行组件统一位于 `sgraphThirdParty`，其许可证优先于 vectorPath 自研
代码许可证。

| 组件 | 版本 | 用途 | 许可证 |
| --- | --- | --- | --- |
| Qt | 5.12.10 | GUI、OpenGL、并发、测试 | LGPLv3/GPLv3 或商业许可 |
| SARibbon | 2.9.0 | Ribbon 界面 | MIT |
| Qt Advanced Docking System | 4.4.1 | 停靠面板 | LGPL-2.1 |
| Clipper2 | 仓库内版本 | 多边形布尔与偏移 | Boost Software License 1.0 |
| GNU LibreDWG | 0.14 | 独立进程形式的 DWG 转换工具 | GPLv3+ |

仓库不包含 Qt SDK。用户必须从 Qt 官方渠道自行安装，构建时脚本自动查找，打包时
`windeployqt` 仅复制应用运行所需文件。

SARibbon、QtADS、Clipper2 和 LibreDWG 的完整许可证文本随各自目录保留。分发包含 Qt、
QtADS 或 LibreDWG 的二进制包时，发布者必须同时满足相应许可证的通知、可替换/反向调试、
源代码或书面要约等要求。尤其 LibreDWG 是 GPLv3+ 程序；本项目通过独立进程调用它，
不得删除其版权、许可证和来源信息。

本文件不是法律意见。商业发布前应由发布者完成依赖与分发方式的许可证审核。
