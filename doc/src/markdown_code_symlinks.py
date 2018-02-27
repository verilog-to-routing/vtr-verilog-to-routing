import logging
import os

from recommonmark import transform

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


class MarkdownCodeSymlinks(transform.AutoStructify, object):
    docs_root_dir = os.path.realpath(os.path.dirname(__file__))
    code_root_dir = os.path.realpath(os.path.join(docs_root_dir, "..", ".."))

    mapping = {
        'docs2code': {},
        'code2docs': {},
    }

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

    @property
    def url_resolver(self):
        return self._url_resolver

    @url_resolver.setter
    def url_resolver(self, value):
        print(self, value)

    # Resolve a link from one markdown to another document.
    def _url_resolver(self, ourl):
        """Resolve a URL found in a markdown file."""
        assert self.docs_root_dir == os.path.realpath(self.root_dir), """\
Configuration error! Document Root != Current Root
Document Root: {}
 Current Root: {}
""".format(self.docs_root_dir, self.root_dir)

        src_path = os.path.abspath(self.document['source'])
        src_dir = os.path.dirname(src_path)
        dst_path = os.path.abspath(os.path.join(self.docs_root_dir, ourl))
        dst_rsrc = os.path.relpath(dst_path, start=src_dir)

        src_rdoc = self.relative_docs(src_path)

        print
        print("url_resolver")
        print(src_path)
        print(dst_path)
        print(dst_rsrc)
        print(src_rdoc)

        # Is the source document a linked one?
        if src_rdoc not in self.mapping['docs2code']:
            # Don't do any rewriting on non-linked markdown.
            url = ourl

        # Is the destination also inside docs?
        elif dst_rsrc not in self.mapping['code2docs']:
            # Return a path to the GitHub repo.
            url = "{}/blob/master/{}".format(
                self.config['github_code_repo'], dst_rsrc)
        else:
            url = os.path.relpath(
                os.path.join(self.docs_root_dir, self.mapping['code2docs'][dst_rsrc]),
                start=src_dir)
            base_url, ext = os.path.splitext(url)
            assert ext in (".md", ".markdown"), (
                "Unknown extension {}""".format(ext))
            url = "{}.html".format(base_url)

        print("---")
        print(ourl)
        print(url)
        print
        return url





if __name__ == "__main__":
    import doctest
    doctest.testmod()
