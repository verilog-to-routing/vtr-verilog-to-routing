
Coding Style
=============

Naming Conventions
~~~~~~~~~~~~~

Consistent naming makes code easier to read, search, and maintain.
The following rules help enforce a uniform naming style throughout the codebase:

General Rules
-------------

- Use descriptive and unambiguous names.
- Avoid abbreviations unless they are widely understood (e.g., `id`, `ctx`, `cfg`).
- Prefer clarity over brevity.

Specific Conventions
--------------------

- **Class names** use `PascalCase`.

  .. code-block:: cpp

      class DecompNetlistRouter {
      };

- **Variable and function names** use `snake_case`.

  .. code-block:: cpp

      int net_count;
      void estimate_wirelength();

- **Private member variables and private methods** should end with an underscore `_`.

  .. code-block:: cpp

      class Example {
      private:
          int count_;
          void update_state_();
      };

- **Enums** should use `enum class` instead of traditional enums. Enum names should start with `e_`.

  .. code-block:: cpp

      enum class e_heap_type {
          INVALID_HEAP = 0,
          BINARY_HEAP,
          FOUR_ARY_HEAP,
      };

- **Trivial structs** that primarily store data without behavior should be prefixed with `t_`.

  .. code-block:: cpp

      struct t_dijkstra_data {
          vtr::vector<RRNodeId, bool> node_expanded;
          vtr::vector<RRNodeId, float> node_visited_costs;
          std::priority_queue<PQ_Entry> pq;
      };

- **Source and header file names** use `snake_case` and should be short but descriptive.

  Example: `router_lookahead_map.cpp`, `netlist_walker.h`


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





