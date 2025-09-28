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


class DownloadError(Exception):
    pass


class ChecksumError(Exception):
    pass


class ExtractionError(Exception):
    pass


TITAN_URL_MIRRORS = {"eecg": "https://www.eecg.utoronto.ca/~vaughn/titan/"}


def parse_args():
    description = textwrap.dedent(
        """
                    Download and extract a Titan benchmark release into a
                    VTR-style directory structure.

                    If a previous matching titan release tar.gz file is found
                    does nothing (unless --force is specified).
                  """
    )
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter, description=description
    )

    parser.add_argument(
        "--titan_version", default="2.1.0", help="Titan release version to download"
    )
    parser.add_argument(
        "--vtr_flow_dir",
        required=True,
        help="The 'vtr_flow' directory under the VTR tree. "
        "If specified this will extract the titan release, "
        "placing benchmarks under vtr_flow/benchmarks/titan ",
    )
    parser.add_argument(
        "--force",
        default=False,
        action="store_true",
        help="Run extraction step even if directores etc. already exist",
    )
    parser.add_argument(
        "--device_family",
        choices=["stratixiv", "stratix10", "all"],
        default="all",
        help="Select the device family for which to download the netlists and sdc files. Default: all",
    )

    parser.add_argument(
        "--full_archive",
        default=False,
        action="store_true",
        help="Download the full archive instead of just downloading the blif archive",
    )

    return parser.parse_args()


def main():

    args = parse_args()

    try:
        if args.full_archive:
            tar_gz_filename = "titan_release_" + args.titan_version + ".tar.gz"
        else:
            tar_gz_filename = "titan_release_" + args.titan_version + "_blif.tar.gz"

        tar_gz_url = urllib.parse.urljoin(TITAN_URL_MIRRORS["eecg"], tar_gz_filename)

        if not args.force and args.full_archive and os.path.isfile(tar_gz_filename):
            print("Found existing {} (skipping download and extraction)".format(tar_gz_filename))
        else:
            print("Downloading {}".format(tar_gz_url))
            download_url(tar_gz_filename, tar_gz_url)

            print("Extracting {}".format(tar_gz_filename))
            extract_to_vtr_flow_dir(args, tar_gz_filename)

    except DownloadError as e:
        print("Failed to download:", e)
        sys.exit(1)
    except ExtractionError as e:
        print("Failed to extract titan release:", e)
        sys.exit(3)

    sys.exit(0)


def download_url(filename, url):
    """
    Downloads the titan release
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


def extract_to_vtr_flow_dir(args, tar_gz_filename):
    """
    Extracts the 'benchmarks' directory of the titan release
    into its corresponding vtr directory
    """

    # Note that in previous VTR releases, arch files also need to be extracted from the titan release
    # into its corresponding VTR directory. This is no longer needed as VTR is now packed with the
    # newest Titan arch files.

    # Reference directories
    arch_dir = os.path.join(args.vtr_flow_dir, "arch")
    benchmarks_dir = os.path.join(args.vtr_flow_dir, "benchmarks")
    titan_benchmarks_extract_dir = os.path.join(benchmarks_dir, "titan_blif")
    titan_arch_extract_dir = os.path.join(arch_dir, "titan")

    if not args.force:
        # Check that all expected directories exist
        expected_dirs = [
            args.vtr_flow_dir,
            benchmarks_dir,
            arch_dir,
            titan_benchmarks_extract_dir,
            titan_arch_extract_dir,
        ]
        for dir in expected_dirs:
            if not os.path.isdir(dir):
                raise ExtractionError("{} should be a directory".format(dir))

    # Create a temporary working directory
    tmpdir = tempfile.mkdtemp(suffix="download_titan", dir=os.path.abspath("."))
    try:
        if not args.full_archive:
            # Extract the contents of the .tar.gz archive directly into the destination directory
            with tarfile.open(tar_gz_filename, "r:gz") as tar:
                members = [
                    m
                    for m in tar.getmembers()
                    if m.name.startswith(f"titan_release_{args.titan_version}_blif/")
                ]
                tar.extractall(tmpdir, members=members)
            tmp_source_blif_dir = os.path.join(tmpdir, f"titan_release_{args.titan_version}_blif")
            for root, dirs, files in os.walk(tmp_source_blif_dir):
                for file in files:
                    source_file = os.path.join(root, file)
                    relative_path = os.path.relpath(source_file, tmp_source_blif_dir)
                    destination_file = os.path.join(titan_benchmarks_extract_dir, relative_path)
                    os.makedirs(os.path.dirname(destination_file), exist_ok=True)
                    shutil.copy2(source_file, destination_file)

        else:
            create_titan_blif_subdirs(titan_benchmarks_extract_dir, args)

            # We use a callback along with extractall() to only walk through the large tar.gz file
            # once (saving runtime). We then move the files to the correct directories, which should
            # be a cheap rename operation

            # Extract matching files into the temporary directory
            with tarfile.TarFile.open(tar_gz_filename, mode="r|*") as tar_file:
                tar_file.extractall(path=tmpdir, members=extract_callback(tar_file, args))

            # Move the extracted files to the relevant directories, SDC files first (since we
            # need to look up the BLIF name to make it match)
            for file_type_prefix in ["*.sdc", "*.blif"]:
                for dirpath, dirnames, filenames in os.walk(tmpdir):
                    for filename in filenames:
                        src_file_path = os.path.join(dirpath, filename)
                        dst_file_path = None
                        for benchmark_subdir in get_benchmark_subdirs(args):
                            if compare_versions(args.titan_version, "2") >= 1:
                                # if it is a 2.0.0 titan release or later use device family in the benchmark directory
                                device_families = get_device_families(args)
                                for family in device_families:
                                    if file_type_prefix == "*.sdc":
                                        dest_file_name = determine_sdc_name(dirpath)
                                    else:
                                        dest_file_name = filename

                                    if dest_file_name == "":
                                        continue

                                    if fnmatch.fnmatch(
                                        src_file_path,
                                        f"*/titan_release*/benchmarks/{benchmark_subdir}/*/{family}/{file_type_prefix}",
                                    ):
                                        dst_file_path = os.path.join(
                                            titan_benchmarks_extract_dir,
                                            benchmark_subdir,
                                            family,
                                            dest_file_name,
                                        )

                            elif fnmatch.fnmatch(
                                src_file_path,
                                f"*/titan_release*/benchmarks/{benchmark_subdir}/*/{file_type_prefix}",
                            ):
                                # if it is a titan release earlier than device family in not used the benchmark directory as there is only support for SIV family
                                if file_type_prefix == "*.sdc":
                                    dest_file_name = determine_sdc_name(dirpath)
                                else:
                                    dest_file_name = filename

                                if dest_file_name == "":
                                    continue

                                dst_file_path = os.path.join(
                                    titan_benchmarks_extract_dir, benchmark_subdir, dest_file_name
                                )

                        if dst_file_path:
                            shutil.move(src_file_path, dst_file_path)

    finally:
        # Clean-up
        shutil.rmtree(tmpdir)

    print("Done")

    # Create the subdirectories under titan_blif


def create_titan_blif_subdirs(titan_benchmarks_extract_dir, args):
    for benchmark_subdir in get_benchmark_subdirs(args):
        titan_benchmark_subdir = os.path.join(titan_benchmarks_extract_dir, benchmark_subdir)
        if os.path.exists(titan_benchmark_subdir):
            shutil.rmtree(titan_benchmark_subdir)
        os.makedirs(titan_benchmark_subdir)
        if compare_versions(args.titan_version, "2") >= 1:
            for family in get_device_families(args):
                titan_benchmark_family_subdir = os.path.join(titan_benchmark_subdir, family)
                if not os.path.exists(titan_benchmark_family_subdir):
                    os.makedirs(titan_benchmark_family_subdir)


def determine_sdc_name(dirpath):
    """
    To have VTR pick up the SDC name, we need to match the corresponding BLIF file name
    """
    benchmark_dir = os.path.dirname(dirpath)  # Benchmark dir
    pattern = os.path.join(benchmark_dir, "*/*.blif")
    blif_paths = glob.glob(pattern)
    if len(blif_paths) < 1:
        print("No BLIF files found in:", benchmark_dir)
        print("Pattern is :", pattern)
        return ""

    blif_path = blif_paths[0]

    blif_file = os.path.basename(blif_path)
    blif_name, ext = os.path.splitext(blif_file)

    return blif_name + ".sdc"


def extract_callback(members, args):
    for tarinfo in members:
        for benchmark_subdir in get_benchmark_subdirs(args):

            if compare_versions(args.titan_version, "2") >= 1:
                # if it is a 2.0.0 titan release or later use device family in the benchmark directory
                device_families = get_device_families(args)
                for family in device_families:
                    if fnmatch.fnmatch(
                        tarinfo.name,
                        f"titan_release*/benchmarks/{benchmark_subdir}/*/{family}/*/*.blif",
                    ):
                        print(tarinfo.name)
                        yield tarinfo
                    elif fnmatch.fnmatch(
                        tarinfo.name,
                        f"titan_release*/benchmarks/{benchmark_subdir}/*/{family}/sdc/vpr.sdc",
                    ):
                        print(tarinfo.name)
                        yield tarinfo
            else:
                # if it is a titan release earlier than device family in not used the benchmark directory as there is only support for SIV family
                if fnmatch.fnmatch(
                    tarinfo.name, f"titan_release*/benchmarks/{benchmark_subdir}/*/*.blif"
                ):
                    print(tarinfo.name)
                    yield tarinfo
                elif fnmatch.fnmatch(
                    tarinfo.name, f"titan_release*/benchmarks/{benchmark_subdir}/*/sdc/vpr.sdc"
                ):
                    print(tarinfo.name)
                    yield tarinfo

        if fnmatch.fnmatch(tarinfo.name, "titan_release*/arch/stratixiv*.xml"):
            print(tarinfo.name)
            yield tarinfo

def get_benchmark_subdirs(args):
    """
    Decide which benchmark subdirectories to use depending on version
    """
    if compare_versions(args.titan_version, "2.1.0") >= 1:
        # version is 2.1.0 or higher
        return ["titanium", "titan23", "other_benchmarks"]
    else:
        return ["titan_new", "titan23", "other_benchmarks"]

def compare_versions(version1, version2):
    """
    Compares two release versions to see which once is more recent
    """
    v1_components = list(map(int, version1.split(".")))
    v2_components = list(map(int, version2.split(".")))

    for v1, v2 in zip(v1_components, v2_components):
        if v1 < v2:
            return -1  # version1 is lower
        elif v1 > v2:
            return 1  # version1 is higher

    # If all components are equal up to this point, check the length of the components.
    if len(v1_components) < len(v2_components):
        return -1  # version1 is lower (e.g., 1.2.3 vs 1.2.3.1)
    elif len(v1_components) > len(v2_components):
        return 1  # version1 is higher (e.g., 1.2.3.1 vs 1.2.3)

    return 0  # Both versions are equal


def get_device_families(args):
    if args.device_family == "all":
        return ["stratixiv", "stratix10"]
    else:
        return [args.device_family]


if __name__ == "__main__":
    main()
