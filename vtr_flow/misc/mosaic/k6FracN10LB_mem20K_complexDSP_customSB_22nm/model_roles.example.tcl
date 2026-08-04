# example exotic role configuration for mosaic synthesis.
# copy relevant lines into arch_config.tcl when an exotic model has classic-
# shaped ports (e.g. a/b/out for integer_mul).
#
# synthesis.tcl passes each {model role} pair to vtr_arch_rules -exotic-role.
# stock templates live under template/rules/roles/<role>_map.v.tmpl.
#
# working smoke fixture (no classic multiply):
#   mosaic/tests/fixtures/min_exotic_integer_mul.xml
#   set exoticRoles {{my_mul integer_mul}}
#
# koios complex-dsp cells (mult_fp_16, mac_fp_16, ...) do not expose a/b/out;
# use stubAllHardblocks + rtl instantiation, or a per-model -exotic template.

set exoticRoles {}
# set exoticRoles {{my_mul integer_mul}}
