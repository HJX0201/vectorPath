# 位图矢量化正确性与性能基准

该目录只保存基准源码、运行入口和输出验证代码。样本按固定种子在本地生成，图片、
数据清单和 HTML 报告不纳入 Git。

- `src`：样本生成、flood fill 基线、水平游程算法调用、正确性验证和 HTML 报告生成代码。
- CI 中的基准步：运行 20 份样本的 smoke 并验收输出结构。
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

先用解决方案或 MSBuild 单独生成基准程序，再直接运行可执行文件：

```powershell
msbuild sgraphVectorBenchmark/vp_bitmap_benchmark.vcxproj /m /p:Configuration=Release /p:Platform=x64
$bench = "build/msbuild/64/Release/smartBitmapVectorBenchmark.exe"

# 完整基准
& $bench

# 5000 文件耐久基准
& $bench --cases 5000

# 小规模验证
& $bench --cases 20 --repetitions 1 --threads 2
```

本项目由原生 `vp_bitmap_benchmark.vcxproj` 构建，不属于解决方案的默认生成范围，要求本机
安装 Qt 5.12.10 与 Qt/MSBuild。程序输出位于
`build/msbuild/<位数>/Release/smartBitmapVectorBenchmark.exe`。仓库 CI 的桌面任务会额外
运行一次 20 份样本的 smoke 并校验输出目录只保留 manifest 与 HTML。
默认的 `results` 输出也只保存在本地；如用 `--output` 指定其他位置，应选择构建目录或
仓库外目录，避免提交生成产物。
