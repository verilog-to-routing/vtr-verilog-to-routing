#!/usr/bin/env python3
"""Ensure a Qt6 >= the supported floor is available for VTR's GUI build, no root.

Qt 6.9.3 is the minimum required version: earlier Qt6 releases have internal
bugs in the QRhi subsystem that cause rendering failures on our targets.

Strategy:
  1. If the SYSTEM already provides Qt6 >= VTR_QT_VERSION (e.g. installed via
     apt), do nothing — the system Qt is used.
  2. Otherwise install a private copy via `aqt` (aqtinstall) into a
     user-writable, repo-local prefix (no sudo).

Invoked by the CI graphics jobs and the Dockerfile, after install_apt_packages.sh.

Configuration (environment variables):

  VTR_QT_PREFIX=/path/to/qt-install-root
      Install root. Default: "<repo>/qt6". aqt installs the SDK under
      "${VTR_QT_PREFIX}/<version>/gcc_64". The prefix (or its parent) must be
      writable by the current user — this script never uses sudo.

  VTR_QT_VERSION=6.9.3
      Minimum/target Qt version. Default 6.9.3 (the supported floor). A system
      Qt at or above this is accepted as-is; otherwise this exact version is
      provisioned via aqt.

Platform support: Linux only. On Windows (native win32 or MSYS2/Cygwin) the
script stops with instructions to install Qt6 manually — automatic provisioning
there is not implemented yet (see is_windows()/main()).

This is a 1:1 port of ensure_qt6_sdk.sh and is kept behaviourally identical.

Idempotent + self-healing: if a suitable system Qt is present it does nothing.
A pre-existing repo-local SDK is validated (key files present AND a tiny Qt
Widgets app builds, links, and runs against it); if that passes it does nothing,
otherwise the stale/partial SDK is removed and a clean copy is installed and
re-validated.
"""

import os
import re
import shutil
import subprocess
import sys
import tempfile
import venv

QT_VERSION = os.environ.get("VTR_QT_VERSION", "6.9.3")

# Repo root is the parent of this script's dev/ directory.
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.dirname(SCRIPT_DIR)

QT_PREFIX = os.environ.get("VTR_QT_PREFIX", os.path.join(REPO_ROOT, "qt6"))
QT_HOME = os.path.join(QT_PREFIX, QT_VERSION, "gcc_64")


def have(cmd):
    """True if an executable named cmd is on PATH (like `command -v`)."""
    return shutil.which(cmd) is not None


def is_windows():
    """True on any Windows Python: native win32, or MSYS2/Cygwin builds."""
    return os.name == "nt" or sys.platform.startswith(("win", "msys", "cygwin"))


def version_tuple(ver):
    """'6.9.3' -> (6, 9, 3); missing components default to 0."""
    parts = (ver.split(".") + ["0", "0", "0"])[:3]
    return tuple(int(p) for p in parts)


def version_ge(ver_a, ver_b):
    """True if version ver_a >= version ver_b."""
    return version_tuple(ver_a) >= version_tuple(ver_b)


# Detect a system Qt6 version (X.Y.Z) that CMake's find_package(Qt6) would
# actually use. We rely ONLY on the distro -devel package that ships the Qt6
# CMake config into a default find_package() search path: qt6-base-dev on
# Debian/Ubuntu (dpkg), qt6-qtbase-devel on Fedora/RHEL (rpm). We deliberately
# do NOT consult `qmake6` or `pkg-config`: a qmake6 on PATH (or a tools-only /
# non-default install) can report a Qt that find_package() cannot find, which
# would make us wrongly skip aqt and leave the build unable to locate Qt6.
def _query_version(cmd):
    """Run a package-query command; return its stdout, or '' on failure."""
    try:
        return subprocess.run(cmd, capture_output=True, text=True, check=False).stdout
    except OSError:
        return ""


def detect_system_qt6():
    """Return the system Qt6 version (X.Y.Z) find_package(Qt6) would use, or ''."""
    if have("dpkg-query"):
        # Debian/Ubuntu: qt6-base-dev installs the Qt6 CMake config under /usr.
        out = _query_version(["dpkg-query", "-W", "-f=${Version}", "qt6-base-dev"])
    elif have("rpm"):
        # Fedora/RHEL: qt6-qtbase-devel installs the Qt6 CMake config under /usr.
        out = _query_version(["rpm", "-q", "--qf", "%{VERSION}", "qt6-qtbase-devel"])
    else:
        return ""
    match = re.match(r"[0-9]+(\.[0-9]+){1,2}", out)
    return match.group(0) if match else ""


# Build + link + run a tiny Qt Widgets app against the given Qt home. Returns
# True on success, False if it fails to build, link, or run. Skipped (treated as
# OK) when no C++ toolchain is available, so a missing compiler does not trigger
# a reinstall.
def qt_smoke_test(qthome):
    """Build, link, and run a tiny Qt Widgets app against qthome; True on success."""
    if not have("make"):
        print("  ensure_qt6_sdk smoke test skipped ('make' not found)")
        return True

    # Exercise the same Qt modules the real VPR GUI (libezgl) links against —
    # Widgets, Xml, Svg, and the private QRhi API — not just Core/Gui/Widgets,
    # so a Qt missing any of them fails here rather than during the VPR build.
    # The required floor (e.g. "6.9.3") is baked straight into the source, and
    # the program compares the *runtime* Qt version (qVersion()) against it with
    # Qt's QVersionNumber, failing if it is older.
    smoke_cpp = """\
#include <QApplication>
#include <QLabel>
#include <QDomDocument>      // Qt6::Xml
#include <QSvgRenderer>      // Qt6::Svg
#include <QVersionNumber>
#include <QtGlobal>
#include <rhi/qrhi.h>        // Qt6::GuiPrivate (QRhi)
#include <memory>
// Exit codes (ensure_qt6_sdk.py maps these to a per-failure message):
//   0 = ok, 2 = runtime Qt below floor, 3 = Xml, 4 = Svg, 5 = QRhi.
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QLabel label(QStringLiteral("ok"));
    label.show();

    // Qt6::Xml — parse a tiny document.
    QDomDocument doc;
    doc.setContent(QByteArray("<root><n>1</n></root>"));
    if (doc.documentElement().tagName() != QLatin1String("root")) {
        qWarning("QtXml (QDomDocument) failed to parse");
        return 3;
    }

    // Qt6::Svg — parse a tiny SVG.
    QSvgRenderer svg(QByteArray(
        "<svg width='1' height='1'><rect width='1' height='1'/></svg>"));
    if (!svg.isValid()) {
        qWarning("QtSvg (QSvgRenderer) failed to parse SVG");
        return 4;
    }

    // Qt6::GuiPrivate — QRhi. The Null backend needs no GPU and works headless,
    // so it just verifies the private QRhi headers/libs link and initialize.
    QRhiNullInitParams rhi_params;
    std::unique_ptr<QRhi> rhi(QRhi::create(QRhi::Null, &rhi_params));
    if (!rhi) {
        qWarning("QRhi (Null backend) failed to initialize");
        return 5;
    }

    // Verify the runtime Qt meets the build floor.
    const QVersionNumber runtime = QVersionNumber::fromString(QString::fromLatin1(qVersion()));
    const QVersionNumber floor = QVersionNumber::fromString(QStringLiteral("__QT_VERSION__"));
    if (runtime < floor) {
        qWarning("runtime Qt %s is below required floor", qVersion());
        return 2;
    }
    return 0;
}
""".replace("__QT_VERSION__", QT_VERSION)
    smoke_pro = """\
QT += core gui gui-private widgets svg xml
CONFIG += console c++17
CONFIG -= app_bundle
TARGET = smoke
SOURCES += smoke.cpp
"""

    with tempfile.TemporaryDirectory() as tmp:
        with open(os.path.join(tmp, "smoke.cpp"), "w") as f:
            f.write(smoke_cpp)
        with open(os.path.join(tmp, "smoke.pro"), "w") as f:
            f.write(smoke_pro)

        built = (
            subprocess.run(
                [os.path.join(qthome, "bin", "qmake"), "smoke.pro"],
                cwd=tmp,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                check=False,
            ).returncode
            == 0
        )
        if built:
            built = (
                subprocess.run(
                    ["make"],
                    cwd=tmp,
                    stdout=subprocess.DEVNULL,
                    stderr=subprocess.DEVNULL,
                    check=False,
                ).returncode
                == 0
            )
        if not built:
            print("  ensure_qt6_sdk smoke test FAILED to build/link against {}".format(qthome))
            return False

        # Run headless: the offscreen platform plugin needs no display/X server.
        env = dict(os.environ)
        env["QT_QPA_PLATFORM"] = "offscreen"
        ld = os.path.join(qthome, "lib")
        if env.get("LD_LIBRARY_PATH"):
            ld = ld + ":" + env["LD_LIBRARY_PATH"]
        env["LD_LIBRARY_PATH"] = ld
        rc = subprocess.run(
            [os.path.join(tmp, "smoke")],
            env=env,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
        ).returncode

    # Distinct exit codes from smoke.cpp, so a failure says *what* is wrong with
    # the SDK rather than just "it failed". Keep in sync with the returns there.
    smoke_failures = {
        2: "the runtime Qt is below the {} floor".format(QT_VERSION),
        3: "the Qt Xml module (QDomDocument) is not usable",
        4: "the Qt Svg module (QSvgRenderer) is not usable",
        5: "the private Qt QRhi API (Qt6::GuiPrivate) is not usable",
    }
    if rc in smoke_failures:
        print("  ensure_qt6_sdk smoke test ran but {}".format(smoke_failures[rc]))
        return False
    if rc != 0:
        print("  ensure_qt6_sdk smoke test built but FAILED to run (exit {})".format(rc))
        return False
    return True


# Validate the Qt SDK at qthome: required Core/Gui/Widgets/Svg/Xml/ShaderTools
# libs, the Qt6 CMake config, and the offscreen plugin must exist, then the
# runtime smoke test.
def validate_qt_sdk(qthome):
    """True if qthome has the required Qt6 files and passes the runtime smoke test."""
    required = [
        "bin/qmake",
        "lib/libQt6Core.so",
        "lib/libQt6Gui.so",
        "lib/libQt6Widgets.so",
        "lib/libQt6Svg.so",
        "lib/libQt6Xml.so",
        "lib/libQt6ShaderTools.so",
        "lib/cmake/Qt6/Qt6Config.cmake",
        "plugins/platforms/libqoffscreen.so",
    ]
    for rel in required:
        if not os.path.exists(os.path.join(qthome, rel)):
            print("  ensure_qt6_sdk: missing {}".format(rel))
            return False
    return qt_smoke_test(qthome)


def main():
    """Ensure a suitable Qt6 is available; return a process exit code."""
    # -----------------------------------------------------------------------
    # Windows: not supported yet (placeholder). Everything below assumes a Linux
    # SDK (aqt linux_gcc_64, .so libraries, LD_LIBRARY_PATH, the offscreen
    # plugin). A Windows implementation would provision the win64_msvc/mingw SDK
    # with a different layout and must run under *native win32* Python — NOT the
    # Python bundled in an MSYS2/Cygwin environment. Until then, set up Qt6
    # manually and point CMAKE_PREFIX_PATH at it.
    # -----------------------------------------------------------------------
    if is_windows():
        print(
            "ensure_qt6_sdk: automatic Qt6 provisioning is not supported on " "Windows yet.",
            file=sys.stderr,
        )
        print(
            "Install Qt6 >= {} manually (e.g. the official Qt installer or "
            "aqt) and point CMAKE_PREFIX_PATH at it.".format(QT_VERSION),
            file=sys.stderr,
        )
        print(
            "Note: a future Windows implementation must use native win32 "
            "Python, not the Python inside an MSYS2/Cygwin environment.",
            file=sys.stderr,
        )
        return 1

    # -----------------------------------------------------------------------
    # Prefer a system Qt6 >= QT_VERSION: if one is installed, no aqt is needed.
    # -----------------------------------------------------------------------
    sys_qt_version = detect_system_qt6()
    if sys_qt_version and version_ge(sys_qt_version, QT_VERSION):
        print(
            "System Qt6 {} satisfies >= {} — using system Qt, skipping aqt.".format(
                sys_qt_version, QT_VERSION
            )
        )
        return 0
    print(
        "System Qt6 ({}) does not satisfy >= {}; provisioning via aqt...".format(
            sys_qt_version or "none found", QT_VERSION
        )
    )

    # -----------------------------------------------------------------------
    # Idempotency + self-heal: if a repo-local SDK is already present, validate
    # it. If valid, we are done. If it fails validation, remove the
    # stale/partial SDK (when non-empty) so a clean copy is installed below.
    # -----------------------------------------------------------------------
    qmake = os.path.join(QT_HOME, "bin", "qmake")
    if os.path.isfile(qmake) and os.access(qmake, os.X_OK):
        print("Found existing repo-local Qt at {}; validating...".format(QT_HOME))
        if validate_qt_sdk(QT_HOME):
            print("Repo-local Qt {} validated — nothing to do.".format(QT_VERSION))
            return 0
        print("Repo-local Qt at {} failed validation.".format(QT_HOME))
        if os.path.isdir(QT_HOME) and os.listdir(QT_HOME):
            print("Removing {} to install a clean Qt {}...".format(QT_HOME, QT_VERSION))
            shutil.rmtree(QT_HOME, ignore_errors=True)

    # -----------------------------------------------------------------------
    # Root-free precondition: the install root must be user-writable.
    # -----------------------------------------------------------------------
    try:
        os.makedirs(QT_PREFIX, exist_ok=True)
    except OSError:
        pass
    if not os.access(QT_PREFIX, os.W_OK):
        print(
            "ERROR: {} is not writable by {}.".format(
                QT_PREFIX, os.environ.get("USER", "the current user")
            ),
            file=sys.stderr,
        )
        print(
            "       This script does not use sudo. Choose a writable location via", file=sys.stderr
        )
        print("       VTR_QT_PREFIX=/some/writable/dir, or fix the permissions.", file=sys.stderr)
        return 1

    # -----------------------------------------------------------------------
    # aqtinstall in an isolated venv (kept beside the SDK, never on PATH so it
    # cannot collide with the project venv). The "*/gcc_64" CMake glob ignores it.
    # -----------------------------------------------------------------------
    # aqtinstall is pinned to 3.3.0 for reproducible Qt installs across machines.
    aqt_version = os.environ.get("VTR_AQT_VERSION", "3.3.0")
    aqt_venv = os.path.join(QT_PREFIX, "aqt-venv")
    aqt = os.path.join(aqt_venv, "bin", "aqt")
    pip = os.path.join(aqt_venv, "bin", "pip")
    if not (os.path.isfile(aqt) and os.access(aqt, os.X_OK)):
        print("Installing aqtinstall=={} into {}...".format(aqt_version, aqt_venv))
        venv.create(aqt_venv, with_pip=True)
        subprocess.run([pip, "install", "--upgrade", "pip"], check=True)
        subprocess.run([pip, "install", "aqtinstall=={}".format(aqt_version)], check=True)

    # -----------------------------------------------------------------------
    # Install Qt. qtshadertools is required by the QRhi shader pipeline.
    # -----------------------------------------------------------------------
    print("Installing Qt {} into {} via aqt (no sudo)...".format(QT_VERSION, QT_PREFIX))
    subprocess.run(
        [
            aqt,
            "install-qt",
            "linux",
            "desktop",
            QT_VERSION,
            "linux_gcc_64",
            "--outputdir",
            QT_PREFIX,
            "--modules",
            "qtshadertools",
        ],
        check=True,
    )

    # Verify the freshly installed SDK the same way (build + link + run).
    print("Validating freshly installed Qt {}...".format(QT_VERSION))
    if not validate_qt_sdk(QT_HOME):
        print(
            "ERROR: Qt {} at {} failed validation after install.".format(QT_VERSION, QT_HOME),
            file=sys.stderr,
        )
        return 1
    print("Qt {} installed and validated at {}".format(QT_VERSION, QT_HOME))
    return 0


if __name__ == "__main__":
    sys.exit(main())
