from sphinxcontrib.domaintools import custom_domain
from sphinx.util.docfields import *

def setup(app):
    app.add_domain(custom_domain('ArchDomain',
        name  = 'arch',
        label = "FPGA Architecture",

        elements = dict(
            tag = dict(
                objname       = "Attribute",
                indextemplate = "pair: %s; Tag Attribute",
                fields        = [
                    GroupedField('required_parameter',
                        label = "Required Attributes",
                        names = [ 'req_param' ]),
                    GroupedField('optional_parameter',
                        label = "Optional Attributes",
                        names = [ 'opt_param' ]),
                    Field('required',
                        label = "Tag Required",
                        names = [ 'required' ])
                ]
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
