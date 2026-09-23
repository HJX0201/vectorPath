#!/usr/bin/env python3
"""Run the existing vectorPath suites without a CMake/CTest dependency."""

from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time


def list_suites(executable: Path, environment: dict[str, str]) -> list[str]:
    completed = subprocess.run(
        [str(executable), "--list-suites"], check=True, capture_output=True,
        text=True, encoding="utf-8", errors="replace", env=environment,
        cwd=executable.parent, timeout=30,
        creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0),
    )
    suites = [line.strip() for line in completed.stdout.splitlines() if line.strip()]
    if not suites or len(suites) != len(set(suites)):
        raise RuntimeError(f"{executable.name} 返回空或重复的测试列表")
    return suites


def desktop_suite_modes(root: Path) -> dict[str, str]:
    registry = root / "sgraphTests" / "vp_desktop_test_registry.inc"
    contents = registry.read_text(encoding="utf-8-sig")
    entries = re.findall(
        r"^\s*VECTORPATH_DESKTOP_TEST\(\s*(\w+)\s*,\s*\w+\s*,\s*(\w+)\s*\)",
        contents, flags=re.MULTILINE,
    )
    modes = dict(entries)
    if not modes or len(modes) != len(entries):
        raise RuntimeError(f"测试注册表为空或包含重复项：{registry}")
    if any(mode not in {"AppLess", "Core", "Gui", "Widgets"} for mode in modes.values()):
        raise RuntimeError(f"测试注册表包含未知应用模式：{registry}")
    return modes


def run_case(name: str, command: list[str], directory: Path,
             environment: dict[str, str]) -> tuple[str, bool, float, str]:
    started = time.perf_counter()
    try:
        completed = subprocess.run(
            command, cwd=directory, env=environment, capture_output=True,
            text=True, encoding="utf-8", errors="replace", timeout=300,
            creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0),
        )
        output = completed.stdout + completed.stderr
        return name, completed.returncode == 0, time.perf_counter() - started, output
    except (OSError, subprocess.TimeoutExpired) as error:
        return name, False, time.perf_counter() - started, str(error)


def verify_benchmark_layout(output: Path) -> None:
    expected = {"manifest.json", "bitmap_vector_benchmark_report.html"}
    actual = {path.name for path in output.iterdir()} if output.is_dir() else set()
    if actual != expected:
        raise RuntimeError(f"基准输出应只保留清单和报告，实际为：{sorted(actual)}")
    if any(not (output / name).is_file() for name in expected):
        raise RuntimeError("基准清单或报告不是文件")
    manifest = json.loads((output / "manifest.json").read_text(encoding="utf-8"))
    if manifest.get("case_count") != 20 or len(manifest.get("cases", [])) != 20:
        raise RuntimeError("位图冒烟测试清单未包含 20 个样本")


def run_tests(root: Path, build_dir: Path, environment: dict[str, str], jobs: int,
              *, desktop: bool, benchmarks: bool, qt_dir: Path | None = None) -> None:
    environment = environment.copy()
    environment["PATH"] = f"{build_dir}{os.pathsep}{environment.get('PATH', '')}"
    if desktop:
        if qt_dir is None:
            raise RuntimeError("桌面测试必须指定 Qt SDK 目录以定位离屏平台插件")
        platform_plugins = qt_dir / "plugins" / "platforms"
        if not platform_plugins.is_dir():
            raise RuntimeError(f"Qt SDK 平台插件目录不存在：{platform_plugins}")
        environment["QT_QPA_PLATFORM_PLUGIN_PATH"] = str(platform_plugins)
        environment["VECTORPATH_LIBREDWG_DIR"] = str(
            root / "sgraphThirdParty" / "libredwg-0.14-win64")
    core_runner = build_dir / "vectorPathCoreTests.exe"
    tasks = [(name, [str(core_runner), name], environment)
             for name in list_suites(core_runner, environment)]
    if desktop:
        desktop_runner = build_dir / "vectorPathDesktopTests.exe"
        suites = list_suites(desktop_runner, environment)
        modes = desktop_suite_modes(root)
        if set(suites) != set(modes):
            raise RuntimeError("桌面测试程序与受控注册表的套件列表不一致，请重新构建")
        for name in suites:
            suite_environment = environment.copy()
            if modes[name] == "Widgets":
                suite_environment["QT_QPA_PLATFORM"] = "offscreen"
            tasks.append((name, [str(desktop_runner), name], suite_environment))
    failures = []
    passed = 0
    total = len(tasks)
    with ThreadPoolExecutor(max_workers=jobs) as executor:
        pending = [executor.submit(run_case, name, command, build_dir, env)
                   for name, command, env in tasks]
        for future in as_completed(pending):
            name, success, elapsed, output = future.result()
            print(f"[{'PASS' if success else 'FAIL'}] {name} ({elapsed:.2f}s)", flush=True)
            if success:
                passed += 1
            else:
                failures.append(name)
                print(output.rstrip(), flush=True)
    if desktop and benchmarks:
        output = build_dir / "bitmap_benchmark_smoke"
        command = [str(build_dir / "smartBitmapVectorBenchmark.exe"),
                   "--cases", "20", "--seed", "20260727", "--threads", "2",
                   "--repetitions", "1", "--smoke", "--output", str(output)]
        name, success, elapsed, transcript = run_case(
            "vectorPathBitmapVectorBenchmarkSmoke", command, build_dir, environment)
        total += 2
        print(f"[{'PASS' if success else 'FAIL'}] {name} ({elapsed:.2f}s)", flush=True)
        if success:
            passed += 1
        else:
            failures.append(name)
            print(transcript.rstrip(), flush=True)
        layout_name = "vectorPathBitmapBenchmarkOutputLayout"
        try:
            if not success:
                raise RuntimeError("位图冒烟测试失败，不能验收输出布局")
            verify_benchmark_layout(output)
            passed += 1
            print(f"[PASS] {layout_name}", flush=True)
        except (OSError, ValueError, RuntimeError) as error:
            failures.append(layout_name)
            print(f"[FAIL] {layout_name}: {error}", flush=True)
    print(f"[Tests] {passed}/{total} passed", flush=True)
    if failures:
        raise RuntimeError(f"测试失败：{', '.join(failures)}")


def main() -> int:
    from vp_build_common import build_output_directory, find_qt, parallel_jobs, repo_root
    parser = argparse.ArgumentParser(description="运行已经构建的 vectorPath 测试")
    parser.add_argument("--bits", choices=("32", "64"), default="64")
    parser.add_argument("--config", choices=("Debug", "Release"), default="Release")
    parser.add_argument("--core", action="store_true")
    parser.add_argument("--benchmarks", action="store_true")
    parser.add_argument("--jobs", default="auto", help="同时运行的独立测试进程数")
    parser.add_argument("--qt-dir")
    arguments = parser.parse_args()
    environment = os.environ.copy()
    qt_dir = None
    if not arguments.core:
        qt_dir = find_qt(arguments.qt_dir, arguments.bits, arguments.config)
        environment["PATH"] = f"{qt_dir / 'bin'}{os.pathsep}{environment.get('PATH', '')}"
    run_tests(repo_root(), build_output_directory(arguments.bits, arguments.config, arguments.core),
              environment, parallel_jobs(arguments.jobs),
              desktop=not arguments.core, benchmarks=arguments.benchmarks, qt_dir=qt_dir)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, subprocess.CalledProcessError, subprocess.TimeoutExpired) as error:
        print(f"[FAIL] {error}", file=sys.stderr, flush=True)
        raise SystemExit(1)
