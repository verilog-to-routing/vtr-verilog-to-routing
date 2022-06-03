===============
Netlist mapping
===============
As shown in the previous section, there are multiple levels of abstraction (multiple netlists) in VPR which are the ClusteredNetlist and the AtomNetlist. To fully use these netlists, we provide some functions to map between them. 

In this section, we will state how to map between the atom and clustered netlists.

Block Id
--------

Atom block Id to Cluster block Id
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
To get the block Id of a cluster in the ClusteredNetlist from the block Id of one of its atoms in the AtomNetlist:

* Using AtomLookUp class

.. code-block:: cpp

    ClusterBlockId clb_index = g_vpr_ctx.atom().lookup.atom_clb(atom_blk_id);


* Using re_cluster_util.h helper functions
    
.. code-block:: cpp

    ClusterBlockId clb_index = atom_to_cluster(atom_blk_id);


Cluster block Id to Atom block Id
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
To get the block Ids of all the atoms in the AtomNetlist that are packed in one cluster block in ClusteredNetlist:

* Using ClusterAtomLookup class

.. code-block:: cpp

    ClusterAtomsLookup cluster_lookup;
    std::vector<AtomBlockId> atom_ids = cluster_lookup.atoms_in_cluster(clb_index);

* Using re_cluster_util.h helper functions

.. code-block:: cpp

    std::vector<AtomBlockId> atom_ids = cluster_to_atoms(clb_index);


Net Id
------

Atom net Id to Cluster net Id
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
To get the net Id in the ClusteredNetlist from its Id in the AtomNetlist, use AtomLookup class as follows

.. code-block:: cpp

   ClusterNetId clb_net = g_vpr_ctx.atom().lookup.clb_net(atom_net);


Cluster net Id to Atom net Id
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
To get the net Id in the AtomNetlist from its Id in the ClusteredNetlist, use AtomLookup class as follows

.. code-block:: cpp

   ClusterNetId atom_net = g_vpr_ctx.atom().lookup.atom_net(clb_net);
