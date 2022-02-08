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


TITAN_URL_MIRRORS = {
    "eecg": "http://www.eecg.utoronto.ca/~kmurray/titan/",
    "google": "https://storage.googleapis.com/verilog-to-routing/titan/",
}


def parse_args():
    description = textwrap.dedent(
        """
                    Download and extract a Titan benchmark release into a
                    VTR-style directory structure.

                    If a previous matching titan release tar.gz file is found
                    does nothing (unless --force is specified).
                  """
    )
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument(
        "--titan_version", default="1.3.1", help="Titan release version to download"
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
        "--mirror", default="google", choices=["eecg", "google"], help="Download mirror"
    )

    parser.add_argument(
        "--upgrade_archs",
        default=True,
        help="Try to upgrade included architecture files (using the upgrade_archs.py)",
    )

    return parser.parse_args()


def main():

    args = parse_args()

    try:
        tar_gz_filename = "titan_release_" + args.titan_version + ".tar.gz"
        md5_filename = "titan_release_" + args.titan_version + ".md5"

        tar_gz_url = urllib.parse.urljoin(TITAN_URL_MIRRORS[args.mirror], tar_gz_filename)
        md5_url = urllib.parse.urljoin(TITAN_URL_MIRRORS[args.mirror], md5_filename)

        # Requires a .decode() here to convert from bytes to a string
        external_md5 = load_md5_from_url(md5_url).decode()

        file_matches = False
        if os.path.isfile(tar_gz_filename):
            file_matches = md5_matches(tar_gz_filename, external_md5)

        if not args.force and file_matches:
            print(
                "Found existing {} with matching checksum (skipping download and extraction)".format(
                    tar_gz_filename
                )
            )
        else:
            if os.path.isfile(tar_gz_filename) and not file_matches:
                print("Local file MD5 does not match remote MD5")

            print("Downloading {}".format(tar_gz_url))
            download_url(tar_gz_filename, tar_gz_url)

            print("Verifying {}".format(tar_gz_url))
            if not md5_matches(tar_gz_filename, external_md5):
                raise ChecksumError(tar_gz_filename)

            print("Extracting {}".format(tar_gz_filename))
            extract_to_vtr_flow_dir(args, tar_gz_filename)

    except DownloadError as e:
        print("Failed to download:", e)
        sys.exit(1)
    except ChecksumError as e:
        print("File corrupt:", e)
        sys.exit(2)
    except ExtractionError as e:
        print("Failed to extract titan release:", e)
        sys.exit(3)

    sys.exit(0)


def download_url(filename, url):
    """
    Downloads the titan release
    """
    urllib.request.urlretrieve(url, filename, reporthook=download_progress_callback)


def verify_titan(tar_gz_filename, md5_url):
    """
    Performs checksum verification of downloaded titan release file
    """

    if filename != tar_gz_filename:
        raise VerificationError(
            "External MD5 appears to be for a different file. Was {} expected {}".format(
                filename, tar_gz_filename
            )
        )

    print("Verifying checksum")
    local_md5 = hashlib.md5()
    with open(filename, "rb") as f:
        # Read in chunks to avoid reading the whole file into memory
        for chunk in iter(lambda: f.read(4096), b""):
            local_md5.update(chunk)

    if local_md5.hexdigest() != external_md5:
        raise ChecksumError(
            "Checksum mismatch! Local {} expected {}".format(local_md5.hexdigest(), external_md5)
        )
    print("OK")


def md5_matches(filename_to_check, reference_md5):

    local_md5 = hashlib.md5()
    with open(filename_to_check, "rb") as f:
        # Read in chunks to avoid reading the whole file into memory
        for chunk in iter(lambda: f.read(4096), b""):
            local_md5.update(chunk)

    if local_md5.hexdigest() != reference_md5:
        return False

    return True


def load_md5_from_url(md5_url):
    md5_data = urllib.request.urlopen(md5_url)
    external_md5, filename = md5_data.read().split()

    return external_md5


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
    titan_other_benchmarks_extract_dir = os.path.join(benchmarks_dir, "titan_other_blif")
    titan_arch_extract_dir = os.path.join(arch_dir, "titan")

    arch_upgrade_script = os.path.join(
        os.path.dirname(os.path.realpath(__file__)), "upgrade_arch.py"
    )

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
    tmpdir = tempfile.mkdtemp(suffix="download_titan", dir=".")

    try:
        # We use a callback along with extractall() to only walk through the large tar.gz file
        # once (saving runtime). We then move the files to the correct directories, which should
        # be a cheap rename operation

        # Extract matching files into the temporary directory
        with tarfile.TarFile.open(tar_gz_filename, mode="r|*") as tar_file:
            tar_file.extractall(path=tmpdir, members=extract_callback(tar_file))

        # Move the extracted files to the relevant directories, SDC files first (since we
        # need to look up the BLIF name to make it match)
        for dirpath, dirnames, filenames in os.walk(tmpdir):
            for filename in filenames:
                src_file_path = os.path.join(dirpath, filename)
                dst_file_path = None
                if fnmatch.fnmatch(src_file_path, "*/titan_release*/benchmarks/titan23/*/*.sdc"):
                    dst_file_path = os.path.join(
                        titan_benchmarks_extract_dir, determine_sdc_name(dirpath)
                    )
                elif fnmatch.fnmatch(
                    src_file_path, "*/titan_release*/benchmarks/other_benchmarks/*/*.sdc"
                ):
                    dst_file_path = os.path.join(
                        titan_other_benchmarks_extract_dir, determine_sdc_name(dirpath)
                    )

                if dst_file_path:
                    shutil.move(src_file_path, dst_file_path)

        # Then BLIFs
        for dirpath, dirnames, filenames in os.walk(tmpdir):
            for filename in filenames:
                src_file_path = os.path.join(dirpath, filename)
                dst_file_path = None
                if fnmatch.fnmatch(src_file_path, "*/titan_release*/benchmarks/titan23/*/*/*.blif"):
                    dst_file_path = os.path.join(titan_benchmarks_extract_dir, filename)
                elif fnmatch.fnmatch(
                    src_file_path, "*/titan_release*/benchmarks/other_benchmarks/*/*/*.blif"
                ):
                    dst_file_path = os.path.join(titan_other_benchmarks_extract_dir, filename)

                if dst_file_path:
                    shutil.move(src_file_path, dst_file_path)

    finally:
        # Clean-up
        shutil.rmtree(tmpdir)

    print("Done")


def determine_sdc_name(dirpath):
    """
    To have VTR pick up the SDC name, we need to match the corresponding BLIF file name
    """
    benchmark_dir = os.path.dirname(dirpath)  # Benchmark dir
    pattern = os.path.join(benchmark_dir, "*/*.blif")
    blif_paths = glob.glob(pattern)

    assert len(blif_paths) == 1

    blif_path = blif_paths[0]

    blif_file = os.path.basename(blif_path)
    blif_name, ext = os.path.splitext(blif_file)

    return blif_name + ".sdc"


def extract_callback(members):
    for tarinfo in members:
        if fnmatch.fnmatch(tarinfo.name, "titan_release*/benchmarks/titan23/*/*/*.blif"):
            print(tarinfo.name)
            yield tarinfo
        elif fnmatch.fnmatch(tarinfo.name, "titan_release*/benchmarks/titan23/*/sdc/vpr.sdc"):
            print(tarinfo.name)
            yield tarinfo
        if fnmatch.fnmatch(tarinfo.name, "titan_release*/benchmarks/other_benchmarks/*/*/*.blif"):
            print(tarinfo.name)
            yield tarinfo
        elif fnmatch.fnmatch(
            tarinfo.name, "titan_release*/benchmarks/other_benchmarks/*/sdc/vpr.sdc"
        ):
            print(tarinfo.name)
            yield tarinfo
        elif fnmatch.fnmatch(tarinfo.name, "titan_release*/arch/stratixiv*.xml"):
            print(tarinfo.name)
            yield tarinfo


if __name__ == "__main__":
    main()
