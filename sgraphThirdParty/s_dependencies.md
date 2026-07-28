# Third-party dependency manifest

| Dependency | Version | License | Purpose |
|---|---:|---|---|
| SARibbon | 2.9.0 | MIT | Ribbon user interface |
| Qt Advanced Docking System | 4.4.1 | LGPL-2.1 | Dockable tool panels |
| Qt | 5.12.10 | LGPL/GPL/commercial | Application framework |
| Clipper2 | 2.0.1 | Boost Software License 1.0 | 2D closed-polyline boolean operations |
| GNU LibreDWG Windows x64 | 0.14 | GPL-3.0-or-later | Isolated DWG-to-DXF and DXF-to-R2000-DWG conversion tools |

第三方组件统一保存在本目录中，并保留原始许可证文本。Qt SDK 不进入仓库，由构建脚本
从使用者环境自动查找。LibreDWG 以外部命令行适配器形式分发，不链接到
smartGraphics 进程。Poppler 仍是计划中的适配器。

Clipper2 2.0.1 is vendored from the official release and compiled as a static third-party target:

- Source: `https://github.com/AngusJohnson/Clipper2`
- Tag: `Clipper2_2.0.1`
- Archive: `AngusJohnson-Clipper2-Clipper2_2.0.1.tar.gz`
- SHA-512:
  `ce753ae3752b7516a9e0cb23c9788d9533c204819d5451ee50cf3b69a06e24165fa9f5270781764c036ad597eef83cf80532a5bebce5c96f5230179dc3ed499a`
- Runtime source directory: `sgraphThirdParty/Clipper2`
- License text: `sgraphThirdParty/Clipper2/LICENSE`
- Integration boundary: `sgraphGeometry/s_polygon_boolean.cpp`; Clipper2 types do not appear in
  smartGraphics public document or GUI interfaces.

LibreDWG 0.14 is fixed to the official Windows x64 release asset:

- Source: `https://github.com/LibreDWG/libredwg/releases/tag/0.14`
- Archive: `libredwg-0.14-win64.zip`
- SHA-256: `1ad7e15344d20b3426c3435b078d82fb84b35062815946b2cca9c5fc9810fea8`
- Runtime directory: `sgraphThirdParty/libredwg-0.14-win64`
- License text: `sgraphThirdParty/libredwg-0.14-win64/COPYING`
- Integration boundary: `sgraphIo/s_dwg_codec.cpp` and `sgraphIo/s_dwg_dxf_adapter.cpp`; only
  versioned temporary R2000 DXF files cross the process boundary, and no LibreDWG headers or ABI
  types appear in smartGraphics modules.
