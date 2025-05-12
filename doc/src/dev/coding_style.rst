
Coding Style
=============

Use of `auto`
~~~~~~~~~~~~~

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
~~~~~~~~~~~~~~~~

Comments help readers understand the purpose, structure, and reasoning behind the code.
This section outlines when and how to use comments in a consistent and helpful way.

General Rules
-------------

- Focus on explaining *why* the code exists or behaves in a certain way.
- Do not explain *what* the code is doing if it's already clear from the code itself.
- Keep comments up to date. Outdated comments are worse than no comments.
- Use Doxygen-style `/** ... */` or `///` for documenting APIs, classes, structs, members, and files.

Comment types and rules:

- Use `/* ... */` **only** for Doxygen documentation comments.
  Do **not** use block comments (`/* */`) to describe code logic or individual lines in function bodies.

- Use `//` for all regular comments within function bodies or implementation code.

Formatting rules for `//` comments:

- Always include a space between `//` and the comment text.
- Use full sentences when appropriate.
- For multi-line comments, prefix each line with `// `.

Incorrect usage:

.. code-block:: cpp

    /* Check if visited */  // Block comments are not allowed here
    if (visited[node_id]) {
        return;
    }

    //Missing space
    //inconsistent formatting

    /* Non-Doxygen block comment */  // Not permitted

When to Comment
---------------

**1. File-Level Comments**
- Every source/header file should begin with a brief comment explaining the overall purpose of the file.

**2. Classes and Structs**
- Add a comment describing what the class or struct is for.
- Comment every member variable to explain its role.

Example:

.. code-block:: cpp

    /**
     * @brief Manages TCP connections for a server.
     */
    class ConnectionManager {
    public:
        /**
         * @brief Starts listening for incoming connections.
         */
        void Start();

    private:
        int port_;          ///< Listening port.
        bool is_running_;   ///< Whether the server is active.
    };


**3. Functions**
- All non-trivial functions must have a Doxygen comment in the header file or on the static declaration.
- Explain what the function does, its parameters, and its return value.

Example:

.. code-block:: cpp

    /**
     * @brief Estimates the wirelength of a net using bounding box.
     *
     * @param net_id ID of the net to analyze.
     * @return Estimated wirelength in units.
     */
    float estimate_wirelength(ClusterNetId net_id);

Example:

.. code-block:: cpp

    // Skip ignored nets
    if (net.is_ignored()) {
        continue;
    }





