#!/usr/bin/python3

"""
It is important that everyone (including CI on Travis / BuildBot) is using
**exactly** the same version of `clang-format` when formatting the code,
otherwise there might be a disagreement on how things should be formatted.

Chromium maintains statically compiled clang-format binaries for formatting
their code base. These binaries are frequently a lot newer than those available
in people's distributions. Rather than duplicate their work to create these
binaries, we just pull their binaries into our own source tree.
"""

import base64
import hashlib
import json
import os
import pprint
import subprocess
import sys
import tempfile
import urllib.request


def get_modified():
    modified = subprocess.check_output("git status -s", shell=True)
    modified = modified.decode('utf-8', errors="replace")
    return modified.strip().splitlines()

if get_modified():
    print("""\
Modified files found!

Please commit before running this script.
""")
    sys.exit(1)

# Where to find the Chromium source.
CHROMIUM_GOOGLE_SRC = "https://chromium.googlesource.com/chromium/src"

# Google Cloud Storage bucket with the actual binaries.
CHROMIUM_CLOUD_STORAGE = "http://chromium-clang-format.storage.googleapis.com"
HEAD = "/refs/heads/master"


def get_from_googlesource(path, ref, ofmt):
    """

    Get the contents of file DEPS at master
    >>> get_from_googlesource(
    ...   "/DEPS", HEAD, "TEXT")

    Get the JSON metadata for file DEPS at master
    >>> get_from_googlesource(
    ...   "/DEPS", HEAD, "JSON")

    Get the JSON metadata for the tree at a given commit
    >>> get_from_googlesource(
    ...   "", "7599d00f4bb53dc1f84826931ec64c2ccbae0a40", "JSON")
    """

    valid_ofmt = ("TEXT", "JSON")
    assert ofmt in valid_ofmt, "Unknown ofmt {}, valid {}".format(ofmt, valid_ofmt)

    url = "{base_url}/+{ref}{path}?format={ofmt}".format(
        base_url = CHROMIUM_GOOGLE_SRC,
        ref = ref,
        path = path,
        ofmt = ofmt,
    )
    #print("Downloading {} as {}".format(url, ofmt))
    with urllib.request.urlopen(url) as u:
        data = u.read()

    assert data
    data = data.decode("utf-8", errors="replace")
    if ofmt == "TEXT":
        data = base64.b64decode(data)
        data = data.decode("utf-8", errors="replace")
        return data
    elif ofmt == "JSON":
        data = data[data.find('{'):]
        data = json.loads(data)
        return data
    else:
        assert False, "Not reached."

# Where the binary hashes are found in the Chromium src tree.
# The keys should match CMAKE_HOST_SYSTEM_NAME output.
CHROMIUM_CLANG_FORMAT_SHA1 = {
    'Windows':  '/buildtools/win/clang-format.exe.sha1',
    'Darwin':   '/buildtools/mac/clang-format.sha1',
    'Linux':    '/buildtools/linux64/clang-format.sha1',
}

CMAKE_PATH = "cmake/modules/AutoClangFormat/bin/"

# Get the binary data and properties.
#current_commit = get_from_googlesource("", HEAD, "JSON")
file_info = {}
for k, path in CHROMIUM_CLANG_FORMAT_SHA1.items():
    print()
    print(k)
    print("-"*75)

    filename = os.path.basename(path).replace('.sha1', '')

    p = {}
    p['path'] = path
    p['filename'], p['fileext'] = os.path.splitext(filename)
    p['filename'] += "-"+k
    p['outfile'] = os.path.join(
        CMAKE_PATH,
        "{filename}{fileext}".format(**p),
    )

    # Get the metadata about the sha1 file.
    p.update(get_from_googlesource(path, HEAD, "JSON"))

    # Get the contents of the sha1 file which tells us what to download from
    # Google Cloud storage.
    dhash = get_from_googlesource(path, HEAD, "TEXT")
    assert len(dhash) == 40, "Invalid data hash {}".format(repr(dhash))
    p["hash"] = dhash

    if os.path.exists(p["outfile"]):
        with open(p["outfile"], "rb") as f:
            file_hash = hashlib.sha1(f.read()).hexdigest()

        if file_hash == dhash:
            print("File {} (with hash {}) doesn't need updating!".format(p["outfile"], dhash))
            continue

    commit_log = get_from_googlesource(path, "log"+HEAD, "JSON")["log"]
    p["log"] = commit_log

    # Download the data from Google Cloud storage
    data_url = "{}/{}".format(CHROMIUM_CLOUD_STORAGE, dhash)
    with urllib.request.urlopen(data_url) as u:
        data = u.read()

    download_hash = hashlib.sha1(data).hexdigest()
    assert dhash == download_hash, """\
   Data hash was {}
But it should be {}
""".format(download_hash, dhash)

    # Write the file out to disk
    ofile = p["outfile"]
    with open(ofile, "wb") as f:
        f.write(data)
    subprocess.check_call("chmod a+x {}".format(ofile), shell=True)
    subprocess.check_call("git add {}".format(ofile), shell=True)

    p["data"] = "{} bytes".format(len(data))

    pprint.pprint(p)
    file_info[k] = p

if not file_info:
    print("No modified_files, not committing!")
    sys.exit(0)

print()
print()
print("="*75)
print()

# Get the clang version info
clang_version = subprocess.check_output(
    "./cmake/modules/AutoClangFormat/bin/clang-format-Linux --version", shell=True)
clang_version = clang_version.decode("utf-8").strip()

# Create a commit message
l = []
for k in file_info:
    p = file_info[k]

    commit = p["log"][0]
    # https://chromium.googlesource.com/chromium/src/+/7599d00f4bb53dc1f84826931ec64c2ccbae0a40
    commit_url = "{url}/+/{commit}".format(
        url=CHROMIUM_GOOGLE_SRC, commit=commit["commit"])
    l.append(
        " * [{platform} updated @ {commit_time} - {commit_msg}]({commit_url}]".format(
            platform=k,
            commit_time=commit["committer"]["time"],
            commit_msg=commit["message"].splitlines()[0],
            commit_url=commit_url,
        )
    )

# Commit the binaries.
with tempfile.NamedTemporaryFile("w") as f:
    f.write("""\
Updating clang-format binaries from Chromium.

clang-format version: `{}`

{}
""".format(clang_version, "\n".join(l)))
    f.flush()

    subprocess.check_call("git commit --file={}".format(f.name), shell=True)
