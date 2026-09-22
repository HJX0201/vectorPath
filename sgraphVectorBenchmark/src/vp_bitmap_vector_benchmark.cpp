#include "vp_bitmap_benchmark_generator.h"
#include "vp_bitmap_benchmark_report.h"
#include "vp_bitmap_benchmark_validation.h"
#include "vp_bitmap_flood_baseline.h"
#include "vp_qt_text.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <algorithm>
#include <array>
#include <functional>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
// Psapi requires Windows declarations; keep this platform include order.
// clang-format off
#include <Windows.h>
#include <Psapi.h>
// clang-format on
#endif

namespace Vp
{
namespace
{

struct VpMeasuredResult
{
    VpBitmapAlgorithmSummary summary;
    VpBitmapVectorResult value;
};

VpMeasuredResult measure(int repetitions,
                         const std::function<VpResult<VpBitmapVectorResult>()>& operation)
{
    VpMeasuredResult measured;
    std::vector<double> times;
    for (int repetition = 0; repetition < repetitions; ++repetition)
    {
        QElapsedTimer timer;
        timer.start();
        VpResult<VpBitmapVectorResult> result = operation();
        const double elapsed = timer.nsecsElapsed() / 1000000.0;
        if (!result)
        {
            measured.summary.error = Vp::toQtError(result);
            return measured;
        }
        times.push_back(elapsed);
        if (repetition == 0)
        {
            measured.value = std::move(result.value());
        }
    }
    std::sort(times.begin(), times.end());
    measured.summary.success = true;
    measured.summary.milliseconds = times[times.size() / 2];
    measured.summary.metrics = measured.value.metrics;
    return measured;
}

void saveFailureArtifacts(const VpBitmapBenchmarkOptions& options,
                          const VpBitmapBenchmarkCase& test_case, const QImage& source,
                          const VpMeasuredResult& flood_fill, const VpMeasuredResult& run_serial,
                          const VpMeasuredResult& run_parallel, const QImage& difference)
{
    QDir root(options.output_directory);
    const QString relative =
        QStringLiteral("failures/case_%1").arg(test_case.id, 4, 10, QLatin1Char('0'));
    root.mkpath(relative);
    QDir directory(root.filePath(relative));
    source.save(directory.filePath(QStringLiteral("input.png")));
    if (!difference.isNull())
    {
        difference.save(directory.filePath(QStringLiteral("difference.png")));
    }
    const auto write_svg = [&](QString name, const VpMeasuredResult& result)
    {
        if (!result.summary.success)
        {
            return;
        }
        QFile file(directory.filePath(name));
        if (file.open(QIODevice::WriteOnly))
        {
            file.write(result.value.svg_data);
        }
    };
    write_svg(QStringLiteral("flood_fill.svg"), flood_fill);
    write_svg(QStringLiteral("run_serial.svg"), run_serial);
    write_svg(QStringLiteral("run_parallel.svg"), run_parallel);
}

quint64 peakWorkingSet()
{
#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS_EX counters{};
    if (GetProcessMemoryInfo(GetCurrentProcess(),
                             reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
                             sizeof(counters)))
    {
        return static_cast<quint64>(counters.PeakWorkingSetSize);
    }
#endif
    return 0;
}

QString nextResultDirectory()
{
    QDir results_directory(
        QDir(QStringLiteral(SGRAPH_BENCHMARK_ROOT)).filePath(QStringLiteral("results")));
    results_directory.mkpath(QStringLiteral("."));
    const QString prefix = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-"));
    int next_index = 1;
    const QStringList entries = results_directory.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& entry : entries)
    {
        if (!entry.startsWith(prefix))
        {
            continue;
        }
        bool is_valid_index = false;
        const int index = entry.mid(prefix.size()).toInt(&is_valid_index);
        if (is_valid_index)
        {
            next_index = std::max(next_index, index + 1);
        }
    }
    return results_directory.filePath(prefix + QString::number(next_index));
}

bool removeTemporaryDirectory(const QString& output_directory, const QString& directory_name)
{
    QDir temporary_directory(QDir(output_directory).filePath(directory_name));
    return !temporary_directory.exists() || temporary_directory.removeRecursively();
}

bool removeTemporaryArtifacts(const QString& output_directory)
{
    const bool cases_removed = removeTemporaryDirectory(output_directory, QStringLiteral("cases"));
    const bool failures_removed =
        removeTemporaryDirectory(output_directory, QStringLiteral("failures"));
    return cases_removed && failures_removed;
}

VpBitmapBenchmarkOptions parseOptions(QCoreApplication& application)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("位图矢量化批量正确性与性能测试"));
    parser.addHelpOption();
    QCommandLineOption case_option(QStringList{QStringLiteral("cases")},
                                   QStringLiteral("测试文件数量"), QStringLiteral("count"),
                                   QStringLiteral("1000"));
    QCommandLineOption seed_option(QStringList{QStringLiteral("seed")},
                                   QStringLiteral("固定随机种子"), QStringLiteral("seed"),
                                   QStringLiteral("20260727"));
    QCommandLineOption thread_option(QStringList{QStringLiteral("threads")},
                                     QStringLiteral("拆分法线程数，0 为自动"),
                                     QStringLiteral("count"), QStringLiteral("0"));
    QCommandLineOption repetition_option(QStringList{QStringLiteral("repetitions")},
                                         QStringLiteral("每种算法重复次数"),
                                         QStringLiteral("count"), QStringLiteral("3"));
    QCommandLineOption output_option(QStringList{QStringLiteral("output")},
                                     QStringLiteral("结果目录"), QStringLiteral("directory"));
    QCommandLineOption smoke_option(QStringList{QStringLiteral("smoke")},
                                    QStringLiteral("仅生成小尺寸冒烟测试图片"));
    parser.addOptions(
        {case_option, seed_option, thread_option, repetition_option, output_option, smoke_option});
    parser.process(application);

    VpBitmapBenchmarkOptions options;
    options.case_count = std::max(1, parser.value(case_option).toInt());
    options.seed = parser.value(seed_option).toUInt();
    options.thread_count = std::max(0, parser.value(thread_option).toInt());
    options.repetitions = std::max(1, parser.value(repetition_option).toInt());
    options.smoke = parser.isSet(smoke_option);
    options.output_directory = parser.value(output_option);
    if (options.output_directory.isEmpty())
    {
        options.output_directory = nextResultDirectory();
    }
    return options;
}

} // namespace
} // namespace Vp

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("smartBitmapVectorBenchmark"));
    const Vp::VpBitmapBenchmarkOptions options = Vp::parseOptions(application);
    qInfo().noquote() << QStringLiteral("生成 %1 张测试图片：%2")
                             .arg(options.case_count)
                             .arg(options.output_directory);
    const auto generated = Vp::generateBitmapBenchmarkCases(options);
    if (!generated)
    {
        qCritical().noquote() << Vp::toQtError(generated);
        return 1;
    }

    std::vector<Vp::VpBitmapBenchmarkCaseResult> results;
    results.reserve(generated.value().size());
    int failure_count = 0;
    for (std::size_t index = 0; index < generated.value().size(); ++index)
    {
        const Vp::VpBitmapBenchmarkCase& test_case = generated.value()[index];
        const QImage source(test_case.file_path);
        Vp::VpBitmapBenchmarkCaseResult case_result;
        case_result.test_case = test_case;
        if (source.isNull())
        {
            case_result.validation_error = QStringLiteral("无法重新读取生成的 PNG 文件。");
            results.push_back(std::move(case_result));
            ++failure_count;
            continue;
        }
        Vp::VpBitmapVectorSettings flood_settings;
        flood_settings.ignore_background = false;
        Vp::VpBitmapVectorSettings serial_settings = flood_settings;
        serial_settings.worker_count = 1;
        Vp::VpBitmapVectorSettings parallel_settings = flood_settings;
        parallel_settings.worker_count = options.thread_count;

        std::array<Vp::VpMeasuredResult, 3> measured;
        std::array<int, 3> order{0, 1, 2};
        std::rotate(order.begin(), order.begin() + static_cast<int>(index % 3), order.end());
        for (int algorithm : order)
        {
            if (algorithm == 0)
            {
                measured[0] =
                    Vp::measure(options.repetitions,
                                [&]()
                                {
                                    return Vp::bitmapToVectorFloodFill(source, flood_settings);
                                });
            }
            else if (algorithm == 1)
            {
                measured[1] =
                    Vp::measure(options.repetitions,
                                [&]()
                                {
                                    return Vp::bitmapToVectorResult(source, serial_settings);
                                });
            }
            else
            {
                measured[2] =
                    Vp::measure(options.repetitions,
                                [&]()
                                {
                                    return Vp::bitmapToVectorResult(source, parallel_settings);
                                });
            }
        }
        case_result.flood_fill = measured[0].summary;
        case_result.run_serial = measured[1].summary;
        case_result.run_parallel = measured[2].summary;
        if (measured[0].summary.success && measured[1].summary.success &&
            measured[2].summary.success)
        {
            const Vp::VpBitmapValidationResult validation = Vp::validateBitmapBenchmarkCase(
                source, measured[0].value, measured[1].value, measured[2].value);
            case_result.passed = validation.passed;
            case_result.validation_error = validation.error;
            case_result.flood_fill.contour_hash = validation.flood_hash;
            case_result.run_serial.contour_hash = validation.serial_hash;
            case_result.run_parallel.contour_hash = validation.parallel_hash;
            if (!validation.passed)
            {
                Vp::saveFailureArtifacts(options, test_case, source, measured[0], measured[1],
                                         measured[2], validation.difference);
            }
        }
        else
        {
            case_result.validation_error =
                measured[0].summary.error + measured[1].summary.error + measured[2].summary.error;
            Vp::saveFailureArtifacts(options, test_case, source, measured[0], measured[1],
                                     measured[2], {});
        }
        if (!case_result.passed)
        {
            ++failure_count;
        }
        results.push_back(std::move(case_result));
        if ((index + 1) % 10 == 0 || index + 1 == generated.value().size())
        {
            qInfo().noquote() << QStringLiteral("进度 %1/%2，失败 %3")
                                     .arg(index + 1)
                                     .arg(generated.value().size())
                                     .arg(failure_count);
        }
    }

    const auto report = Vp::writeBitmapBenchmarkReport(options, results, Vp::peakWorkingSet());
    if (!report)
    {
        qCritical().noquote() << Vp::toQtError(report);
        return 1;
    }
    if (!Vp::removeTemporaryArtifacts(options.output_directory))
    {
        qCritical().noquote() << QStringLiteral("无法清理本次测试生成的临时文件。");
        return 3;
    }
    qInfo().noquote() << QStringLiteral("报告：%1").arg(report.value());
    qInfo().noquote()
        << QStringLiteral("通过：%1/%2").arg(results.size() - failure_count).arg(results.size());
    return failure_count == 0 ? 0 : 2;
}
