import logging
import os, sys
from os.path import splitext
from docutils import nodes
from sphinx import addnodes
from sphinx.roles import XRefRole
from sphinx.domains import Domain
from sphinx.util.nodes import make_refnode

if sys.version_info < (3, 0):
    from urlparse import urlparse, unquote
else:
    from urllib.parse import urlparse, unquote

from recommonmark import parser
"""
Allow linking of Markdown documentation from the source code tree into the Sphinx
documentation tree.

The Markdown documents will have links relative to the source code root, rather
than the place they are now linked too - this code fixes these paths up.

We also want links from two Markdown documents found in the Sphinx docs to
work, so that is also fixed up.
"""


def path_contains(parent_path, child_path):
    """Check a path contains another path.

    >>> path_contains("a/b", "a/b")
    True
    >>> path_contains("a/b", "a/b/")
    True
    >>> path_contains("a/b", "a/b/c")
    True
    >>> path_contains("a/b", "c")
    False
    >>> path_contains("a/b", "c/b")
    False
    >>> path_contains("a/b", "c/../a/b/d")
    True
    >>> path_contains("../a/b", "../a/b/d")
    True
    >>> path_contains("../a/b", "../a/c")
    False
    >>> path_contains("a", "abc")
    False
    >>> path_contains("aa", "abc")
    False
    """
    # Append a separator to the end of both paths to work around the fact that
    # os.path.commonprefix does character by character comparisons rather than
    # path segment by path segment.
    parent_path = os.path.join(os.path.normpath(parent_path), '')
    child_path = os.path.join(os.path.normpath(child_path), '')
    common_path = os.path.commonprefix((parent_path, child_path))
    return common_path == parent_path


def relative(parent_dir, child_path):
    """Get the relative between a path that contains another path."""
    child_dir = os.path.dirname(child_path)
    assert path_contains(parent_dir, child_dir), "{} not inside {}".format(
        child_path, parent_dir)
    return os.path.relpath(child_path, start=parent_dir)


class MarkdownSymlinksDomain(Domain):
    """
    Extension of the Domain class to implement custom cross-reference links
    solve methodology
    """

    name = 'markdown_symlinks'
    label = 'MarkdownSymlinks'

    roles = {
        'xref': XRefRole(),
    }

    docs_root_dir = os.path.realpath(os.path.dirname(__file__))
    code_root_dir = os.path.realpath(os.path.join(docs_root_dir, "..", ".."))

    mapping = {
        'docs2code': {},
        'code2docs': {},
    }

    github_repo_url = ""
    github_repo_branch = ""

    @classmethod
    def relative_code(cls, url):
        """Get a value relative to the code directory."""
        return relative(cls.code_root_dir, url)

    @classmethod
    def relative_docs(cls, url):
        """Get a value relative to the docs directory."""
        return relative(cls.docs_root_dir, url)

    @classmethod
    def add_mapping(cls, docs_rel, code_rel):
        assert docs_rel not in cls.mapping['docs2code'], """\
Assertion error! Document already in mapping!
    New Value: {}
Current Value: {}
""".format(docs_rel, cls.mapping['docs2code'][docs_rel])

        assert code_rel not in cls.mapping['code2docs'], """\
Assertion error! Document already in mapping!
    New Value: {}
Current Value: {}
""".format(docs_rel, cls.mapping['code2docs'][code_rel])

        cls.mapping['docs2code'][docs_rel] = code_rel
        cls.mapping['code2docs'][code_rel] = docs_rel

    @classmethod
    def find_links(cls):
        """Walk the docs dir and find links to docs in the code dir."""
        for root, dirs, files in os.walk(cls.docs_root_dir):
            for dname in dirs:
                dpath = os.path.abspath(os.path.join(root, dname))

                if not os.path.islink(dpath):
                    continue

                link_path = os.path.join(root, os.readlink(dpath))
                # Is link outside the code directory?
                if not path_contains(cls.code_root_dir, link_path):
                    continue

                # Is link internal to the docs directory?
                if path_contains(cls.docs_root_dir, link_path):
                    continue

                docs_rel = cls.relative_docs(dpath)
                code_rel = cls.relative_code(link_path)

                cls.add_mapping(docs_rel, code_rel)

            for fname in files:
                fpath = os.path.abspath(os.path.join(root, fname))

                if not os.path.islink(fpath):
                    continue

                link_path = os.path.join(root, os.readlink(fpath))
                # Is link outside the code directory?
                if not path_contains(cls.code_root_dir, link_path):
                    continue

                # Is link internal to the docs directory?
                if path_contains(cls.docs_root_dir, link_path):
                    continue

                docs_rel = cls.relative_docs(fpath)
                code_rel = cls.relative_code(link_path)

                cls.add_mapping(docs_rel, code_rel)

        import pprint
        pprint.pprint(cls.mapping)

    @classmethod
    def add_github_repo(cls, github_repo_url, github_repo_branch):
        """Initialize the github repository to update links correctly."""
        cls.github_repo_url = github_repo_url
        cls.github_repo_branch = github_repo_branch

    # Overriden method to solve the cross-reference link
    def resolve_xref(
            self, env, fromdocname, builder, typ, target, node, contnode):
        if '#' in target:
            todocname, targetid = target.split('#')
        else:
            todocname = target
            targetid = ''

        if todocname not in self.mapping['code2docs']:
            # Could be a link to a repository's code tree directory/file
            todocname = '{}{}{}'.format(self.github_repo_url, self.github_repo_branch, todocname)

            newnode = nodes.reference('', '', internal=True, refuri=todocname)
            newnode.append(contnode[0])
        else:
            # Removing filename extension (e.g. contributing.md -> contributing)
            todocname, _ = os.path.splitext(self.mapping['code2docs'][todocname])

            newnode = make_refnode(
                builder, fromdocname, todocname, targetid, contnode[0])

        return newnode

    def resolve_any_xref(
            self, env, fromdocname, builder, target, node, contnode):
        res = self.resolve_xref(
            env, fromdocname, builder, 'xref', target, node, contnode)
        return [('markdown_symlinks:xref', res)]


class LinkParser(parser.CommonMarkParser, object):
    def visit_link(self, mdnode):
        ref_node = nodes.reference()

        destination = mdnode.destination

        ref_node.line = self._get_line(mdnode)
        if mdnode.title:
            ref_node['title'] = mdnode.title

        url_check = urlparse(destination)
        # If there's not a url scheme (e.g. 'https' for 'https:...' links),
        # or there is a scheme but it's not in the list of known_url_schemes,
        # then assume it's a cross-reference and pass it to Sphinx as an `:any:` ref.
        known_url_schemes = self.config.get('known_url_schemes')
        if known_url_schemes:
            scheme_known = url_check.scheme in known_url_schemes
        else:
            scheme_known = bool(url_check.scheme)

        is_dest_xref = url_check.fragment and destination[:1] != '#'

        ref_node['refuri'] = destination
        next_node = ref_node

        # Adds pending-xref wrapper to unsolvable cross-references
        if (is_dest_xref or not url_check.fragment) and not scheme_known:
            wrap_node = addnodes.pending_xref(
                reftarget=unquote(destination),
                reftype='xref',
                refdomain='markdown_symlinks',  # Added to enable cross-linking
                refexplicit=True,
                refwarn=True)
            # TODO also not correct sourcepos
            wrap_node.line = self._get_line(mdnode)
            if mdnode.title:
                wrap_node['title'] = mdnode.title
            wrap_node.append(ref_node)
            next_node = wrap_node

        self.current_node.append(next_node)
        self.current_node = ref_node


if __name__ == "__main__":
    import doctest
    doctest.testmod()
