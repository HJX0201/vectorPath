# 位图矢量化正确性与性能基准

该目录独立保存基准源码、固定样例、正式结果和运行入口，不与日常 CAD 测试数据混放。

- `src`：生成器、flood fill 基线、水平游程算法调用、验证和 HTML 报告。
- `fixtures`：生成“真实样例变体”时使用的小型固定图片。
- `tests`：成功运行输出结构的自动验收。
- `results`：按 `yyyyMMdd-N` 归档的正式结果。

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
位图基准；同样要求本机安装 Qt 5.12.10，并支持 `--qt-dir`、`--bits 32|64` 和 `--jobs`。
默认应用构建不生成本项目。若只需普通开发验证与小型位图检查，可在仓库根目录执行：

```powershell
python sgraphBuildTools/vp_build.py --test --benchmarks
```

这会在 38 项普通套件之外运行 smoke 和输出布局两项；正式历史数据和报告不会被覆盖。

已验收的历史结果及测试口径见 [`results/README.md`](results/README.md)。
