# -*- coding: utf-8 -*-
"""
    flask_cors
    ~~~~
    Flask-CORS is a simple extension to Flask allowing you to support cross
    origin resource sharing (CORS) using a simple decorator.

    :copyright: (c) 2014 by Cory Dolphin.
    :license: MIT, see LICENSE for more details.
"""
from .decorator import cross_origin
from .extension import CORS
from .version import __version__

__all__ = ["CORS", "cross_origin"]
