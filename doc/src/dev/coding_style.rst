
Coding Style
=============

.. note::

   Many of the rules and conventions described in this document are not yet followed consistently throughout the codebase.
   They represent the direction we want to move in.
   Contributors are encouraged to adopt these guidelines in new code and when modifying existing code,
   so that the project gradually becomes more consistent and maintainable over time.

Header Guard Convention
~~~~~~~~~~~~~~~~~~~~~

- Always use ``#pragma once`` as the first line in every header file, even before any file-level comments or code.

  It is recommended that ``#pragma once`` appears before the file comment.
  The reason is that file comments may be duplicated each time the header is included,
  which can slow down compilation in large codebases due to extra file I/O.

  Example:

  .. code-block:: cpp

      #pragma once

      /**
       * @file netlist_walker.h
       * @brief Utilities for traversing the netlist.
       */
      // ...rest of header...

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



Commenting Style
~~~~~~~~~~~~~~~~

Comments help readers understand the purpose, structure, and reasoning behind the code.
This section outlines when and how to use comments in a consistent and helpful way.

General Rules
-------------

- In **header files (`.h`)**, use comments to document *what* each API, class, struct, or function does.
  If the purpose, design, or behavior is not obvious, also explain *why* it is structured that way.
- In **implementation files (`.cpp`)**, use comments to clarify *how* the code works,
  especially for non-obvious algorithms, tricky logic, or important implementation details.
  Only add comments where the code itself is not self-explanatory.
- Do not restate what the code is plainly doing in the comments.
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

.. note::

   This strict separation between `/* ... */` and `//` comments is designed to improve readability and reduce mistakes.
   Using `/* ... */` only for Doxygen documentation makes it clear at a glance which comments are meant for generated docs,
   and which are for developers reading the code. When Doxygen (or similar) comments are visually distinct from inline implementation comments,
   it's harder to accidentally generate incomplete or misleading API documentation.


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

- For **static functions**, the comment describing *what* the function does should appear at the function's declaration
  (usually at the top of the `.cpp` file), not at its definition. Do not repeat the *what* comment at the implementation.
  Only add comments to the definition if there are non-obvious details about *how* or *why* the code works as it does.

  Example:

  .. code-block:: cpp

      // Calculates the bounding box wirelength for a net.
      static float estimate_wirelength(ClusterNetId net_id);

      static float estimate_wirelength(ClusterNetId net_id) {
          // Use HPWL to estimate the wirelngth
          // The estimated WL is adjusted based the net fanout
          ...
      }




Avoid Unnecessary Complexity
~~~~~~~~~~~~~~~~~~~~~~~~~~~

- Prefer easier-to-read language features and idioms.
- Avoid overusing advanced C++ features (e.g., template metaprogramming, operator overloading, SFINAE) unless they are essential.
- Write for clarity first; optimize for performance or conciseness only if needed and after measuring.

.. code-block:: cpp

    // Prefer this
    const std::vector<int>& get_ids() const;

    // Avoid this unless you truly need it
    template<typename T>
    auto&& get_ids() const && noexcept;


- **Write short functions.** Functions should do one thing. Short functions are easier to understand, test, and reuse, and their purpose can be clearly described in a concise comment or documentation block.
- **Limit function length.** If a function is growing too long or is difficult to describe in a sentence or two, consider splitting it into smaller helper functions.
- **Favor simplicity.** Avoid clever or unnecessarily complex constructs. Code should be easy to read and maintain by others, not just the original author.
- **Avoid deep abstract class hierarchies.** Excessive layering of virtual base classes makes the code hard to navigate and understand. Many tools (e.g., IDEs, LSPs) struggle to follow virtual call chains, especially in large or templated codebases.
- **Avoid mixing templates with virtual functions.** This combination is particularly difficult to reason about and navigate. If you need to write generic code, prefer using either virtual dispatch or templates, but not both in the same interface.
- Before adding new functions, classes, or utilities, check the codebase and documentation to see if a similar utility already exists.
- Reuse or extend existing routines instead of duplicating functionality. This reduces bugs and makes the codebase more maintainable.



Group Related Data
~~~~~~~~~~~~~~~~~~

- Group related data members into `structs` or `classes` rather than passing multiple related parameters separately.
- Each data member and member function should be commented to explain its role, even in simple data structures.

.. code-block:: cpp

    /**
     * @brief Data about a routing node.
     */
    struct t_node_info {
        int id;            ///< Unique identifier.
        float cost;        ///< Routing cost.
        bool expanded;     ///< True if this node has been expanded.
    };

    // Instead of:
    void process_node(int node_id, float cost, bool expanded);

    // Prefer:
    void process_node(const t_node_info& info);

.. note::

   Organizing related data and routines in structs or classes with clear comments makes code easier to extend and understand.
   It also helps avoid errors from mismatched or misused arguments.



Assertion and Safety Check
~~~~~~~~~~~~~~~~

Assertions help catch bugs early by checking that assumptions hold at runtime.
Consistent use of assertions improves code reliability and helps developers identify problems close to their source.

General Guidelines
------------------

- Use assertions and data structure validators wherever possible.
- Prefer using `VTR_ASSERT` for checks that are inexpensive and should be enforced even in release builds.
- In CPU-critical or performance-sensitive code, use `VTR_ASSERT_SAFE` for potentially expensive checks.
  These checks are disabled in release builds but are useful during development and debugging.

.. code-block:: cpp

    // Cheap check: always on
    VTR_ASSERT(ptr != nullptr);

    // Expensive check: enabled only in debug/development builds
    VTR_ASSERT_SAFE(is_valid_graph(rr_graph));

- Use assertions to document the assumptions and constraints in your code.
- Prefer placing assertions as close as possible to where they might have been violated.

.. note::

   For more on assertion macros and their behavior, see :ref:`vtr_assertion` for more details.


Logging and Error Reporting
~~~~~~~~~~~~~
Use the VTR logging and error handling utilities instead of raw `printf`, `std::cerr`, `exit()`, or `throw`.

- Use `VTR_LOG`, `VTR_LOG_WARN`, and `VTR_LOG_ERROR`


.. code-block:: cpp

    VTR_LOG("Incr Slack updates %zu in %g sec\n", incr_slack_updates, incr_slack_update_time_sec);

    if (check_route_option == e_check_route_option::OFF) {
        VTR_LOG_WARN("The user disabled the check route step.");
        return;
    }

    if (total_edges_to_node[inode] != 0) {
        VTR_LOG_ERROR("in check_rr_graph: SOURCE node %d has a fanin of %d, expected 0.\n", inode, total_edges_to_node[inode]);
    }


Refer to :doc:`Logging - Errors - Assertions</api/vtrutil/logging>` to learn more about logging and error reporting.



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