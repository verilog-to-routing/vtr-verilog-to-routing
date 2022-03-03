# RRGraph Folding
This directory is used for storing / folding routing resource graphs using different folding methods.

## Files / Directories
- `flat_graphs/` is the directory where the folding scripts will look for the flat rr_graph.xml files
    - to save flat rr_graphs to this directory, run vpr with the argument 
    ```
    --write_rr_graph /path/to/flat_graphs/your_graph.xml
    ```
    - for convenience, the [rr_graph](https://docs.verilogtorouting.org/en/latest/quickstart/#running-vpr-on-a-pre-synthesized-circuit) for architecture EArch.xml and circuit tseng.blif is already included in `flat_graphs/`
- `folded_graphs/` is the directory where the folding scripts will save the folded rr_graph.xml files
    - to generate folded rr_graphs, follow the instructions below
- `fold_rr_graph.py` is the script that takes care of folding and saving rr_graphs from flat to folded

## Generating Folded RR Graphs
- To generate a folded rr_graph, run `fold_rr_graph.py` with the following command
```
python fold_rr_graph.py <folding_method> <flat_graph_name>
```
- the `<folding_method>` argument refers to the folding method script in the `folding_methods/` directory to be used for folding
- the `<flat_graph_name>` refers to the name of the flat graph in the `flat_graphs` directory that you wish to fold
- If you want to fold all the graphs in the `flat_graphs` directory, simply omit the `<flat_graph_name>` argument, and every flat graph will be folded.
- `fold_rr_graph.py` will execute in the following order
    1. Read flat rr_graph into memory from `flat_graphs/<flat_graph_name>.xml`
    2. Fold the flat rr_graph using the desired `<folding_method>`
    3. Save the folded rr_graph as `<folding_method>_<flat_graph_name>.xml` in the `folded_graphs` directory
    4. Print out folding metrics to the console including data such as nodes and edges % of original size.

## Using Folded RR Graphs in VTR
- Currently the only working method to use a folded rr_graph in VTR requires that the folded graph is read from a file.
- This means that a custom rr_graph reader must be implemented for each folding method.
- Instructions on how to implement changes to the rr_graph reader can be found at `$VTR_ROOT/vpr/src/route/SCHEMA_GENERATOR.md`

# Working Example

- Branches for each folding method follow the naming convention that the git branch name is the same as the folding method name.
- A working example of the `nodes_all_attr` folding method can be found on the nodes_all_attr branch. (add link...)
- To run this example, simply checkout the branch, build VTR and then run the following command
```
This is the command...
```
- To compare with the flat graph, checkout this (add link...) branch, build VTR and then run the following command
```
This is the command...
```