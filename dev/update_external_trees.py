#!/usr/bin/env python

import argparse
import os

class ExternalSubtree:
    def __init__(self, internal_path, external_url, default_external_ref='master'):
        self.internal_path = internal_path
        self.external_url = external_url
        self.default_external_ref = default_external_ref

EXTERNAL_SUBTREES = {
    'abc': ExternalSubtree(internal_path='abc', 
                           external_url='https://github.com/berkeley-abc/abc.git'),
    'libargparse': ExternalSubtree(internal_path='libs/EXTERNAL/libargparse', 
                                   external_url='https://github.com/kmurray/libargparse.git'),
    'libblifparse': ExternalSubtree(internal_path='libs/EXTERNAL/libblifparse',
                                    external_url='https://github.com/kmurray/libblifparse.git'),
    'libsdcparse': ExternalSubtree(internal_path='libs/EXTERNAL/libsdcparse',
                                   external_url='https://github.com/kmurray/libsdcparse.git'),
    'libtatum': ExternalSubtree(internal_path='libs/EXTERNAL/libtatum',
                                external_url='https://github.com/kmurray/tatum.git'),
}

def parse_args():
    parser = argparse.ArgumentParser(description="VTR uses external tools/libraries which are developed outside of the VTR source tree. Some of these are included in the VTR source tree using git's subtree feature. This script is used to update git-subtrees in a consistent manner. It must be run from the root of the VTR source tree.")

    parser.add_argument("components",
                        nargs="*",
                        metavar='COMPONENT',
                        help="External components to upgrade (COMPONENT must be one of {})".format(', '.join(EXTERNAL_SUBTREES.keys())))

    parser.add_argument("--external_ref",
                        default=None,
                        help="Specifies the external reference (revision/branch). If unspecified uses the subtree default.")

    parser.add_argument("--list",
                        action="store_true",
                        default=False,
                        help="External components to upgrade")

    parser.add_argument("-n", "--dry_run",
                        action="store_true",
                        default=False,
                        help="Show the commands which would be executed, but do not actually perform them.")

    args = parser.parse_args()

    return args

def main():
    args = parse_args()

    if args.list and len(args.components) == 0:
        args.components = EXTERNAL_SUBTREES.keys()

    for component in args.components:
        update_component(args, component, EXTERNAL_SUBTREES[component])

def update_component(args, component_name, subtree_info):
    print "{}: {} <- {}".format(component_name, subtree_info.internal_path, subtree_info.external_url)

    if args.list:
        return #List only

    external_ref = None
    if not args.external_ref:
        external_ref = subtree_info.default_external_ref
    assert external_ref != None

    action = None
    message = None
    if not os.path.exists(subtree_info.internal_path):
        #Create
        action = 'add'
        message = "{name}: Adding '{path}/' as an external git subtree from {url} {rev}".format(
                                                                      name=component_name,
                                                                      path=subtree_info.internal_path,
                                                                      url=subtree_info.external_url,
                                                                      rev=external_ref)
    else:
        #Pull
        action = 'pull'
        message = "{name}: Updating '{path}/' (external git subtree from {url} {rev})".format(
                                                                      name=component_name,
                                                                      path=subtree_info.internal_path,
                                                                      url=subtree_info.external_url,
                                                                      rev=external_ref)
    assert action != None
    assert message != None



    cmd = "git subtree {action} --prefix {internal_path} {external_url} {external_branch} --squash --message '{message}'".format(
                    action=action,
                    internal_path=subtree_info.internal_path,
                    external_url=subtree_info.external_url,
                    external_branch=external_ref,
                    message=message)

    if args.dry_run:
        print cmd
    else:
        os.system(cmd)
    

if __name__ == "__main__":
    main()
