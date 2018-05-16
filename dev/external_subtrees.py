#!/usr/bin/env python

import argparse
import os
import xml.etree.ElementTree as ET
from collections import OrderedDict

class ExternalSubtree:
    def __init__(self, name, internal_path, external_url, default_external_ref):
        self.name = name
        self.internal_path = internal_path
        self.external_url = external_url
        self.default_external_ref = default_external_ref

SCRIPT_DIR=os.path.dirname(os.path.realpath(__file__))
DEFAULT_CONFIG=os.path.join(SCRIPT_DIR, 'subtree_config.xml')

def parse_args():
    parser = argparse.ArgumentParser(description="VTR uses external tools/libraries which are developed outside of the VTR source tree. Some of these are included in the VTR source tree using git's subtree feature. This script is used to update git-subtrees in a consistent manner. It must be run from the root of the VTR source tree.")

    parser.add_argument("components",
                        nargs="*",
                        metavar='COMPONENT',
                        help="External components to upgrade (See list of components with --list)")

    parser.add_argument("--external_ref",
                        default=None,
                        help="Specifies the external reference (revision/branch). If unspecified uses the subtree default (usually master).")

    exclusive_group = parser.add_mutually_exclusive_group()
    exclusive_group.add_argument("--list",
                        action="store_true",
                        default=False,
                        help="List known components")

    exclusive_group.add_argument("--update",
                        action="store_true",
                        default=False,
                        help="Update components subtrees")

    parser.add_argument("-n", "--dry_run",
                        action="store_true",
                        default=False,
                        help="Show the commands which would be executed, but do not actually perform them.")

    parser.add_argument("--subtree_config",
                        default=DEFAULT_CONFIG,
                        help="Path to subtree config file (Default: %(default)s)")

    args = parser.parse_args()

    return args

def main():
    args = parse_args()

    config = load_subtree_config(args.subtree_config)

    if args.list and len(args.components) == 0:
        args.components = config.keys()

    for component in args.components:
        update_component(args, config[component])

def load_subtree_config(config_path):

    config = OrderedDict()

    tree = ET.parse(config_path)
    root = tree.getroot()

    assert root.tag == 'subtrees', "Expected root tag to be 'subtrees'"

    for child in root:
        assert child.tag == 'subtree', "Expected children to be 'subtree'"

        name = None
        internal_path = None
        external_url = None
        default_external_ref = None
        for attrib, value in child.attrib.iteritems():

            if attrib == 'name':
                name = value
            elif attrib == 'internal_path':
                internal_path = value
            elif attrib == 'external_url':
                external_url = value
            elif attrib == 'default_external_ref':
                default_external_ref = value
            else:
                assert False, "Unrecognized subtree attribute {}".format(attrib)

        assert name != None, "Missing subtree name"
        assert internal_path != None, "Missing subtree internal path"
        assert external_url != None, "Missing subtree external url"
        assert default_external_ref != None, "Missing subtree default external ref"

        assert name not in config, "Duplicate subtree name '{}'".format(name)

        config[name] = ExternalSubtree(name, internal_path, external_url, default_external_ref)

    return config

def update_component(args, subtree_info):
    external_ref = None
    if not args.external_ref:
        external_ref = subtree_info.default_external_ref

    print "Component: {:<15} Path: {:<30} URL: {:<45} URL_Ref: {}".format(subtree_info.name, subtree_info.internal_path, subtree_info.external_url, external_ref)

    if args.list:
        return #List only
    else:
        assert args.update

    assert external_ref != None

    action = None
    message = None
    if not os.path.exists(subtree_info.internal_path):
        #Create
        action = 'add'
        message = "{name}: Adding '{path}/' as an external git subtree from {url} {rev}".format(
                                                                      name=subtree_info.name,
                                                                      path=subtree_info.internal_path,
                                                                      url=subtree_info.external_url,
                                                                      rev=external_ref)
    else:
        #Pull
        action = 'pull'
        message = "{name}: Updating '{path}/' (external git subtree from {url} {rev})".format(
                                                                      name=subtree_info.name,
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
