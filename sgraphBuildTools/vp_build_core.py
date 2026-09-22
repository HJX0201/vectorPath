#!/usr/bin/env python3
"""Build and test the extracted C++ core without discovering or loading Qt."""

import argparse
from pathlib import Path
import shutil
import subprocess

from vp_build_common import repo_root, visual_studio_environment


def main() -> None:
    parser = argparse.ArgumentParser(description="构建无 Qt 核心")
    parser.add_argument("--bits", choices=("32", "64"), default="64")
    parser.add_argument("--config", choices=("Debug", "Release"), default="Release")
    parser.add_argument("--jobs", type=int, default=4)
    arguments = parser.parse_args()
    if arguments.jobs < 1:
        parser.error("--jobs must be positive")
    root = repo_root()
    build_dir = root / "build" / "core" / arguments.bits / arguments.config
    environment = visual_studio_environment(arguments.bits)
    cmake = shutil.which("cmake", path=environment.get("PATH"))
    ninja = shutil.which("ninja", path=environment.get("PATH"))
    if not cmake or not ninja:
        raise RuntimeError("找不到 CMake 或 Ninja")
    ctest = str(Path(cmake).with_name("ctest.exe"))
    subprocess.run(
        [cmake, "-S", str(root), "-B", str(build_dir), "-G", "Ninja",
         f"-DCMAKE_MAKE_PROGRAM={ninja}", f"-DCMAKE_BUILD_TYPE={arguments.config}",
         "-DVECTORPATH_BUILD_DESKTOP=OFF", "-DVECTORPATH_BUILD_TESTS=ON",
         "-DCMAKE_DISABLE_FIND_PACKAGE_Qt5=ON"],
        check=True, env=environment,
    )
    subprocess.run(
        [cmake, "--build", str(build_dir), "--parallel", str(arguments.jobs)],
        check=True, env=environment,
    )
    subprocess.run(
        [ctest, "--test-dir", str(build_dir), "--output-on-failure"],
        check=True, env=environment,
    )


if __name__ == "__main__":
    main()
