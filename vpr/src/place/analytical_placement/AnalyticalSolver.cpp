#include "AnalyticalSolver.h"
#include <Eigen/Sparse>
#include <Eigen/IterativeLinearSolvers>
#include "atom_netlist.h"
#include "vtr_assert.h"

static inline void populate_hybrid_matrix(Eigen::SparseMatrix<double> &A_sparse,
                                          Eigen::VectorXd &b_x,
                                          Eigen::VectorXd &b_y,
                                          const AtomNetlist& netlist, 
                                          PartialPlacement& p_placement) {
    std::vector<Eigen::Triplet<double>> tripletList;
    size_t num_moveable_nodes = p_placement.num_moveable_nodes;
    tripletList.reserve(num_moveable_nodes * netlist.nets().size());

    size_t star_node_offset = 0;
    // FIXME: Instead of iterating over the whole nelist and reverse looking up
    //        it may make more sense to pre-compute the netlist.
    for (AtomNetId net_id : netlist.nets()) {
        if (p_placement.net_is_ignored_for_placement(net_id))
            continue;
        int num_pins = netlist.net_pins(net_id).size();
        VTR_ASSERT(num_pins > 1);
        if (num_pins > 3) {
            // FIXME: THIS WAS DIRECTLY COPIED FROM THE STAR FORMULATION. MOVE TO OWN FUNCTION.
            //          (with the exeption of the star node offset).
            // Using the weight from FastPlace
            double w = static_cast<double>(num_pins) / static_cast<double>(num_pins - 1);
            size_t star_node_id = num_moveable_nodes + star_node_offset;
            for (AtomPinId pin_id : netlist.net_pins(net_id)) {
                AtomBlockId blk_id = netlist.pin_block(pin_id);
                size_t node_id = p_placement.block_id_to_node_id[blk_id];
                // Note: the star node is always moveable
                if (p_placement.is_moveable_node(node_id)) {
                    tripletList.emplace_back(star_node_id, star_node_id, w);
                    tripletList.emplace_back(node_id, node_id, w);
                    tripletList.emplace_back(star_node_id, node_id, -w);
                    tripletList.emplace_back(node_id, star_node_id, -w);
                } else {
                    tripletList.emplace_back(star_node_id, star_node_id, w);
                    b_x(star_node_id) += w * p_placement.node_loc_x[node_id];
                    b_y(star_node_id) += w * p_placement.node_loc_y[node_id];
                }
            }
            star_node_offset++;
        } else {
            // FIXME: THIS WAS DIRECTLY COPIED FROM THE CLIQUE FORMULATION. MOVE TO OWN FUNCTION.
            // Using the weight from FastPlace
            double w = 1.0 / static_cast<double>(num_pins - 1);

            for (int ipin = 0; ipin < num_pins; ipin++) {
                // FIXME: Is it possible for two pins to be connected to the same block?
                //        I am wondering if this doesnt matter because it would appear as tho
                //        this block really wants to be connected lol.
                for (int jpin = ipin + 1; jpin < num_pins; jpin++) {
                    AtomBlockId first_block_id = netlist.net_pin_block(net_id, ipin);
                    AtomBlockId second_block_id = netlist.net_pin_block(net_id, jpin);
                    size_t first_node_id = p_placement.block_id_to_node_id[first_block_id];
                    size_t second_node_id = p_placement.block_id_to_node_id[second_block_id];
                    // Make sure that the first node is moveable. This makes creating the connection easier.
                    if (!p_placement.is_moveable_node(first_node_id)) {
                        if (!p_placement.is_moveable_node(second_node_id)) {
                            continue;
                        }
                        std::swap(first_node_id, second_node_id);
                    }
                    if (p_placement.is_moveable_node(second_node_id)) {
                        tripletList.emplace_back(first_node_id, first_node_id, w);
                        tripletList.emplace_back(second_node_id, second_node_id, w);
                        tripletList.emplace_back(first_node_id, second_node_id, -w);
                        tripletList.emplace_back(second_node_id, first_node_id, -w);
                    } else {
                        tripletList.emplace_back(first_node_id, first_node_id, w);
                        b_x(first_node_id) += w * p_placement.node_loc_x[second_node_id];
                        b_y(first_node_id) += w * p_placement.node_loc_y[second_node_id];
                    }
                }
            }
        }
    }

    A_sparse.setFromTriplets(tripletList.begin(), tripletList.end());
}

void QPHybridSolver::solve(PartialPlacement &p_placement) {
    const AtomNetlist& netlist = p_placement.atom_netlist;

    size_t num_star_nodes = 0;
    for (AtomNetId net_id : netlist.nets()) {
        if (p_placement.net_is_ignored_for_placement(net_id))
            continue;
        if (netlist.net_pins(net_id).size() > 3)
            num_star_nodes++;
    }

    size_t num_moveable_nodes = p_placement.num_moveable_nodes;
    Eigen::SparseMatrix<double> A_sparse(num_moveable_nodes + num_star_nodes, num_moveable_nodes + num_star_nodes);
    Eigen::VectorXd b_x = Eigen::VectorXd::Zero(num_moveable_nodes + num_star_nodes);
    Eigen::VectorXd b_y = Eigen::VectorXd::Zero(num_moveable_nodes + num_star_nodes);
    populate_hybrid_matrix(A_sparse, b_x, b_y, netlist, p_placement);

    Eigen::VectorXd x, y;
    Eigen::ConjugateGradient<Eigen::SparseMatrix<double>, Eigen::Lower|Eigen::Upper> cg;
    cg.compute(A_sparse);
    x = cg.solve(b_x);
    y = cg.solve(b_y);
    // TODO: This copy is a bit expensive. Maybe we can have eigen
    //       solve directly into the data structure we need.
    //       https://eigen.tuxfamily.org/dox/group__QuickRefPage.html#title4
    //       Map may work for what we want.
    for (size_t node_id = 0; node_id < num_moveable_nodes; node_id++) {
        p_placement.node_loc_x[node_id] = x[node_id];
        p_placement.node_loc_y[node_id] = y[node_id];
    }
}

