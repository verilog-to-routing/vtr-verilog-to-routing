# mosaic primitive profiles are named policy packs.
# synthesis.tcl sources this file before arch_config so an arch can override
# primitiveProfile, and mosaicApplyPrimitiveProfile runs after arch_config so
# the chosen pack can still force stubAllHardblocks when the arch left it unset.
set mosaicProfileData [dict create \
    vtr_classic [dict create \
        forceStubAll 0] \
    passthrough_exotics [dict create \
        forceStubAll 1]]

proc mosaicKnownProfiles {} {
    global mosaicProfileData
    return [lsort [dict keys $mosaicProfileData]]
}

# USE: applies the selected primitiveProfile pack onto live synth knobs.
proc mosaicApplyPrimitiveProfile {} {
    global primitiveProfile stubAllHardblocks mosaicProfileData
    if { ![dict exists $mosaicProfileData $primitiveProfile] } {
        error "mosaic: unknown primitiveProfile '$primitiveProfile' (known: [join [mosaicKnownProfiles] {, }])"
    }
    set profileInfo [dict get $mosaicProfileData $primitiveProfile]
    if { [dict get $profileInfo forceStubAll] } {
        set stubAllHardblocks 1
    }
}

# USE: warn when inferred $mul cannot bind to a hard multiply.
# rule gen already warns, and this surfaces the same contract in the synth log
# so designers see why behavioral multiply stayed soft.
proc mosaicCheckClassicMulContract {} {
    global multiplyPresent stubAllHardblocks exoticRoles exoticTemplatePairs \
        primitiveProfile
    if { $multiplyPresent } {
        return
    }
    if { [llength $exoticRoles] > 0 || [llength $exoticTemplatePairs] > 0 } {
        return
    }
    if { $stubAllHardblocks } {
        log -warning "mosaic: profile '$primitiveProfile' has no classic multiply; inferred \$mul stays soft. rtl-instantiated exotic cells passthrough when stubAllHardblocks is on; bind \$mul with exoticRoles or exoticTemplatePairs."
        return
    }
    log -warning "mosaic: profile '$primitiveProfile' has no classic multiply and no exotic mul binding; inferred \$mul stays soft."
}
