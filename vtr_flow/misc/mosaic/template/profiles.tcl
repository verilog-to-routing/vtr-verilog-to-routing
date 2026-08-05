# mosaic primitive profiles as data (not comments).
# sourced by synthesis.tcl before arch_config so policy can override
# primitiveProfile; mosaicApplyPrimitiveProfile runs after arch_config.

set mosaicProfileData [dict create \
    vtr_classic [dict create \
        requireClassicRams 1 \
        forceStubAll 0 \
        inferClassicMulAdd 1] \
    passthrough_exotics [dict create \
        requireClassicRams 1 \
        forceStubAll 1 \
        inferClassicMulAdd 1]]

proc mosaicKnownProfiles {} {
    global mosaicProfileData
    return [lsort [dict keys $mosaicProfileData]]
}

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

# after arch_facts: warn when classic inference cannot bind $mul but exotics exist.
# rule-gen also warns; this surfaces the profile contract in the synth log.
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
