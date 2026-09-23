#!/usr/bin/env python3
"""Shared Windows build driver for vectorPath."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import zipfile


QT_VERSION = "5.12.10"
PRODUCT_NAME = "vectorPath"
PRODUCT_VERSION = "0.2.0-alpha.1"
RUNTIME_DIRECTORIES = (
    "iconengines",
    "imageformats",
    "libredwg",
    "platforms",
    "printsupport",
    "styles",
    "translations",
)


def repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def pe_architecture(path: Path) -> str | None:
    try:
        with path.open("rb") as stream:
            if stream.read(2) != b"MZ":
                return None
            stream.seek(0x3C)
            pe_offset = struct.unpack("<I", stream.read(4))[0]
            stream.seek(pe_offset)
            if stream.read(4) != b"PE\0\0":
                return None
            machine = struct.unpack("<H", stream.read(2))[0]
    except (OSError, struct.error):
        return None
    return {0x014C: "32", 0x8664: "64"}.get(machine, f"0x{machine:04x}")


def run_output(command: list[str], environment: dict[str, str] | None = None) -> str:
    completed = subprocess.run(
        command,
        check=True,
        capture_output=True,
        text=True,
        errors="replace",
        env=environment,
    )
    return completed.stdout.strip()


def expanded_qt_candidates(explicit: str | None, bits: str) -> list[Path]:
    raw_candidates: list[Path] = []
    if explicit:
        raw_candidates.append(Path(explicit))
    for variable in (f"SGRAPH_QT{bits}_DIR", "QTDIR"):
        value = os.environ.get(variable)
        if value:
            raw_candidates.append(Path(value))

    project_root = repo_root()
    raw_candidates.extend(
        (
            project_root / ".toolchain" / "Qt",
            Path("C:/Qt"),
            Path.home() / "Qt",
            Path(os.environ.get("LOCALAPPDATA", "")) / "Programs" / "Qt",
        )
    )
    qmake = shutil.which("qmake")
    if qmake:
        raw_candidates.append(Path(qmake).resolve().parents[1])

    kit_name = "msvc2017" if bits == "32" else "msvc2017_64"
    candidates: list[Path] = []
    for raw in raw_candidates:
        if not str(raw) or not raw.exists():
            continue
        direct = raw.resolve()
        candidates.append(direct)
        candidates.append(direct / QT_VERSION / kit_name)
        candidates.append(direct / f"Qt{QT_VERSION}" / QT_VERSION / kit_name)
        try:
            candidates.extend(direct.glob(f"*/{QT_VERSION}/{kit_name}"))
            candidates.extend(direct.glob(f"*/{kit_name}"))
        except OSError:
            pass

    unique: list[Path] = []
    seen: set[str] = set()
    for candidate in candidates:
        key = str(candidate).casefold()
        if key not in seen:
            seen.add(key)
            unique.append(candidate)
    return unique


def validate_qt(candidate: Path, bits: str, configuration: str) -> tuple[bool, str]:
    qmake = candidate / "bin" / "qmake.exe"
    headers = candidate / "include" / "QtCore" / "qglobal.h"
    core_name = "Qt5Cored.dll" if configuration == "Debug" else "Qt5Core.dll"
    core = candidate / "bin" / core_name
    deployer = candidate / "bin" / "windeployqt.exe"
    moc = candidate / "bin" / "moc.exe"
    rcc = candidate / "bin" / "rcc.exe"
    core_library = candidate / "lib" / ("Qt5Cored.lib" if configuration == "Debug" else "Qt5Core.lib")
    missing = [
        path.name for path in (qmake, headers, core, core_library, deployer, moc, rcc)
        if not path.exists()
    ]
    if missing:
        return False, f"缺少 {', '.join(missing)}"
    try:
        version = run_output([str(qmake), "-query", "QT_VERSION"])
    except (OSError, subprocess.CalledProcessError) as error:
        return False, f"qmake 无法运行：{error}"
    if version != QT_VERSION:
        return False, f"版本为 {version}，要求 {QT_VERSION}"
    architecture = pe_architecture(core)
    if architecture != bits:
        return False, f"{core_name} 为 {architecture} 位，要求 {bits} 位"
    return True, "匹配"


def find_qt(explicit: str | None, bits: str, configuration: str) -> Path:
    inspected: list[str] = []
    for candidate in expanded_qt_candidates(explicit, bits):
        valid, reason = validate_qt(candidate, bits, configuration)
        inspected.append(f"  {candidate}: {reason}")
        if valid:
            print(f"[Qt] {candidate}", flush=True)
            return candidate
    details = "\n".join(inspected) if inspected else "  未发现候选目录"
    variable = f"SGRAPH_QT{bits}_DIR"
    raise RuntimeError(
        f"找不到 Qt {QT_VERSION} {bits} 位 {configuration} 套件。\n"
        f"{details}\n"
        f"请安装 Qt 后设置 {variable}，或使用 --qt-dir。"
    )


def find_vcvarsall() -> Path:
    installer = Path(
        os.environ.get("ProgramFiles(x86)", "C:/Program Files (x86)")
    ) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
    if installer.exists():
        try:
            installation = run_output(
                [
                    str(installer),
                    "-latest",
                    "-products",
                    "*",
                    "-requires",
                    "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                    "-property",
                    "installationPath",
                ]
            )
            candidate = (
                Path(installation)
                / "VC"
                / "Auxiliary"
                / "Build"
                / "vcvarsall.bat"
            )
            if candidate.exists():
                return candidate
        except (OSError, subprocess.CalledProcessError):
            pass

    base = Path("C:/Program Files/Microsoft Visual Studio/2022")
    for edition in ("BuildTools", "Community", "Professional", "Enterprise"):
        candidate = base / edition / "VC" / "Auxiliary" / "Build" / "vcvarsall.bat"
        if candidate.exists():
            return candidate
    raise RuntimeError("找不到 Visual Studio 2022 C++ x86/x64 工具链。")


def visual_studio_environment(bits: str) -> dict[str, str]:
    vcvarsall = find_vcvarsall()
    argument = "x86" if bits == "32" else "amd64"
    command = (
        f'cmd.exe /d /s /c ""{vcvarsall}" {argument} >nul && set"'
    )
    completed = subprocess.run(
        command,
        capture_output=True,
        text=True,
        errors="replace",
    )
    if completed.returncode != 0:
        details = (completed.stderr or completed.stdout).strip()
        raise RuntimeError(
            f"Visual Studio 工具链初始化失败（{completed.returncode}）：{details}"
        )
    environment: dict[str, str] = {}
    for line in completed.stdout.splitlines():
        if "=" in line:
            name, value = line.split("=", 1)
            canonical = name.upper()
            if canonical not in environment:
                environment[canonical] = value
    return environment


def find_msbuild(environment: dict[str, str]) -> Path:
    available = shutil.which("MSBuild.exe", path=environment.get("PATH"))
    if available:
        return Path(available)
    installation = environment.get("VSINSTALLDIR")
    base = Path(installation) if installation else find_vcvarsall().parents[3]
    for relative in ("MSBuild/Current/Bin/amd64/MSBuild.exe", "MSBuild/Current/Bin/MSBuild.exe"):
        candidate = base / relative
        if candidate.is_file():
            return candidate
    raise RuntimeError("找不到 Visual Studio MSBuild，请安装 C++ 桌面开发工具。")


def find_qt_msbuild() -> Path:
    candidates = []
    explicit = os.environ.get("QtMsBuild") or os.environ.get("QTMSBUILD")
    if explicit:
        candidates.append(Path(explicit))
    local_app_data = os.environ.get("LOCALAPPDATA")
    if local_app_data:
        candidates.append(Path(local_app_data) / "QtMsBuild")
    required = ("qt_defaults.props", "Qt.props", "qt.targets")
    inspected = []
    for candidate in candidates:
        missing = [name for name in required if not (candidate / name).is_file()]
        if not missing:
            print(f"[QtMsBuild] {candidate}", flush=True)
            return candidate.resolve()
        inspected.append(f"  {candidate}: 缺少 {', '.join(missing)}")
    details = "\n".join(inspected) if inspected else "  未发现候选目录"
    raise RuntimeError(
        f"找不到完整的 Qt MSBuild 集成文件。\n{details}\n"
        "请安装 Qt Visual Studio Tools/Qt MSBuild，并设置 QtMsBuild 环境变量。"
    )


def build_output_directory(bits: str, configuration: str, core: bool = False) -> Path:
    directory = repo_root() / "build" / "msbuild"
    if core:
        directory /= "core"
    return directory / bits / configuration


def parallel_jobs(value: str) -> int:
    if value == "auto":
        return os.cpu_count() or 1
    try:
        jobs = int(value)
    except ValueError as error:
        raise RuntimeError("--jobs 必须为 auto 或正整数") from error
    if jobs < 1:
        raise RuntimeError("--jobs 必须为 auto 或正整数")
    return jobs


def executable_path(build_dir: Path) -> Path:
    return build_dir / f"{PRODUCT_NAME}.exe"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def runtime_files(build_dir: Path) -> list[Path]:
    files: list[Path] = []
    for path in build_dir.iterdir():
        if path.is_file() and (
            path.name == f"{PRODUCT_NAME}.exe"
            or path.suffix.lower() == ".dll"
            or path.name.lower().startswith("vc_redist.")
        ):
            files.append(path)
    for directory_name in RUNTIME_DIRECTORIES:
        directory = build_dir / directory_name
        if directory.exists():
            files.extend(path for path in directory.rglob("*") if path.is_file())
    return sorted(files)


def file_origin(relative: Path) -> str:
    first = relative.parts[0].casefold()
    if first == "libredwg":
        return "LibreDWG 0.14"
    if relative.name.casefold().startswith("qtadvanceddocking"):
        return "Qt Advanced Docking System 4.4.1"
    if relative.name.casefold().startswith("vc_redist"):
        return "Microsoft Visual C++ Runtime"
    if relative.name == f"{PRODUCT_NAME}.exe":
        return PRODUCT_NAME
    return f"Qt {QT_VERSION}"


def write_runtime_manifest(build_dir: Path, bits: str, configuration: str) -> Path:
    executable = executable_path(build_dir)
    if not executable.exists():
        raise RuntimeError(f"未生成 {executable}")
    if pe_architecture(executable) != bits:
        raise RuntimeError(f"{executable.name} 架构与 {bits} 位输出目录不匹配。")

    core_name = "Qt5Cored.dll" if configuration == "Debug" else "Qt5Core.dll"
    core = build_dir / core_name
    platform = build_dir / "platforms" / (
        "qwindowsd.dll" if configuration == "Debug" else "qwindows.dll"
    )
    docking_name = (
        "qtadvanceddocking-qt5d.dll"
        if configuration == "Debug"
        else "qtadvanceddocking-qt5.dll"
    )
    required = (core, platform, build_dir / docking_name)
    missing = [str(path.relative_to(build_dir)) for path in required if not path.exists()]
    if missing:
        raise RuntimeError(f"运行时部署不完整：{', '.join(missing)}")

    records = []
    for path in runtime_files(build_dir):
        relative = path.relative_to(build_dir)
        architecture = pe_architecture(path)
        if (
            architecture in {"32", "64"}
            and relative.parts[0].casefold() != "libredwg"
            and not relative.name.casefold().startswith("vc_redist.")
            and architecture != bits
        ):
            raise RuntimeError(f"错误架构文件：{relative} ({architecture} 位)")
        records.append(
            {
                "path": relative.as_posix(),
                "bytes": path.stat().st_size,
                "sha256": sha256(path),
                "architecture": architecture,
                "origin": file_origin(relative),
            }
        )
    manifest = build_dir / "runtime_manifest.json"
    manifest.write_text(
        json.dumps(
            {
                "product": PRODUCT_NAME,
                "version": PRODUCT_VERSION,
                "architecture": bits,
                "configuration": configuration,
                "qt_version": QT_VERSION,
                "files": records,
            },
            ensure_ascii=False,
            indent=2,
        ),
        encoding="utf-8",
    )
    return manifest


def create_package(build_dir: Path, bits: str, configuration: str) -> Path:
    destination = repo_root() / "dist"
    destination.mkdir(exist_ok=True)
    architecture = "x64" if bits == "64" else "x86"
    configuration_suffix = "" if configuration == "Release" else "-debug"
    archive = destination / (
        f"{PRODUCT_NAME}-{PRODUCT_VERSION}-windows-{architecture}{configuration_suffix}.zip"
    )
    manifest = build_dir / "runtime_manifest.json"
    files = runtime_files(build_dir) + [manifest]
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as output:
        for path in files:
            output.write(path, path.relative_to(build_dir).as_posix())
    checksum = archive.with_suffix(archive.suffix + ".sha256")
    checksum.write_text(f"{sha256(archive)}  {archive.name}\n", encoding="ascii")
    return archive


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="构建 vectorPath")
    parser.add_argument("--jobs", default="auto", help="MSBuild 项目并行数，默认 auto；每项目单编译进程")
    parser.add_argument("--qt-dir", help="显式指定 Qt 套件根目录")
    parser.add_argument("--clean", action="store_true", help="先删除对应输出目录")
    parser.add_argument("--run", action="store_true", help="构建完成后启动应用")
    parser.add_argument("--bits", choices=("32", "64"), default="64")
    parser.add_argument("--config", choices=("Debug", "Release"), default="Release")
    parser.add_argument("--core", action="store_true", help="仅构建无 Qt 核心")
    parser.add_argument("--test", action="store_true", help="构建并运行测试")
    parser.add_argument("--benchmarks", action="store_true", help="构建可选性能基准")
    parser.add_argument("--package", action="store_true", help="生成便携 ZIP")
    return parser.parse_args()


def run_build() -> int:
    arguments = parse_arguments()
    bits = arguments.bits
    configuration = arguments.config
    root = repo_root()
    jobs = parallel_jobs(arguments.jobs)
    if arguments.core and (arguments.run or arguments.package):
        raise RuntimeError("--core 不能与 --run 或 --package 同时使用")
    build_dir = build_output_directory(bits, configuration, arguments.core)
    resolved_build = build_dir.resolve()
    allowed_build_root = root.resolve() / "build" / "msbuild"
    if not resolved_build.is_relative_to(allowed_build_root) or resolved_build == allowed_build_root:
        raise RuntimeError("构建目录必须位于项目 build/msbuild 内")

    environment = visual_studio_environment(bits)
    msbuild = find_msbuild(environment)
    qt_dir = None
    properties = [
        f"/p:Configuration={configuration}",
        f"/p:Platform={'Win32' if bits == '32' else 'x64'}",
        f"/p:VpCoreOnly={'true' if arguments.core else 'false'}",
        "/p:BuildProjectReferences=true",
        "/p:VpCompilerProcesses=1",
    ]
    if not arguments.core:
        qt_dir = find_qt(arguments.qt_dir, bits, configuration)
        qt_msbuild = find_qt_msbuild()
        properties.extend((f"/p:VpQtDir={qt_dir}", f"/p:QtMsBuild={qt_msbuild}"))
        environment["PATH"] = f"{qt_dir / 'bin'};{environment.get('PATH', '')}"
    environment["PATH"] = f"{build_dir};{environment.get('PATH', '')}"

    if arguments.core:
        projects = [root / "sgraphGeometry" / "vp_geometry_core.vcxproj"]
        if arguments.test:
            projects = [root / "sgraphTests" / "vp_core_tests.vcxproj"]
    else:
        projects = [root / "vectorPath.sln"]
        if arguments.test:
            projects.extend((root / "sgraphTests" / "vp_core_tests.vcxproj",
                             root / "sgraphTests" / "vp_desktop_tests.vcxproj"))
    if arguments.benchmarks:
        projects.append(root / "sgraphTests" / "vp_id_collection_benchmark.vcxproj")
        if not arguments.core:
            projects.append(root / "sgraphVectorBenchmark" / "vp_bitmap_benchmark.vcxproj")
    missing = [str(project.relative_to(root)) for project in projects if not project.is_file()]
    if missing:
        raise RuntimeError(f"缺少原生 Visual Studio 工程：{', '.join(missing)}")
    if arguments.clean and build_dir.exists():
        shutil.rmtree(build_dir)
    build_dir.mkdir(parents=True, exist_ok=True)

    for project in projects:
        subprocess.run(
            [str(msbuild), str(project), "/nologo", "/t:Build", f"/m:{jobs}",
             "/verbosity:minimal", *properties],
            check=True, env=environment, cwd=root,
        )
    if arguments.test:
        from vp_test import run_tests
        run_tests(root, build_dir, environment, jobs,
                  desktop=not arguments.core, benchmarks=arguments.benchmarks, qt_dir=qt_dir)

    if arguments.core:
        print(f"[OK] Core: {build_dir}", flush=True)
        return 0

    manifest = write_runtime_manifest(build_dir, bits, configuration)
    print(f"[OK] {executable_path(build_dir)}", flush=True)
    print(f"[OK] {manifest}", flush=True)
    if arguments.package:
        print(f"[OK] {create_package(build_dir, bits, configuration)}", flush=True)
    if arguments.run:
        subprocess.Popen(
            [str(executable_path(build_dir))], cwd=build_dir, env=environment,
        )
    return 0


def main() -> None:
    try:
        raise SystemExit(run_build())
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"[FAIL] {error}", file=sys.stderr, flush=True)
        raise SystemExit(1)
