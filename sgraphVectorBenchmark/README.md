# 位图矢量化正确性与性能基准

该目录独立保存基准源码、固定输入、正式结果和运行入口，不与日常 CAD 测试数据混放。

- `src`：生成器、flood fill 基线、水平游程算法调用、验证和 HTML 报告。
- `data`：固定基准图片。
- `results`：1000 与 5000 文件正式 HTML 报告和 `manifest.json`。
- `generated`：本机新生成的 PNG、失败附件和报告；已由 `.gitignore` 排除。

生成器会复用输出目录中名称和尺寸均匹配的有效 PNG，可在长时间基准中断后从缺失文件处
继续生成。

运行 64 位 Release 完整基准：

```powershell
python sgraphVectorBenchmark/s_run_benchmark.py
```

5000 文件耐久基准：

```powershell
python sgraphVectorBenchmark/s_run_benchmark.py --cases 5000 `
    --output sgraphVectorBenchmark/generated/5000-case-20260727
```

小规模验证：

```powershell
python sgraphVectorBenchmark/s_run_benchmark.py --cases 20 --repetitions 1 --threads 2
```

脚本先调用项目标准构建入口，因此同样要求本机安装 Qt 5.12.10，并支持
`--qt-dir`、`--bits 32|64` 和 `--jobs`。
