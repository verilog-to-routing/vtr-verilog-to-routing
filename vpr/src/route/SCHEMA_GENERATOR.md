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

### Defining the serialization interface object

The code generator emits an abstract base class in
`rr_graph_uxsdcxx_interface.h` that must be implemented to bind VPR to the
generated code.  Roughly half of the methods are for the reader interface
(from disk to VPR) and half of the methods are for the writer interface (from
VPR to disk).

The methods starting with `preallocate_`, `add_`, `init_`, and `finish_`
are associated with the reader interface.  The reader interface has three additionals methods, `start_load`, `finish_load` and `error_encountered`.

The methods starting with `get_`, `has_` and `size_` are associated with
the writer interface.  The writer interface has two additionals methods, `start_write`, and `finish_write`.

#### Load interface

##### Start, finish, and error handling

The first method called upon a read is `start_load`.  `start_load` has one parameter, a
`std::function` called `report_error`.

This function should be called if an error is detected in the file.  Calling
this function will pass control the reader implementation.  The reader
implementation can then add context about where the error occur (e.g. file and
line number).

The `error_encountered` method is used by the reader implementation to
declare that an error has occurred during parsing.  That error may be generated
via the reader implementation itself, or by the load interface class by calling
the `report_error` function.

`error_encountered` is expected to throw some form of C++ exception to unwind
the parsing stack.  If `error_encountered` does not throw an exception, the
reader implementation will throw a `std::runtime_error` exception.

The `std::function` based into `start_load` will have a lifetime that exists
until after the `finish_load` method is called.  The `finish_load` method
will be called after loading is complete and no error was encountered.

If an error is detected by either the reader implementation or the
implementation class, the `error_encountered` will be invoked. After
`error_encountered` is invoked, then no further methods will be invoked until
a new load is started.

##### Method invocation order during read

During a load, the reader interface will invoke the following methods in order:

1. (optional) The reader implementation may call `preallocate_` on instances
   that can have size.  The implementation class should not require this
   method be called, in the event that the reader implementation does not have
   size information available.
2. The reader implementation will call with `init_` or `add_` with any
   required non-string attributes.  Required string attributes and other
   optional attributes will be set next.
3. The reader implementation will call `set_` with any required string or
   optional attributes present on the object.
4. If the object has children, the reader will return to #1 with the child
   objects.
5. Once all children of the current object have been read and the
   implementation class methods have been invoked, the `finish_` method will
   be called.

##### Context variables

Context variables are available to allow the implementation class to keep
state between method invocations.  By default all context variables are
`void *`, but they can be changed to any type via the template parameter
`ContextTypes`.

The root tag context variable is passed to the first load function
`load_rr_graph_xml`.  Context variables are then introduced whenever an
`init_` or `add_` call is made, used during `set_` calls, and the `finish_`
method call.  After the `finish_` method, the context variable leaves scope
and is destroyed.

When descending into a child object, the parents context variable is passed to
the child's `preallocate_`, `init_` or `add_` method calls.

##### Load Example

For example:

Consider an object tree like:

```
<rr_graph tool_comment="Comment">
  <rr_nodes>
    <node id="0" type="CHANX"/>
    <node id="1" type="CHANX"/>
    <node id="2" type="CHANX">
      <metadata>
        <meta name="fasm_feature">CLB</meta>
      </metadata>
    </node>
  </rr_nodes>
</rr_graph>
```

The method call order will be:

1. `start_load` invoked with a `report_error` `std::function`.
2. `set_rr_graph_tool_comment` with the initial context variable passed to
   `load_rr_graph_xml`.
3. `init_rr_graph_rr_nodes` with the initial context variable passed to
   `load_rr_graph_xml`.  The `init_rr_graph_rr_nodes` method returns the
   new context variable to be used for `rr_nodes`, lets call it
   `rr_nodes_context`.
4. (optional) `preallocate_rr_nodes_node` with size=3 and the context variable
   `rr_nodes_context`.
5. `add_rr_nodes_node` with the context variable `rr_nodes_context`, with
   id=0, and type=CHANX.  `add_rr_nodes_node` will return a new context
   variable, `node0_context`.
6. `finish_rr_nodes_node` with the context variable `node0_context`.
7. `node0_context` will leave scope and be destroyed.
8. `add_rr_nodes_node` with the context variable `rr_nodes_context`, with
   id=1, and type=CHANX.  `add_rr_nodes_node` will return a new context
   variable, `node1_context`.
9. `finish_rr_nodes_node` with the context variable `node1_context`.
10. `node1_context` will leave scope and be destroyed.
11. `add_rr_nodes_node` with the context variable `rr_nodes_context`, with
   id=1, and type=CHANX.  `add_rr_nodes_node` will return a new context
   variable, `node2_context`.
12. `init_node_metadata` with the context variable `node2_context`, and will
    return new context variable `node2_metadata_context`.
13. `add_metadata_meta` with the context variable `node2_metadata_context`,
    returning `node2_metadata_meta_context`.
14. `set_meta_name` with the context variable `node2_metadata_meta_context`,
    and name="fasm_feature".
14. `set_meta_value` with the context variable
    `node2_metadata_meta_context` and value="CLB".
15. `finish_metadata_meta` with the context variable  `node2_metadata_meta_context`.
16. `node2_metadata_meta_context` will leave scope and be destroyed.
17. `finish_node_metadata` with the context variable `node2_metadata_context`.
18. `finish_rr_nodes_node` with the context variable `node2_context`.
19. `node2_context` will leave scope and be destroyed.
20. `finish_rr_graph_rr_nodes` with the context variable with the initial
    context variable passed to `load_rr_graph_xml`.
21. `finish_load` invoked

#### Write interface

##### Method invocation order during write

During a writer, the writer interface will invoke the following methods in order:

1. For scalar optional fields, the `has_` method will be invoked.  If the
   `has_` method returns false, that field and/or object will be skipped
   during writing.
   For non-scalar fields, the `num_` method will be invoked.
2. The `get_` method will be invoked with the parents context variable.  The
   `get_` method should return a new context variable for the relevant object.
3. For each required attributes, a `get_` method will be called with the
   current context variable.
4. For each optional attribute, a `has_` method will be called with the
   current context variable, followed by a `get_` method if the `has_`
   returns true.
5. For any children, 1-4 repeats.
6. If this object is part of a non-scalar field, 2-5 repeats for each remaining
   item.

##### Write Example

For example:

Consider writing an object tree like:

```
<rr_graph tool_comment="Comment">
  <rr_nodes>
    <node id="10" type="CHANX"/>
    <node id="23" type="CHANX"/>
    <node id="50" type="CHANX">
      <metadata>
        <meta name="fasm_feature">CLB</meta>
      </metadata>
    </node>
  </rr_nodes>
</rr_graph>
```

The method call order will be:

1. `start_write` invoked
2. `get_rr_graph_tool_comment` invoked with the initial context variable passed
    to `write_rr_graph_xml`, returning "Comment".  Note that the returned
    string pointer must live at least until the next implementation method
    call.
3. `get_rr_graph_rr_nodes` invoked with the initial context variable passed
    to `write_rr_graph_xml`, returning a new context variable
    `rr_nodes_context`.
4. `num_rr_nodes_node` invoked with context variable `rr_nodes_context`,
   returns 3.
5. `get_rr_nodes_node` invoked with n=0 and context variable
   `rr_nodes_context`, returns context variable `node0_context`.
6. `get_node_id` invoked with context variable `node0_context`, returns 10.
7. `get_node_type` invoked with context variable `node0_context`, returns CHANX.
8. `has_node_metadata` invoked with context variable `node0_context`, returns false.
9. `node0_context` leaves scope and is destroyed.
10. `get_rr_nodes_node` invoked with n=1 and context variable
   `rr_nodes_context`, returns context variable `node1_context`.
11. `get_node_id` invoked with context variable `node1_context`, returns 23.
12. `get_node_type` invoked with context variable `node1_context`, returns CHANX.
13. `has_node_metadata` invoked with context variable `node1_context`, returns false.
14. `node1_context` leaves scope and is destroyed.
15. `get_rr_nodes_node` invoked with n=2 and context variable
   `rr_nodes_context`, returns context variable `node2_context`.
16. `get_node_id` invoked with context variable `node2_context`, returns 50.
17. `get_node_type` invoked with context variable `node2_context`, returns CHANX.
18. `has_node_metadata` invoked with context variable `node2_context`, returns true.
19. `get_node_metadata` invoked with context variable `node2_context`, returning `node2_metadata_context`.
20. `num_metadata_meta` invoked with context variable `node2_metadata_context`, returns 1.
21. `get_metadata_meta` invoked with n=0 and context variable `node2_metadata_context`, returns `node2_metadata_meta_context`.
22. `get_meta_name` invoked with context variable `node2_metadata_meta_context`, returns "fasm_feature".
23. `get_meta_value` invoked with context variable `node2_metadata_meta_context`, returns "CLB".
24. `node2_metadata_meta_context` leaves scope and is destroyed.
25. `node2_metadata_context` leaves scope and is destroyed.
26. `node2_context` leaves scope and is destroyed.
27. `finish_write` invoked
