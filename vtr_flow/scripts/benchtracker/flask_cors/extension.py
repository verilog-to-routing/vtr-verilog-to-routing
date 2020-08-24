# -*- coding: utf-8 -*-
"""
    flask_cors
    ~~~~
    Flask-CORS is a simple extension to Flask allowing you to support cross
    origin resource sharing (CORS) using a simple decorator.

    :copyright: (c) 2014 by Cory Dolphin.
    :license: MIT, see LICENSE for more details.
"""
from flask import request
from .core import *


class CORS(object):
    """
        Initializes Cross Origin Resource sharing for the application. The
        arguments are identical to :py:func:`cross_origin`, with the
        addition of a `resources` parameter. The resources parameter
        defines a series of regular expressions for resource paths to match
        and optionally, the associated options
        to be applied to the particular resource. These options are
        identical to the arguments to :py:func:`cross_origin`.

        The settings for CORS are determined in the following order:
            Resource level settings (e.g when passed as a dictionary)
            Keyword argument settings
            App level configuration settings (e.g. CORS_*)
            Default settings

        Note: as it is possible for multiple regular expressions to match a
        resource path, the regular expressions are first sorted by length,
        from longest to shortest, in order to attempt to match the most
        specific regular expression. This allows the definition of a
        number of specific resource options, with a wildcard fallback
        for all other resources.

        :param resources: the series of regular expression and (optionally)
        associated CORS options to be applied to the given resource path.

        If the argument is a dictionary, it is expected to be of the form:
        regexp : dict_of_options

        If the argument is a list, it is expected to be a list of regular
        expressions, for which the app-wide configured options are applied.

        If the argument is a string, it is expected to be a regular
        expression for which the app-wide configured options are applied.

        Default :'*'

        :type resources: dict, iterable or string

    """

    def __init__(self, app=None, **kwargs):
        self._options = kwargs
        if app is not None:
            self.init_app(app, **kwargs)

    def init_app(self, app, **kwargs):
        # The resources and options may be specified in the App Config, the CORS constructor
        # or the kwargs to the call to init_app.
        options = get_cors_options(app, self._options, kwargs)

        # Flatten our resources into a list of the form
        # (pattern_or_regexp, dictionary_of_options)
        resources = parse_resources(options.get("resources"))

        # Compute the options for each resource by combining the options from
        # the app's configuration, the constructor, the kwargs to init_app, and
        # finally the options specified in the resources dictionary.
        resources = [
            (pattern, get_cors_options(app, options, opts)) for (pattern, opts) in resources
        ]

        # Create a human readable form of these resources by converting the compiled
        # regular expressions into strings.
        resources_human = dict(
            [(get_regexp_pattern(pattern), opts) for (pattern, opts) in resources]
        )
        getLogger(app).info("Configuring CORS with resources: %s", resources_human)

        def cors_after_request(resp):
            """
                The actual after-request handler, retains references to the
                the options, and definitions of resources through a closure.
            """
            # If CORS headers are set in a view decorator, pass
            if resp.headers.get(ACL_ORIGIN):
                debugLog("CORS have been already evaluated, skipping")
                return resp

            for res_regex, res_options in resources:
                if try_match(request.path, res_regex):
                    debugLog(
                        "Request to '%s' matches CORS resource '%s'. Using options: %s",
                        request.path,
                        get_regexp_pattern(res_regex),
                        res_options,
                    )
                    set_cors_headers(resp, res_options)
                    break
            else:
                debugLog("No CORS rule matches")
            return resp

        app.after_request(cors_after_request)

        # Wrap exception handlers with cross_origin
        # These error handlers will still respect the behavior of the route
        if options.get("intercept_exceptions", True):

            def _after_request_decorator(f):
                def wrapped_function(*args, **kwargs):
                    return cors_after_request(app.make_response(f(*args, **kwargs)))

                return wrapped_function

            app.handle_exception = _after_request_decorator(app.handle_exception)
            app.handle_user_exception = _after_request_decorator(app.handle_user_exception)
