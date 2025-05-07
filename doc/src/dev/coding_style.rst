
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
