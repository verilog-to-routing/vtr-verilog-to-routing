#!/usr/bin/env python
import sys
import os
import argparse
import urlparse
import urllib
import urllib2
import hashlib
import math
import textwrap
import tarfile
import fnmatch
import errno
import pudb

class DownloadError(Exception):
    pass

class ChecksumError(Exception):
    pass

class ExtractionError(Exception):
    pass

TITAN_URL="http://www.eecg.utoronto.ca/~kmurray/titan/"


def parse_args():
    description = textwrap.dedent("""
                    Download and extract a Titan benchmark release into a
                    VTR-style directory structure.
                  """)
    parser = argparse.ArgumentParser(
                formatter_class=argparse.ArgumentDefaultsHelpFormatter
             )

    parser.add_argument("--titan_version",
                        default="1.1.0",
                        help="Titan release version to download")
    parser.add_argument("--vtr_flow_dir",
                        default=None,
                        help="The 'vtr_flow' directory under the VTR tree. "
                             "If specified this will extract the titan release, "
                             "placing benchmarks under vtr_flow/benchmarks/titan, "
                             "and architectures under vtr_flow/arch/titan ")
    parser.add_argument("--skip_verification",
                        default=False,
                        action="store_true",
                        help="Skips checksum verification")
    parser.add_argument("--force",
                        default=False,
                        action="store_true",
                        help="Run extraction step even if directores etc. already exist")

    return parser.parse_args()

def main():

    args = parse_args()

    try:
        tar_gz_filename = download_titan(args)

        if args.vtr_flow_dir:
            extract_to_vtr_flow_dir(args, tar_gz_filename)

    except DownloadError as e:
        print "Failed to download:", e
        sys.exit(1)
    except ChecksumError as e:
        print "File corrupt:", e
        sys.exit(2)
    except ExtractionError as e:
        print "Failed to extract titan release:", e
        sys.exit(3)

    sys.exit(0)


def download_titan(args):
    """
    Downloads the titan release
    """
    tar_gz_filename = "titan_release_" + args.titan_version + '.tar.gz'
    md5_filename = "titan_release_" + args.titan_version + '.md5'
    
    tar_gz_url = urlparse.urljoin(TITAN_URL, tar_gz_filename)
    md5_url = urlparse.urljoin(TITAN_URL, md5_filename)

    if not os.path.isfile(tar_gz_filename):
        print "Downloading {}".format(tar_gz_url)
        urllib.urlretrieve(tar_gz_url, tar_gz_filename, reporthook=download_progress_callback)
    else:
        print "Found existing {}".format(tar_gz_filename)

    if not args.skip_verification:
        verify_titan(tar_gz_filename, md5_url)

    return tar_gz_filename

def verify_titan(tar_gz_filename, md5_url):
    """
    Performs checksum verification of downloaded titan release file
    """

    print "Downloading {}".format(md5_url)
    md5_data = urllib2.urlopen(md5_url)
    external_md5, filename = md5_data.read().split()

    if(filename != tar_gz_filename):
        raise VerificationError("External MD5 appears to be for a different file. Was {} expected {}".format(filename, tar_gz_filename))

    print "Verifying checksum"
    local_md5 = hashlib.md5()
    with open(filename, "rb") as f:
        #Read in chunks to avoid reading the whole file into memory
        for chunk in iter(lambda: f.read(4096), b""):
            local_md5.update(chunk)

    if local_md5.hexdigest() != external_md5:
        raise ChecksumError("Checksum mismatch! Local {} expected {}".format(local_md5.hexdigest(), external_md5))
    print "OK"

def download_progress_callback(block_num, block_size, expected_size):
    """
    Callback for urllib.urlretrieve which prints a dot for every percent of a file downloaded
    """
    total_blocks = int(math.ceil(expected_size / block_size))
    progress_increment = int(math.ceil(total_blocks / 100))

    if block_num % progress_increment == 0:
        sys.stdout.write(".")
        sys.stdout.flush()
    if block_num*block_size >= expected_size:
        print ""

def extract_to_vtr_flow_dir(args, tar_gz_filename):
    """
    Extracts the 'arch' and 'benchmarks' directories of the titan release
    into their corresponding vtr directories
    """

    #Reference directories
    arch_dir = os.path.join(args.vtr_flow_dir, "arch")
    benchmarks_dir = os.path.join(args.vtr_flow_dir, "benchmarks")
    titan_benchmarks_extract_dir = os.path.join(benchmarks_dir, 'titan_blif')
    titan_arch_extract_dir = os.path.join(arch_dir, 'titan')

    if not args.force:
        #Check that all expected directories exist
        expected_dirs = [args.vtr_flow_dir, benchmarks_dir, arch_dir, titan_benchmarks_extract_dir, titan_arch_extract_dir]
        for dir in expected_dirs:
            if not os.path.isdir(dir):
                raise ExtractionError("{} should be a directory".format(dir))

    tar_file = tarfile.TarFile.open(tar_gz_filename, mode="r|*")

    print "Searching release for benchmarks and architectures..."
    members_to_extract = []
    for member in tar_file.getmembers():
        #print member
        if fnmatch.fnmatch(member.name, "titan_release*/benchmarks/titan23/*/*/*.blif"):
            members_to_extract.append(member)
        elif fnmatch.fnmatch(member.name, "titan_release*/arch/stratixiv*.xml"):
            members_to_extract.append(member)

    #print [x.name for x in members_to_extract]

    #pudb.set_trace()

    tar_file = tarfile.TarFile.open(tar_gz_filename, mode="r|*")
    
    for member in members_to_extract:
        if member.name.endswith(".blif"):
            output_filename = os.path.join(titan_benchmarks_extract_dir, os.path.basename(member.name))
        else:
            assert member.name.endswith(".xml")
            output_filename = os.path.join(titan_arch_extract_dir, os.path.basename(member.name))
        print "Extracting", member.name, "to", output_filename
        with open(output_filename, 'wb') as f:
            for line in tar_file.extractfile(member):
                print >>f, line,

    print "Done"

if __name__ == "__main__":
    main()
