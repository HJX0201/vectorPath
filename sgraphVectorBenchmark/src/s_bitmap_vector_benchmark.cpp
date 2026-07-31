#include "s_bitmap_benchmark_generator.h"
#include "s_bitmap_benchmark_report.h"
#include "s_bitmap_benchmark_validation.h"
#include "s_bitmap_flood_baseline.h"

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
#include <Windows.h>
#include <Psapi.h>
#endif

namespace vectorPath
{
namespace
{

struct SMeasuredResult
{
    SBitmapAlgorithmSummary summary;
    SBitmapVectorResult value;
};

SMeasuredResult measure(
    int repetitions,
    const std::function<SResult<SBitmapVectorResult>()>& operation)
{
    SMeasuredResult measured;
    std::vector<double> times;
    for (int repetition = 0; repetition < repetitions; ++repetition)
    {
        QElapsedTimer timer;
        timer.start();
        SResult<SBitmapVectorResult> result = operation();
        const double elapsed = timer.nsecsElapsed() / 1000000.0;
        if (!result)
        {
            measured.summary.error = result.errorMessage();
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

void saveFailureArtifacts(const SBitmapBenchmarkOptions& options,
                          const SBitmapBenchmarkCase& test_case,
                          const QImage& source,
                          const SMeasuredResult& flood_fill,
                          const SMeasuredResult& run_serial,
                          const SMeasuredResult& run_parallel,
                          const QImage& difference)
{
    QDir root(options.output_directory);
    const QString relative =
        QStringLiteral("failures/case_%1").arg(
            test_case.id, 4, 10, QLatin1Char('0'));
    root.mkpath(relative);
    QDir directory(root.filePath(relative));
    source.save(directory.filePath(QStringLiteral("input.png")));
    if (!difference.isNull())
    {
        difference.save(directory.filePath(QStringLiteral("difference.png")));
    }
    const auto write_svg = [&](QString name, const SMeasuredResult& result)
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
    if (GetProcessMemoryInfo(
            GetCurrentProcess(),
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
        QDir(QStringLiteral(SGRAPH_BENCHMARK_ROOT))
            .filePath(QStringLiteral("results")));
    results_directory.mkpath(QStringLiteral("."));
    const QString prefix =
        QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-"));
    int next_index = 1;
    const QStringList entries = results_directory.entryList(
        QDir::Dirs | QDir::NoDotAndDotDot);
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
    return results_directory.filePath(
        prefix + QString::number(next_index));
}

bool removeTemporaryDirectory(const QString& output_directory,
                              const QString& directory_name)
{
    QDir temporary_directory(
        QDir(output_directory).filePath(directory_name));
    return !temporary_directory.exists() ||
           temporary_directory.removeRecursively();
}

bool removeTemporaryArtifacts(const QString& output_directory)
{
    const bool cases_removed = removeTemporaryDirectory(
        output_directory, QStringLiteral("cases"));
    const bool failures_removed = removeTemporaryDirectory(
        output_directory, QStringLiteral("failures"));
    return cases_removed && failures_removed;
}

SBitmapBenchmarkOptions parseOptions(QCoreApplication& application)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("位图矢量化批量正确性与性能测试"));
    parser.addHelpOption();
    QCommandLineOption case_option(
        QStringList{QStringLiteral("cases")}, QStringLiteral("测试文件数量"),
        QStringLiteral("count"), QStringLiteral("1000"));
    QCommandLineOption seed_option(
        QStringList{QStringLiteral("seed")}, QStringLiteral("固定随机种子"),
        QStringLiteral("seed"), QStringLiteral("20260727"));
    QCommandLineOption thread_option(
        QStringList{QStringLiteral("threads")},
        QStringLiteral("拆分法线程数，0 为自动"),
        QStringLiteral("count"), QStringLiteral("0"));
    QCommandLineOption repetition_option(
        QStringList{QStringLiteral("repetitions")},
        QStringLiteral("每种算法重复次数"),
        QStringLiteral("count"), QStringLiteral("3"));
    QCommandLineOption output_option(
        QStringList{QStringLiteral("output")}, QStringLiteral("结果目录"),
        QStringLiteral("directory"));
    QCommandLineOption smoke_option(
        QStringList{QStringLiteral("smoke")},
        QStringLiteral("仅生成小尺寸冒烟测试图片"));
    parser.addOptions({case_option, seed_option, thread_option,
                       repetition_option, output_option, smoke_option});
    parser.process(application);

    SBitmapBenchmarkOptions options;
    options.case_count = std::max(1, parser.value(case_option).toInt());
    options.seed = parser.value(seed_option).toUInt();
    options.thread_count =
        std::max(0, parser.value(thread_option).toInt());
    options.repetitions =
        std::max(1, parser.value(repetition_option).toInt());
    options.smoke = parser.isSet(smoke_option);
    options.output_directory = parser.value(output_option);
    if (options.output_directory.isEmpty())
    {
        options.output_directory = nextResultDirectory();
    }
    return options;
}

} // namespace
} // namespace vectorPath

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    application.setApplicationName(
        QStringLiteral("smartBitmapVectorBenchmark"));
    const vectorPath::SBitmapBenchmarkOptions options =
        vectorPath::parseOptions(application);
    qInfo().noquote() << QStringLiteral("生成 %1 张测试图片：%2")
                             .arg(options.case_count)
                             .arg(options.output_directory);
    const auto generated = vectorPath::generateBitmapBenchmarkCases(options);
    if (!generated)
    {
        qCritical().noquote() << generated.errorMessage();
        return 1;
    }

    std::vector<vectorPath::SBitmapBenchmarkCaseResult> results;
    results.reserve(generated.value().size());
    int failure_count = 0;
    for (std::size_t index = 0; index < generated.value().size(); ++index)
    {
        const vectorPath::SBitmapBenchmarkCase& test_case =
            generated.value()[index];
        const QImage source(test_case.file_path);
        vectorPath::SBitmapBenchmarkCaseResult case_result;
        case_result.test_case = test_case;
        if (source.isNull())
        {
            case_result.validation_error =
                QStringLiteral("无法重新读取生成的 PNG 文件。");
            results.push_back(std::move(case_result));
            ++failure_count;
            continue;
        }
        vectorPath::SBitmapVectorSettings flood_settings;
        flood_settings.ignore_background = false;
        vectorPath::SBitmapVectorSettings serial_settings = flood_settings;
        serial_settings.worker_count = 1;
        vectorPath::SBitmapVectorSettings parallel_settings = flood_settings;
        parallel_settings.worker_count = options.thread_count;

        std::array<vectorPath::SMeasuredResult, 3> measured;
        std::array<int, 3> order{0, 1, 2};
        std::rotate(order.begin(),
                    order.begin() + static_cast<int>(index % 3),
                    order.end());
        for (int algorithm : order)
        {
            if (algorithm == 0)
            {
                measured[0] = vectorPath::measure(
                    options.repetitions,
                    [&]()
                    {
                        return vectorPath::bitmapToVectorFloodFill(
                            source, flood_settings);
                    });
            }
            else if (algorithm == 1)
            {
                measured[1] = vectorPath::measure(
                    options.repetitions,
                    [&]()
                    {
                        return vectorPath::bitmapToVectorResult(
                            source, serial_settings);
                    });
            }
            else
            {
                measured[2] = vectorPath::measure(
                    options.repetitions,
                    [&]()
                    {
                        return vectorPath::bitmapToVectorResult(
                            source, parallel_settings);
                    });
            }
        }
        case_result.flood_fill = measured[0].summary;
        case_result.run_serial = measured[1].summary;
        case_result.run_parallel = measured[2].summary;
        if (measured[0].summary.success &&
            measured[1].summary.success &&
            measured[2].summary.success)
        {
            const vectorPath::SBitmapValidationResult validation =
                vectorPath::validateBitmapBenchmarkCase(
                    source, measured[0].value,
                    measured[1].value, measured[2].value);
            case_result.passed = validation.passed;
            case_result.validation_error = validation.error;
            case_result.flood_fill.contour_hash = validation.flood_hash;
            case_result.run_serial.contour_hash = validation.serial_hash;
            case_result.run_parallel.contour_hash = validation.parallel_hash;
            if (!validation.passed)
            {
                vectorPath::saveFailureArtifacts(
                    options, test_case, source, measured[0], measured[1],
                    measured[2], validation.difference);
            }
        }
        else
        {
            case_result.validation_error =
                measured[0].summary.error + measured[1].summary.error +
                measured[2].summary.error;
            vectorPath::saveFailureArtifacts(
                options, test_case, source, measured[0], measured[1],
                measured[2], {});
        }
        if (!case_result.passed)
        {
            ++failure_count;
        }
        results.push_back(std::move(case_result));
        if ((index + 1) % 10 == 0 || index + 1 == generated.value().size())
        {
            qInfo().noquote()
                << QStringLiteral("进度 %1/%2，失败 %3")
                       .arg(index + 1)
                       .arg(generated.value().size())
                       .arg(failure_count);
        }
    }

    const auto report = vectorPath::writeBitmapBenchmarkReport(
        options, results, vectorPath::peakWorkingSet());
    if (!report)
    {
        qCritical().noquote() << report.errorMessage();
        return 1;
    }
    if (!vectorPath::removeTemporaryArtifacts(options.output_directory))
    {
        qCritical().noquote()
            << QStringLiteral("无法清理本次测试生成的临时文件。");
        return 3;
    }
    qInfo().noquote() << QStringLiteral("报告：%1").arg(report.value());
    qInfo().noquote() << QStringLiteral("通过：%1/%2")
                             .arg(results.size() - failure_count)
                             .arg(results.size());
    return failure_count == 0 ? 0 : 2;
}
