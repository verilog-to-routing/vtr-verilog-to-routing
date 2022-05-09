# -*- coding: utf-8 -*-
#
# Updated documentation of the configuration options is available at
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import sys
import os
import shlex
import shutil
import subprocess


sys.path.append(".")
sys.path.insert(0, os.path.abspath("../../vtr_flow/scripts/python_libs"))
from markdown_code_symlinks import LinkParser, MarkdownSymlinksDomain

# Cool looking ReadTheDocs theme
import sphinx_rtd_theme

# See if sphinxcontrib.bibtex has been installed
have_sphinxcontrib_bibtex = True
try:
    import sphinxcontrib.bibtex
except ImportError:
    have_sphinxcontrib_bibtex = False


sys.path.append(os.path.abspath("../_exts"))  # For extensions
sys.path.append(os.path.abspath("."))  # For vtr_version

from vtr_version import get_vtr_version, get_vtr_release

# -- General configuration ------------------------------------------------

needs_sphinx = "3.0"

extensions = [
    "sphinx.ext.todo",
    "sphinx.ext.mathjax",
    "sphinx.ext.imgmath",
    "sphinx.ext.napoleon",
    "sphinx.ext.coverage",
    "breathe",
    "notfound.extension",
    "sphinx_markdown_tables",
    "sdcdomain",
    "archdomain",
    "rrgraphdomain",
    "myst_parser",
    "sphinx.ext.autodoc",
    "sphinx.ext.graphviz",
]

if have_sphinxcontrib_bibtex:
    extensions.append("sphinxcontrib.bibtex")
    bibtex_bibfiles = ["z_references.bib"]
else:
    print(
        "Warning: Could not find sphinxcontrib.bibtex for managing citations, attempting to build anyway..."
    )

templates_path = []

source_suffix = [".rst"]

master_doc = "index"

project = "Verilog-to-Routing"
copyright = "2012-2022, VTR Developers"
author = "VTR Developers"

version = get_vtr_version()
release = get_vtr_release()

language = None

exclude_patterns = ["_build"]

pygments_style = "sphinx"

todo_include_todos = True

numfig = True

# -- Options for HTML output ----------------------------------------------

html_theme = "sphinx_rtd_theme"

html_theme_path = [sphinx_rtd_theme.get_html_theme_path()]

html_logo = "_static/vtr_logo.svg"

html_favicon = "_static/favicon.ico"

html_static_path = ["_static"]

html_scaled_image_link = True

htmlhelp_basename = "Verilog-to-Routingdoc"

# -- Options for LaTeX output ---------------------------------------------

latex_elements = {}

latex_documents = [
    (
        master_doc,
        "Verilog-to-Routing.tex",
        "Verilog-to-Routing Documentation",
        "VTR Developers",
        "manual",
    ),
]

latex_logo = "_static/vtr_logo.pdf"

# -- Options for manual page output ---------------------------------------

man_pages = [(master_doc, "verilog-to-routing", "Verilog-to-Routing Documentation", [author], 1)]

# -- Options for Texinfo output -------------------------------------------

texinfo_documents = [
    (
        master_doc,
        "Verilog-to-Routing",
        "Verilog-to-Routing Documentation",
        author,
        "Verilog-to-Routing",
        "One line description of project.",
        "Miscellaneous",
    ),
]

# -- Options for 404 page -------------------------------------------

# sphinx-notfound-page
# https://github.com/readthedocs/sphinx-notfound-page
notfound_context = {
    "title": "Page Not Found",
    "body": """
<h1>Page Not Found</h1>
<p>Sorry, we couldn't find that page.</p>
<p>Try using the search box or go to the homepage.</p>
""",
}

if shutil.which("doxygen"):
    breathe_projects = {
        "vpr": "../_build/doxygen/vpr/xml",
        "vtr": "../_build/doxygen/vtr/xml",
        "abc": "../_build/doxygen/abc/xml",
        "ace2": "../_build/doxygen/ace2/xml",
        "ODIN_II": "../_build/doxygen/ODIN_II/xml",
        "blifexplorer": "../_build/doxygen/blifexplorer/xml",
        "librrgraph": "../_build/doxygen/librrgraph/xml",
    }
    breathe_default_project = "vpr"

    if not os.environ.get("SKIP_DOXYGEN", None) == "True":
        for prjname, prjdir in breathe_projects.items():
            print("Generating doxygen files for {}...".format(prjname))
            os.makedirs(prjdir, exist_ok=True)
            cmd = "cd ../_doxygen && doxygen {}.dox".format(prjname)
            subprocess.call(cmd, shell=True)
    else:
        for prjname, prjdir in breathe_projects.items():
            assert os.path.exists(prjdir) == True, "Regenerate doxygen XML for {}".format(prjname)

# Add page anchors for myst parser
myst_heading_anchors = 4


def setup(app):
    app.add_stylesheet("css/vtr.css")
