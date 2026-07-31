#!/usr/bin/env python3
"""Shared Windows build driver for smartCam."""

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
PRODUCT_NAME = "smartCam"
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
    qt_config = candidate / "lib" / "cmake" / "Qt5" / "Qt5Config.cmake"
    core_name = "Qt5Cored.dll" if configuration == "Debug" else "Qt5Core.dll"
    core = candidate / "bin" / core_name
    deployer = candidate / "bin" / "windeployqt.exe"
    missing = [path.name for path in (qmake, qt_config, core, deployer) if not path.exists()]
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
            print(f"[Qt] {candidate}")
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
    parser = argparse.ArgumentParser(description="构建 smartCam")
    parser.add_argument("--jobs", default="auto", help="并行数，默认 auto")
    parser.add_argument("--qt-dir", help="显式指定 Qt 套件根目录")
    parser.add_argument("--clean", action="store_true", help="先删除对应输出目录")
    parser.add_argument("--run", action="store_true", help="构建完成后启动应用")
    parser.add_argument("--no-test", action="store_true", help="不构建和运行测试")
    parser.add_argument("--package", action="store_true", help="生成便携 ZIP")
    return parser.parse_args()


def run_build(bits: str, configuration: str) -> int:
    arguments = parse_arguments()
    root = repo_root()
    build_dir = root / "build" / bits / configuration
    if arguments.clean and build_dir.exists():
        shutil.rmtree(build_dir)
    build_dir.mkdir(parents=True, exist_ok=True)

    qt_dir = find_qt(arguments.qt_dir, bits, configuration)
    environment = visual_studio_environment(bits)
    environment["PATH"] = f"{qt_dir / 'bin'};{environment.get('PATH', '')}"

    cmake = shutil.which("cmake", path=environment.get("PATH"))
    ninja = shutil.which("ninja", path=environment.get("PATH"))
    if not cmake or not ninja:
        raise RuntimeError("找不到 CMake 或 Ninja，请先安装完整编译工具链。")

    tests = "OFF" if arguments.no_test else "ON"
    subprocess.run(
        [
            cmake,
            "-S",
            str(root),
            "-B",
            str(build_dir),
            "-G",
            "Ninja",
            f"-DCMAKE_MAKE_PROGRAM={ninja}",
            f"-DCMAKE_BUILD_TYPE={configuration}",
            f"-DCMAKE_PREFIX_PATH={qt_dir}",
            f"-DSMARTCAM_BUILD_TESTS={tests}",
        ],
        check=True,
        env=environment,
    )
    jobs = str(os.cpu_count() or 1) if arguments.jobs == "auto" else arguments.jobs
    subprocess.run(
        [cmake, "--build", str(build_dir), "--parallel", jobs],
        check=True,
        env=environment,
    )
    if not arguments.no_test:
        subprocess.run(
            ["ctest", "--test-dir", str(build_dir), "--output-on-failure", "-j", jobs],
            check=True,
            env=environment,
        )

    manifest = write_runtime_manifest(build_dir, bits, configuration)
    print(f"[OK] {executable_path(build_dir)}")
    print(f"[OK] {manifest}")
    if arguments.package:
        print(f"[OK] {create_package(build_dir, bits, configuration)}")
    if arguments.run:
        subprocess.Popen(
            [str(executable_path(build_dir))],
            cwd=build_dir,
            env=environment,
        )
    return 0


def main(bits: str, configuration: str) -> None:
    try:
        raise SystemExit(run_build(bits, configuration))
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"[FAIL] {error}", file=sys.stderr)
        raise SystemExit(1)
