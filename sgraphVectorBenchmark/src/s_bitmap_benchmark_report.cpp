#include "s_bitmap_benchmark_report.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMap>
#include <QSysInfo>
#include <QTextStream>
#include <QThread>
#include <algorithm>
#include <cmath>

namespace smartCam
{
namespace
{

struct SAggregate
{
    int count = 0;
    int passed = 0;
    double flood_ms = 0.0;
    double serial_ms = 0.0;
    double parallel_ms = 0.0;
};

QString escapeHtml(QString value)
{
    return value.replace('&', QStringLiteral("&amp;"))
        .replace('<', QStringLiteral("&lt;"))
        .replace('>', QStringLiteral("&gt;"))
        .replace('"', QStringLiteral("&quot;"));
}

QString milliseconds(double value)
{
    return QString::number(value, 'f', value < 10.0 ? 3 : 2);
}

QString speedup(double baseline, double candidate)
{
    if (candidate <= 0.0)
    {
        return QStringLiteral("—");
    }
    return QString::number(baseline / candidate, 'f', 2) +
           QStringLiteral("×");
}

double percentile(std::vector<double> values, double fraction)
{
    if (values.empty())
    {
        return 0.0;
    }
    std::sort(values.begin(), values.end());
    const double position = fraction * (values.size() - 1);
    const std::size_t lower = static_cast<std::size_t>(std::floor(position));
    const std::size_t upper = static_cast<std::size_t>(std::ceil(position));
    const double weight = position - lower;
    return values[lower] * (1.0 - weight) + values[upper] * weight;
}

void writeSummaryCards(QTextStream& stream, int passed, int total,
                       double flood_ms, double serial_ms, double parallel_ms,
                       quint64 peak_bytes)
{
    stream << "<section class=\"cards\">"
           << "<div><b>" << passed << " / " << total
           << QStringLiteral("</b><span>正确性通过</span></div>")
           << "<div><b>" << milliseconds(flood_ms)
           << QStringLiteral(" ms</b><span>四邻域累计</span></div>")
           << "<div><b>" << milliseconds(serial_ms)
           << QStringLiteral(" ms</b><span>拆分单线程累计</span></div>")
           << "<div><b>" << milliseconds(parallel_ms)
           << QStringLiteral(" ms</b><span>拆分多线程累计</span></div>")
           << "<div><b>" << speedup(flood_ms, parallel_ms)
           << QStringLiteral("</b><span>多线程 / 四邻域</span></div>")
           << "<div><b>" << QString::number(
                  peak_bytes / 1024.0 / 1024.0, 'f', 1)
           << QStringLiteral(
                  " MB</b><span>进程峰值工作集</span></div></section>");
}

void writeCategoryTable(QTextStream& stream,
                        const QMap<QString, SAggregate>& categories)
{
    stream << QStringLiteral(
        "<h2>按图案类别</h2><table><thead><tr>"
        "<th>类别</th><th>文件数</th><th>通过</th>"
        "<th>四邻域 ms</th><th>拆分单线程 ms</th>"
        "<th>拆分多线程 ms</th><th>相对四邻域</th>"
        "</tr></thead><tbody>");
    for (auto iterator = categories.cbegin();
         iterator != categories.cend(); ++iterator)
    {
        const SAggregate& value = iterator.value();
        stream << "<tr><td>" << escapeHtml(iterator.key()) << "</td><td>"
               << value.count << "</td><td>" << value.passed << "</td><td>"
               << milliseconds(value.flood_ms) << "</td><td>"
               << milliseconds(value.serial_ms) << "</td><td>"
               << milliseconds(value.parallel_ms) << "</td><td>"
               << speedup(value.flood_ms, value.parallel_ms)
               << "</td></tr>";
    }
    stream << "</tbody></table>";
}

void writeWorstTable(QTextStream& stream, QString title,
                     std::vector<const SBitmapBenchmarkCaseResult*> values,
                     bool by_regression)
{
    std::sort(values.begin(), values.end(),
              [by_regression](const auto* first, const auto* second)
              {
                  if (by_regression)
                  {
                      const double first_speed =
                          first->flood_fill.milliseconds /
                          std::max(0.000001,
                                   first->run_parallel.milliseconds);
                      const double second_speed =
                          second->flood_fill.milliseconds /
                          std::max(0.000001,
                                   second->run_parallel.milliseconds);
                      return first_speed < second_speed;
                  }
                  return first->run_parallel.milliseconds >
                         second->run_parallel.milliseconds;
              });
    stream << "<h2>" << title
           << QStringLiteral(
                  "</h2><table><thead><tr>"
                  "<th>ID</th><th>类别</th><th>尺寸</th><th>四邻域 ms</th>"
                  "<th>拆分多线程 ms</th><th>加速比</th></tr></thead><tbody>");
    const std::size_t limit = std::min<std::size_t>(20, values.size());
    for (std::size_t index = 0; index < limit; ++index)
    {
        const auto& value = *values[index];
        stream << "<tr><td>" << value.test_case.id << "</td><td>"
               << escapeHtml(value.test_case.category) << "</td><td>"
               << value.test_case.width << QStringLiteral("×")
               << value.test_case.height
               << "</td><td>" << milliseconds(value.flood_fill.milliseconds)
               << "</td><td>"
               << milliseconds(value.run_parallel.milliseconds)
               << "</td><td>"
               << speedup(value.flood_fill.milliseconds,
                          value.run_parallel.milliseconds)
               << "</td></tr>";
    }
    stream << "</tbody></table>";
}

void writeAllRows(QTextStream& stream,
                  const std::vector<SBitmapBenchmarkCaseResult>& results)
{
    stream << QStringLiteral(
        "<h2>全部逐文件结果</h2>"
        "<table id=\"all\"><thead><tr>"
        "<th onclick=\"sortTable(0)\">ID</th>"
        "<th onclick=\"sortTable(1)\">类别</th>"
        "<th onclick=\"sortTable(2)\">尺寸</th>"
        "<th onclick=\"sortTable(3)\">颜色</th>"
        "<th onclick=\"sortTable(4)\">结果</th>"
        "<th onclick=\"sortTable(5)\">四邻域 ms</th>"
        "<th onclick=\"sortTable(6)\">拆分单线程 ms</th>"
        "<th onclick=\"sortTable(7)\">拆分多线程 ms</th>"
        "<th onclick=\"sortTable(8)\">加速比</th>"
        "<th>游程</th><th>线段</th><th>色块</th><th>轮廓</th>"
        "<th>SVG bytes</th><th>估算内存 MB</th><th>说明</th>"
        "</tr></thead><tbody>");
    for (const SBitmapBenchmarkCaseResult& value : results)
    {
        const SBitmapVectorMetrics& metrics = value.run_parallel.metrics;
        stream << "<tr class=\"" << (value.passed ? "pass" : "fail")
               << "\"><td>" << value.test_case.id << "</td><td>"
               << escapeHtml(value.test_case.category) << "</td><td>"
               << value.test_case.width << QStringLiteral("×")
               << value.test_case.height
               << "</td><td>" << value.test_case.color_count << "</td><td>"
               << (value.passed ? "PASS" : "FAIL") << "</td><td>"
               << milliseconds(value.flood_fill.milliseconds) << "</td><td>"
               << milliseconds(value.run_serial.milliseconds) << "</td><td>"
               << milliseconds(value.run_parallel.milliseconds) << "</td><td>"
               << speedup(value.flood_fill.milliseconds,
                          value.run_parallel.milliseconds)
               << "</td><td>" << metrics.run_count << "</td><td>"
               << metrics.segment_count << "</td><td>"
               << metrics.component_count << "</td><td>"
               << metrics.contour_count << "</td><td>"
               << metrics.svg_bytes << "</td><td>"
               << QString::number(metrics.estimated_working_bytes /
                                      1024.0 / 1024.0,
                                  'f', 2)
               << "</td><td>" << escapeHtml(value.validation_error)
               << "</td></tr>";
    }
    stream << "</tbody></table>";
}

} // namespace

SResult<QString> writeBitmapBenchmarkReport(
    const SBitmapBenchmarkOptions& options,
    const std::vector<SBitmapBenchmarkCaseResult>& results,
    quint64 peak_working_set_bytes)
{
    const QString report_path =
        QDir(options.output_directory)
            .filePath(QStringLiteral("bitmap_vector_benchmark_report.html"));
    QFile file(report_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return SResult<QString>::failure(
            QStringLiteral("无法写入 HTML 测试报告。"));
    }
    int passed = 0;
    double flood_ms = 0.0;
    double serial_ms = 0.0;
    double parallel_ms = 0.0;
    QMap<QString, SAggregate> categories;
    std::vector<double> speedups;
    std::vector<const SBitmapBenchmarkCaseResult*> valid_results;
    for (const SBitmapBenchmarkCaseResult& value : results)
    {
        passed += value.passed ? 1 : 0;
        flood_ms += value.flood_fill.milliseconds;
        serial_ms += value.run_serial.milliseconds;
        parallel_ms += value.run_parallel.milliseconds;
        SAggregate& aggregate = categories[value.test_case.category];
        ++aggregate.count;
        aggregate.passed += value.passed ? 1 : 0;
        aggregate.flood_ms += value.flood_fill.milliseconds;
        aggregate.serial_ms += value.run_serial.milliseconds;
        aggregate.parallel_ms += value.run_parallel.milliseconds;
        if (value.flood_fill.success && value.run_parallel.success)
        {
            speedups.push_back(
                value.flood_fill.milliseconds /
                std::max(0.000001, value.run_parallel.milliseconds));
            valid_results.push_back(&value);
        }
    }
    const bool all_passed = passed == static_cast<int>(results.size());
    const bool is_faster = parallel_ms < flood_ms;
    const QString conclusion =
        all_passed && is_faster
        ? QStringLiteral("全部正确性测试通过；拆分法自动多线程累计实测提升 %1。")
              .arg(speedup(flood_ms, parallel_ms))
        : QStringLiteral(
              "尚不能声明性能提升：%1，累计多线程耗时%2四邻域基线。")
              .arg(all_passed ? QStringLiteral("正确性通过")
                              : QStringLiteral("存在正确性失败"),
                   is_faster ? QStringLiteral("低于")
                             : QStringLiteral("不低于"));

    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    stream << QStringLiteral(
                  "<!doctype html><html lang=\"zh-CN\"><head><meta charset=\"utf-8\">"
                  "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
                  "<title>位图矢量化 ")
           << results.size()
           << QStringLiteral(" 文件基准报告</title><style>")
           << "body{font:14px system-ui;margin:0;background:#f4f6fa;color:#172033}"
           << "main{max-width:1500px;margin:auto;padding:28px}"
           << "h1{margin:0 0 6px}.meta{color:#627086}.verdict{padding:16px;"
           << "background:#e8f1ff;border-left:5px solid #2563eb;margin:20px 0}"
           << ".cards{display:grid;grid-template-columns:repeat(auto-fit,minmax(180px,1fr));"
           << "gap:12px}.cards div{background:white;padding:16px;border-radius:10px;"
           << "box-shadow:0 1px 4px #ccd3df}.cards b{font-size:21px;display:block}"
           << ".cards span{color:#627086}table{width:100%;border-collapse:collapse;"
           << "background:white;margin:10px 0 24px;font-size:12px}"
           << "th,td{padding:8px;border:1px solid #dce2eb;text-align:right}"
           << "th:first-child,td:first-child,th:nth-child(2),td:nth-child(2){text-align:left}"
           << "th{background:#e9eef7;cursor:pointer;position:sticky;top:0}"
           << "tr.fail{background:#ffe7e7}tr.pass:hover{background:#f0f6ff}"
           << "#all{display:block;overflow:auto;max-height:720px}"
           << QStringLiteral(
                  "</style></head><body><main><h1>位图矢量化批量正确性与性能报告</h1>"
                  "<p class=\"meta\">生成时间：")
           << QDateTime::currentDateTime().toString(
                  QStringLiteral("yyyy-MM-dd HH:mm:ss"))
           << QStringLiteral("｜Qt ") << QT_VERSION_STR
           << QStringLiteral("｜线程：")
           << (options.thread_count > 0 ? options.thread_count
                                       : QThread::idealThreadCount())
           << QStringLiteral("｜系统：")
           << escapeHtml(QSysInfo::prettyProductName())
           << QStringLiteral("｜固定种子：") << options.seed
           << QStringLiteral("｜每方案中位数次数：")
           << options.repetitions << "</p><div class=\"verdict\"><b>"
           << escapeHtml(conclusion)
           << QStringLiteral("</b><br>逐文件加速比 P10 / P50 / P90：")
           << QString::number(percentile(speedups, 0.10), 'f', 2)
           << QStringLiteral("× / ")
           << QString::number(percentile(speedups, 0.50), 'f', 2)
           << QStringLiteral("× / ")
           << QString::number(percentile(speedups, 0.90), 'f', 2)
           << QStringLiteral("×</div>");
    writeSummaryCards(stream, passed, static_cast<int>(results.size()),
                      flood_ms, serial_ms, parallel_ms,
                      peak_working_set_bytes);
    writeCategoryTable(stream, categories);
    writeWorstTable(stream, QStringLiteral("拆分多线程最慢的 20 个文件"),
                    valid_results, false);
    writeWorstTable(stream, QStringLiteral("相对四邻域退化最明显的 20 个文件"),
                    valid_results, true);
    writeAllRows(stream, results);
    stream << "<script>function sortTable(n){const t=document.getElementById('all'),"
           << "b=t.tBodies[0],r=[...b.rows],a=t.dataset.sort!=n;"
           << "r.sort((x,y)=>{let A=x.cells[n].innerText,B=y.cells[n].innerText,"
           << "an=parseFloat(A),bn=parseFloat(B),v=!isNaN(an)&&!isNaN(bn)?an-bn:"
           << "A.localeCompare(B,'zh-CN');return a?v:-v});r.forEach(x=>b.appendChild(x));"
           << "t.dataset.sort=a?n:''}</script></main></body></html>";
    return SResult<QString>::success(report_path);
}

} // namespace smartCam
