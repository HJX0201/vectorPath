#!/usr/bin/env python3
"""Build and run the smartGraphics bitmap vector benchmark."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import subprocess
import sys


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="运行位图矢量化批量基准")
    parser.add_argument("--bits", choices=("32", "64"), default="64")
    parser.add_argument("--cases", type=int, default=1000)
    parser.add_argument("--seed", type=int, default=20260727)
    parser.add_argument("--threads", default="0", help="0 表示自动")
    parser.add_argument("--repetitions", type=int, default=3)
    parser.add_argument("--jobs", default="auto")
    parser.add_argument("--qt-dir")
    parser.add_argument("--output")
    parser.add_argument("--clean", action="store_true")
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    root = Path(__file__).resolve().parents[1]
    wrapper = root / "sgraphBuildTools" / (
        f"s_build_{arguments.bits}_release.py"
    )
    build_command = [
        sys.executable,
        str(wrapper),
        "--jobs",
        arguments.jobs,
    ]
    if arguments.clean:
        build_command.append("--clean")
    if arguments.qt_dir:
        build_command.extend(("--qt-dir", arguments.qt_dir))
    subprocess.run(build_command, check=True, cwd=root)

    tools_directory = root / "sgraphBuildTools"
    sys.path.insert(0, str(tools_directory))
    from s_build_common import find_qt

    qt_directory = find_qt(arguments.qt_dir, arguments.bits, "Release")
    environment = os.environ.copy()
    environment["PATH"] = (
        f"{qt_directory / 'bin'}{os.pathsep}{environment.get('PATH', '')}"
    )
    executable = (
        root
        / "build"
        / arguments.bits
        / "Release"
        / "sgraphVectorBenchmark"
        / "smartBitmapVectorBenchmark.exe"
    )
    command = [
        str(executable),
        "--cases",
        str(max(1, arguments.cases)),
        "--seed",
        str(arguments.seed),
        "--threads",
        str(arguments.threads),
        "--repetitions",
        str(max(1, arguments.repetitions)),
    ]
    if arguments.output:
        command.extend(("--output", arguments.output))
    return subprocess.run(command, cwd=root, env=environment).returncode


if __name__ == "__main__":
    raise SystemExit(main())
