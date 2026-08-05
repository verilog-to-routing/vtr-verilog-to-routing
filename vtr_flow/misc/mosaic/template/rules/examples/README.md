# Example exotic techmap templates

These `.tmpl` files are not loaded automatically. Point at them from an arch
`arch_config.tcl` with `exoticTemplatePairs`, for example:

```tcl
set exoticTemplatePairs {
    {mult_fp_16 $templateDir/rules/examples/mult_fp_16_passthrough.v.tmpl}
}
```

`mult_fp_16_passthrough.v.tmpl` shows scanned `@PORT_*@` / `@MODEL_NAME@`
tokens and an identity replace. Adapt the body to bind Yosys ops if you need
inference rather than RTL-instantiated passthrough.
