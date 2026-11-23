# example exotic techmap templates

these `.tmpl` files are not loaded automatically. point at them from an arch
`arch_config.tcl` with `exoticTemplatePairs`, for example:

```tcl
set exoticTemplatePairs {
    {mult_fp_16 $templateDir/rules/examples/mult_fp_16_passthrough.v.tmpl}
}
```

`mult_fp_16_passthrough.v.tmpl` shows scanned `@PORT_*@` / `@MODEL_NAME@`
tokens and an identity replace. adapt the body to bind yosys ops if you need
inference rather than rtl-instantiated passthrough.
