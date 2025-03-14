from sphinxcontrib.domaintools import custom_domain
from sphinx.util.docfields import *


def setup(app):
    app.add_domain(
        custom_domain(
            "VPRConstraintsDomain",
            name="vpr_constraints",
            label="Place and Route Constraints",
            elements=dict(
                tag=dict(
                    objname="Attribute",
                    indextemplate="pair: %s; Tag Attribute",
                    fields=[
                        GroupedField(
                            "required_parameter", label="Required Attributes", names=["req_param"]
                        ),
                        GroupedField(
                            "optional_parameter", label="Optional Attributes", names=["opt_param"]
                        ),
                        Field("required", label="Tag Required", names=["required"]),
                    ],
                ),
            ),
        )
    )
