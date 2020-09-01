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


class DownloadError(Exception):
    pass


class ChecksumError(Exception):
    pass


class ExtractionError(Exception):
    pass


ISPD_URL_MIRRORS = {
    "google": "https://storage.googleapis.com/verilog-to-routing/ispd/",
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
        "--version", default="0.0.1", help="ISPD for VTR benchmark release version to download"
    )
    parser.add_argument(
        "--vtr_flow_dir",
        required=True,
        help="The 'vtr_flow' directory under the VTR tree. "
        "If specified this will extract the titan release, "
        "placing benchmarks under vtr_flow/benchmarks/titan, "
        "and architectures under vtr_flow/arch/titan ",
    )
    parser.add_argument(
        "--force",
        default=False,
        action="store_true",
        help="Run extraction step even if directores etc. already exist",
    )

    parser.add_argument(
        "--mirror", default="google", choices=list(ISPD_URL_MIRRORS.keys()), help="Download mirror"
    )

    return parser.parse_args()


def main():

    args = parse_args()

    try:
        tar_gz_filename = "ispd_benchmarks_vtr_v" + args.version + ".tar.gz"
        md5_filename = "ispd_benchmarks_vtr_v" + args.version + ".md5"

        tar_gz_url = urllib.parse.urljoin(ISPD_URL_MIRRORS[args.mirror], tar_gz_filename)
        md5_url = urllib.parse.urljoin(ISPD_URL_MIRRORS[args.mirror], md5_filename)

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
        print("Failed to extrac :", e)
        sys.exit(3)

    sys.exit(0)


def download_url(filename, url):
    """
    Downloads the release
    """
    urllib.request.urlretrieve(url, filename, reporthook=download_progress_callback)


def verify(tar_gz_filename, md5_url):
    """
    Performs checksum verification of downloaded release file
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
    Extracts the 'arch' and 'benchmarks' directories of the ispd release
    into their corresponding vtr directories
    """

    # Reference directories
    arch_dir = os.path.join(args.vtr_flow_dir, "arch")
    benchmarks_dir = os.path.join(args.vtr_flow_dir, "benchmarks")
    ispd_benchmarks_extract_dir = os.path.join(benchmarks_dir, "ispd_blif")

    if not args.force:
        # Check that all expected directories exist
        expected_dirs = [args.vtr_flow_dir, benchmarks_dir, arch_dir, ispd_benchmarks_extract_dir]
        for dir in expected_dirs:
            if not os.path.isdir(dir):
                raise ExtractionError("{} should be a directory".format(dir))

    # Create a temporary working directory
    tmpdir = tempfile.mkdtemp(suffix="download_ispd", dir=".")

    try:
        # We use a callback along with extractall() to only walk through the large tar.gz file
        # once (saving runtime). We then move the files to the correct directories, which should
        # be a cheap rename operation

        # Extract matching files into the temporary directory
        with tarfile.TarFile.open(tar_gz_filename, mode="r|*") as tar_file:
            tar_file.extractall(path=tmpdir, members=extract_callback(tar_file))

        # Move the extracted files to the relevant directories
        for dirpath, dirnames, filenames in os.walk(tmpdir):
            for filename in filenames:
                src_file_path = os.path.join(dirpath, filename)
                dst_file_path = None
                if fnmatch.fnmatch(src_file_path, "*/ispd_benchmarks*/benchmarks/*/*.blif"):
                    dst_file_path = os.path.join(ispd_benchmarks_extract_dir, filename)

                shutil.move(src_file_path, dst_file_path)

    finally:
        # Clean-up
        shutil.rmtree(tmpdir)

    print("Done")


def extract_callback(members):
    for tarinfo in members:
        if fnmatch.fnmatch(tarinfo.name, "ispd_benchmarks*/benchmarks/*/*.blif"):
            print(tarinfo.name)
            yield tarinfo


if __name__ == "__main__":
    main()
