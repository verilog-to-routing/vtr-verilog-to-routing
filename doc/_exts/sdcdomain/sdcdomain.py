from domaintools import custom_domain
from sphinx.util.docfields import *

def setup(app):
    app.add_domain(custom_domain('SdcDomain',
        name  = 'sdc',
        label = "SDC", 

        elements = dict(
            command = dict(
                objname      = "SDC Command",
                indextemplate = "pair: %s; SDC Command",
            ),
            option = dict(
                objname      = "SDC Option",
                indextemplate = "pair: %s; SDC Option",
            ),
            #var   = dict(
                #objname = "Make Variable",
                #indextemplate = "pair: %s; Make Variable"
            #),
            #expvar = dict(
                #objname = "Make Variable, exported",
                #indextemplate = "pair: %s; Make Variable, exported"
            #)
        )))
