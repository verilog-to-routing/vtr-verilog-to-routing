Explanation of Architectural features:


Channels:
    - 

Wire Segments:
    The following is wrong (I think):
    - Wires that span through an FPGA channel.
    - They are of various lengths and their start and end can be connected to other wires through switch blocks. 
    - A wire cannot have a connection somewhere that is not one of its end.
    - However, the from_set and to_set parameters can be used to shift their channel passing through a switchblock, therefore making them not exactly straight. Eg. Using the parameter a wire passing through track 1 at CHANX (1,1) could be connected to track 3 at CHANX (2,1) if defined in the architecture.

    This is true I think, but only assuming that we used populated SB, this means wire passing through multiple SBs and having switchpoint at these SBs and not only at their endpoints:
    - Wires that span through an FPGA channel.
    - They are of various lengths and can be connected to other wires through switchblocks. 
    - When defining a switchblock, the permutation are applied to the wires switchpoints.
    - <wireconn.../> allows the user to set which set of wire type (wiresegment previously defined) connect to which other type as well as specifying exactly which of their switchpoint should connect which other switchpoint.
    - Each wireconn defined will follow the same SB permutation but will apply to differently set sets of wires.
    - Having multiple wireconn that uses some of the same types and switchpoints will create overlaping connections with different permutations depending on the sizes of the to/from sets. 
    - Eg. If we have wireconn A and B where both have uses the same switchpoints of wire X and Y but where B also uses wire Z, the same permutation formulas will be used but A and B won't result in the same connections as B has a larger set of to/from. If B didn't have the switchpoint of Z, A and B would result in an overlapping pattern as their permutation is the same and would apply the same size of sets.

Switchpoint:
    - This refers to points at which a switch exists and therefore where a connection can happen.
    - The depopulation specifies where switchpoints should be along a wire. 
    - This is true for both CB and SB depopulation.

CB/SB population/depopulation:
    - This is the quantity of connections made by a wire to the connect boxes it passes through.
    - By definition, a wire of length L can pass through L number of CB and L+1 number of SB.
    - In VTR, the depopulation of a L=6 wire is specified as follow: <sb type="pattern">1 0 1 0 1 0 1</sb> and <cb type="pattern">1 0 1 0 1 1</cb> with 0 specifying a pass through and 1 specifying a connection.