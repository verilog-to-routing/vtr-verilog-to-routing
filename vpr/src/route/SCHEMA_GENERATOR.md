## Schema generator instructions

The rrgraph reader and writer is now generated via uxsdcxx and the
`rr_graph.xsd` file.  The interface between the generated code and the VPR is
mediated via RrGraphBase located in `rr_graph_uxsdcxx_interface.h`.

If `rr_graph.xsd` is modified, then the following files must be updated:

 - `vpr/src/route/gen/rr_graph_uxsdcxx.h`
 - `vpr/src/route/gen/rr_graph_uxsdcxx_capnp.h`
 - `vpr/src/route/gen/rr_graph_uxsdcxx_interface.h`
 - `libs/libvtrcapnproto/rr_graph_uxsdcxx.capnp`

### Instructions to update generated files (using CMake)

1. Run target `generate_rr_graph_serializers`, e.g. run `make generate_rr_graph_serializers`.
2. Run target `format`, e.g. run `make format`.
3. Update `vpr/src/route/rr_graph_uxsdcxx_interface_impl.h`, implement or
   update interfaces that are new or are changed.  The compiler will complain
   that virtual methods are missing if the schema has changed.

### Instructions to update generated files (manually)

1. Clone https://github.com/duck2/uxsdcxx/
1. Checkout interface-consumer branch
2. Run `python3 -mpip install --user -r requirements.txt`
3. Run `python3 uxsdcxx.py vpr/src/route/rr_graph.xsd`
3. Run `python3 uxsdcap.py vpr/src/route/rr_graph.xsd`
4. Copy `rr_graph_uxsdcxx.h`, `rr_graph_uxsdcxx_capnp.h`,
   `rr_graph_uxsdcxx_interface.h` to `vpr/src/route/`
5. Copy `rr_graph_uxsdcxx.capnp` to `libs/libvtrcapnproto/`
6. Run `make format`
7. Update `vpr/src/route/rr_graph_uxsdcxx_interface_impl.h`, implement or
   update interfaces that are new or are changed.  The compiler will complain
   that virtual methods are missing if the schema has changed.
