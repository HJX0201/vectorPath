# 位图矢量化正确性与性能基准

该目录只保存基准源码、运行入口和输出验证代码。样本按固定种子在本地生成，图片、
数据清单和 HTML 报告不纳入 Git。

- `src`：样本生成、flood fill 基线、水平游程算法调用、正确性验证和 HTML 报告生成代码。
- `../sgraphBuildTools/vp_test.py`：运行 smoke 并验收成功输出结构。
- `results`：运行时自动创建的本地输出目录，已加入 `.gitignore`。

所有样本均由代码生成；“合成图案变体”也不依赖外部图片。仓库原有固定图片及两批
历史结果已移除。历史测量使用的“真实样例变体”输入与当前生成器不同，不能用当前
结果直接替代旧表格中的测量值。

每次运行自动选择当天尚未使用的序号，例如 `results/20260728-1`。运行期间，测试 PNG
临时保存在本次结果目录的 `cases` 子目录；成功生成报告后自动删除，最终只保留：

- `manifest.json`：固定种子、尺寸、类别和文件名等可复现数据。
- `bitmap_vector_benchmark_report.html`：正确性与性能报告。

如果运行中断，`cases` 会保留。使用 `--output` 指向同一目录即可复用尺寸匹配的有效 PNG，
并从缺失或损坏文件处继续。正常流程生成报告后会清理 `cases` 和 `failures`，无论报告中的
正确性结果是否全部通过。

运行 64 位 Release 完整基准：

```powershell
python sgraphVectorBenchmark/vp_run_benchmark.py
```

5000 文件耐久基准：

```powershell
python sgraphVectorBenchmark/vp_run_benchmark.py --cases 5000
```

小规模验证：

```powershell
python sgraphVectorBenchmark/vp_run_benchmark.py --cases 20 --repetitions 1 --threads 2
```

脚本内部调用统一入口 `sgraphBuildTools/vp_build.py --benchmarks`，再运行所选规模的
位图基准；直接通过 MSBuild 编译原生 `vp_bitmap_benchmark.vcxproj`，要求本机安装
Qt 5.12.10、Qt/MSBuild 与 Python 3.10+，支持 `--qt-dir`、`--bits 32|64` 和 `--jobs`。
程序输出位于 `build/msbuild/<位数>/Release/smartBitmapVectorBenchmark.exe`。
默认应用构建不生成本项目。若只需普通开发验证与小型位图检查，可在仓库根目录执行：

```powershell
python sgraphBuildTools/vp_build.py --test --benchmarks
```

这会在 38 项普通套件之外运行 smoke 和输出布局两项，检查输出保存在已忽略的构建目录。
默认的 `results` 输出也只保存在本地；如用 `--output` 指定其他位置，应选择构建目录或
仓库外目录，避免提交生成产物。
