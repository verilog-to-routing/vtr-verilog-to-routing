###################################################
# Architecture Files
###################################################

This directory contains sample architecture files.

The vast majority of users will want to use our flagship architecture file found here:
timing/k6_frac_N10_mem32K_40nm.xml

This architecture file describes a modern-style architecture somewhat aligned to 
a commercial 40nm Stratix IV FPGA. Our flagship architecture contains 
fracturable 6-LUTs, configurable 32Kb memories, and fracturable 36x36 
multipliers. Details about delay, area, and power modelling are found in the 
comments within that file.

For those interested in other architectures, we have organized our architectures 
into three categories: non-timing-driven architectures, power-only 
architectures, and timing-driven architectures.

# Timing-driven architectures

We have included several variations of our flagship architecture.  The most 
important one being an architecture with carry chain support 
(k6_frac_N10_frac_chain_mem32K_40nm.xml).  This architecture file serves as a 
proof-of-concept on how carry chains may be used in our CAD flow.  This 
architecture should only be used by advanced CAD/architecture researchers 
because limitations in our current logic synthesis engine results in netlists 
that are bigger than what they should be when targeting this particular architecture.

We have included a few simple architecture files and an architecture file with 
embedded floating-point-cores to demonstrate how one can specify a large range 
of different architecture files using our framework.  The delay models used here
are somewhat reasonable but not as well vetted as our flagship architecture.

# Power-only architectures

These architecture files serve as examples on how to specify power models in VTR.

# Non-timing-driven architectures

These architecture files serve as examples on how to specify a range of 
different fracturable LUT and configurable memory architectures.  The intention
is to provide more examples of the specification language.  As these architecture files
do not have delay models, we don't recommend drawing conclusions from them.
