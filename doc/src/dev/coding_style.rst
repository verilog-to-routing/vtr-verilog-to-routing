
Use of `auto`
=============

Use `auto` only when the type is long, complex, or hard to write explicitly.

Examples where `auto` is appropriate:

.. code-block:: cpp

    auto it = container.begin();  // Iterator type is long and not helpful to spell out

    // The return type is std::vector<std::vector<std::pair<int, float>>>
    auto matrix = generate_adjacency_matrix();

    // Lambdas have unreadable and compiler-generated types — use auto for them
    auto add = [](int x, int y) { return x + y; };


Avoid `auto` when the type is simple and clear:

.. code-block:: cpp

    // Use type names when they are short and readable.
    for (RRNodeId node_id : device_ctx.rr_graph.nodes()) {
        t_rr_node_route_inf& node_inf = route_ctx.rr_node_route_inf[rr_id];
    }

    int count = 0;
    std::string name = "example";
    std::vector<int> numbers = {1, 2, 3};

Avoid:

.. code-block:: cpp

    auto count = 0;                // Simple and obvious type
    auto name = std::string("x");  // Hides a short, clear type

    for (auto node_id : device_ctx.rr_graph.nodes()) {
        // node_id is RRNodeId. Write it out for clarity.
        auto& node_inf = route_ctx.rr_node_route_inf[rr_id];
        // node_inf is t_rr_node_route_inf. Use the explicit type since it's simple and meaningful.
    }

Rationale: clear, explicit types help with readability and understanding. Avoid hiding simple types behind `auto`.


Commenting Style
================
Comment types and rules:

- Use `/* ... */` **only** for Doxygen documentation comments.
  Do **not** use block comments (`/* */`) to describe code logic or individual lines in function bodies.

- Use `//` for all regular comments within function bodies or implementation code.

Formatting rules for `//` comments:

- Always include a space between `//` and the comment text.
- Use full sentences when appropriate.
- For multi-line comments, prefix each line with `// `.

Examples (correct usage):

.. code-block:: cpp

    // Check if the node has already been visited
    if (visited[node_id]) {
        return;
    }

    // Enable debug output for route estimation
    bool debug_enabled = true;

    /**
     * @brief Estimates the wirelength of a net using bounding box.
     *
     * @param net_id ID of the net to analyze.
     * @return Estimated wirelength in units.
     */
    float estimate_wirelength(ClusterNetId net_id);

Incorrect usage:

.. code-block:: cpp

    /* Check if visited */  // Block comments are not allowed here
    if (visited[node_id]) {
        return;
    }

    //Missing space
    //inconsistent formatting

    /* Non-Doxygen block comment */  // Not permitted
