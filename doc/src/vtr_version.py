from __future__ import print_function

import os
import re

class VersionInfo:
    def __init__(self, major, minor, patch, prerelease):
        self.major = major
        self.minor = minor
        self.patch = patch
        self.prerelease = prerelease

    def version_str(self):
        version_str = "<unkown vtr version>"
        if self.major != None and self.minor != None:
            version_str = "{}.{}".format(self.major, self.minor)

        return version_str

    def release_str(self):
        release_str = "<unkown vtr release>"

        if self.major != None and self.minor != None and self.patch != None:
            release_str = "{}.{}.{}".format(self.major, self.minor, self.patch)

            if self.prerelease != None and self.prerelease != "":
                release_str += "-{}".format(self.prerelease)

        return release_str

def get_vtr_version():
    return get_vtr_version_info().version_str()

def get_vtr_release():
    return get_vtr_version_info().release_str()

def get_vtr_version_info():
    #Assumes called with CWD set to the doc/src directory

    root_cmakelists = os.getcwd() + "/../../CMakeLists.txt"
    root_cmakelists = os.path.abspath(root_cmakelists)

    major = None
    minor = None
    patch = None
    prerelease = None
    try:

        major_regex = re.compile(r".*VTR_VERSION_MAJOR (?P<major>\d+)")
        minor_regex = re.compile(r".*VTR_VERSION_MINOR (?P<minor>\d+)")
        patch_regex = re.compile(r".*VTR_VERSION_PATCH (?P<patch>\d+)")
        prerelease_regex = re.compile(r".*VTR_VERSION_PRERELEASE \"(?P<prerelease>.*)\"")

        with open(root_cmakelists) as f:
            for line in f:

                match = major_regex.match(line)
                if match:
                    major = match.group('major')

                match = minor_regex.match(line)
                if match:
                    minor = match.group("minor")

                match = patch_regex.match(line)
                if match:
                    patch = match.group("patch")

                match = prerelease_regex.match(line)
                if match:
                    prerelease = match.group("prerelease")

    except IOError:
        print("Warning: Failed to find root CMakeLists.txt to find VTR version")
        pass

    return VersionInfo(major, minor, patch, prerelease)
