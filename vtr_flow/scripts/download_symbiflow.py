#!/usr/bin/env python3
import sys
import os
import argparse
import urllib.parse
import urllib.request, urllib.parse, urllib.error
import urllib.request, urllib.error, urllib.parse
import hashlib
import math
import textwrap
import tarfile
import fnmatch
import errno
import tempfile
import shutil
import glob
import subprocess


class DownloadError(Exception):
    pass


class ChecksumError(Exception):
    pass


class ExtractionError(Exception):
    pass


SYMBIFLOW_URL_MIRRORS = {
        "google": "https://storage.googleapis.com/symbiflow-arch-defs/artifacts/prod/foss-fpga-tools/symbiflow-arch-defs/presubmit/install/731/20200926-090152/symbiflow-arch-defs-install-e3de025d.tar.xz"
}


def parse_args():
    description = textwrap.dedent(
        """
                    Download and extract a symbiflow benchmark release into a
                    VTR-style directory structure.

                    If a previous matching symbiflow release tar.gz file is found
                    does nothing (unless --force is specified).
                  """
    )
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument(
        "--vtr_flow_dir",
        required=True,
        help="The 'vtr_flow' directory under the VTR tree. "
        "If specified this will extract the symbiflow release, "
        "placing benchmarks under vtr_flow/benchmarks/symbiflow ",
    )
    parser.add_argument(
        "--force",
        default=False,
        action="store_true",
        help="Run extraction step even if directores etc. already exist",
    )

    parser.add_argument(
        "--mirror", default="google", choices=["google"], help="Download mirror"
    )

    parser.add_argument(
        "--upgrade_archs",
        action='store_true',
        default=True,
        help="Try to upgrade included architecture files (using the upgrade_archs.py)",
    )

    return parser.parse_args()


def main():

    args = parse_args()

    try:
        tar_xz_url = SYMBIFLOW_URL_MIRRORS[args.mirror]

        tar_xz_filename = "symbiflow.tar.xz"

        print("Downloading {}".format(tar_xz_url))
        download_url(tar_xz_filename, tar_xz_url)

        print("Extracting {}".format(tar_xz_filename))
        extract_to_vtr_flow_dir(args, tar_xz_filename)

    except DownloadError as e:
        print("Failed to download:", e)
        sys.exit(1)
    except ExtractionError as e:
        print("Failed to extract symbiflow release:", e)
        sys.exit(2)

    sys.exit(0)


def download_url(filename, url):
    """
    Downloads the symbiflow release
    """
    urllib.request.urlretrieve(url, filename, reporthook=download_progress_callback)


def download_progress_callback(block_num, block_size, expected_size):
    """
    Callback for urllib.urlretrieve which prints a dot for every percent of a file downloaded
    """
    total_blocks = int(math.ceil(expected_size / block_size))
    progress_increment = int(math.ceil(total_blocks / 100))

    if block_num % progress_increment == 0:
        sys.stdout.write(".")
        sys.stdout.flush()
    if block_num * block_size >= expected_size:
        print("")

def force_symlink(file1, file2):
    try:
        os.symlink(file1, file2)
    except OSError as e:
        if e.errno == errno.EEXIST:
            os.remove(file2)
            os.symlink(file1, file2)


def extract_to_vtr_flow_dir(args, tar_xz_filename):
    """
    Extracts the 'benchmarks' directory of the symbiflow release
    into its corresponding vtr directory
    """

    # Reference directories
    arch_dir = os.path.join(args.vtr_flow_dir, "arch")
    symbiflow_arch_extract_dir = os.path.join(arch_dir, "symbiflow")
    symbiflow_test_dir = os.path.join(args.vtr_flow_dir, "tasks", "regression_tests", "vtr_reg_nightly", "symbiflow")

    arch_upgrade_script = os.path.join(
        os.path.dirname(os.path.realpath(__file__)), "upgrade_arch.py"
    )

    if not args.force:
        # Check that all expected directories exist
        expected_dirs = [
            args.vtr_flow_dir,
            symbiflow_arch_extract_dir,
        ]
        for dir in expected_dirs:
            if not os.path.isdir(dir):
                raise ExtractionError("{} should be a directory".format(dir))

    # Create a temporary working directory
    tmpdir = tempfile.mkdtemp(suffix="download_symbiflow", dir=".")

    # Extract matching files into the temporary directory
    subprocess.call("tar -C {} -xf {} share/symbiflow/arch/xc7a50t_test".format(tmpdir, tar_xz_filename), shell=True)

    # Move the extracted files to the relevant directories, SDC files first (since we
    # need to look up the BLIF name to make it match)
    for dirpath, dirnames, filenames in os.walk(tmpdir):
        for filename in filenames:
            src_file_path = os.path.join(dirpath, filename)
            dst_file_path = None

            if fnmatch.fnmatch(src_file_path, "*/xc7a50t_test/arch.timing.xml"):
                dst_file_path = os.path.join(
                    symbiflow_arch_extract_dir, "arch.timing.xml"
                )

                subprocess.call("{} {}".format(arch_upgrade_script, src_file_path), shell=True)

            elif fnmatch.fnmatch(src_file_path, "*/xc7a50t_test/*.bin"):
                dst_file_path = os.path.join(
                    symbiflow_test_dir, filename
                )

            if dst_file_path:
                shutil.move(src_file_path, dst_file_path)


    shutil.rmtree(tmpdir)

    print("Done")


def extract_callback(members):
    for tarinfo in members:
        if fnmatch.fnmatch(tarinfo.name, "*/*.xml"):
            print(tarinfo.name)
            yield tarinfo
        elif fnmatch.fnmatch(tarinfo.name, "*/*.bin"):
            print(tarinfo.name)
            yield tarinfo


if __name__ == "__main__":
    main()
