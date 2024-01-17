
#include "analytical_initial_placer.h"
#include "globals.h"
#include "vtr_log.h"
#include "atom_netlist.h"
// #include "place_constraints.h"
#include "vtr_time.h"
#include "vtr_ndmatrix.h"
#include "clustered_netlist_utils.h"
#include <set>
#include <utility>
#include <limits>

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <Eigen/IterativeLinearSolvers>

#define UNKNOWN_POS std::numeric_limits<float>::lowest()

namespace {

// Compressed Column Form of a matrix 
// UMFPACK_QuickStart.pdf
// https://en.wikipedia.org/wiki/Sparse_matrix
// NOTE: The matrix is symettric, so we can literally do either form, they should be identical
struct csc_matrix {
    int32_t *Ap;
    int32_t *Ai;
    double *Ax;
    csc_matrix(const vtr::NdMatrix<double, 2> &dense_matrix) {
        // The dense matrix is m by n with nz non-zero entries.
        size_t m = dense_matrix.dim_size(0);
        size_t n = dense_matrix.dim_size(1);

        // Ap is of length (n + 1)
        Ap = new int32_t[n + 1];

        // Count the number of non-zero entries
        size_t nz = 0;
        for (size_t i = 0; i < m; i++) {
            for (size_t j = 0; j < n; j++) {
                if (dense_matrix[i][j] != 0.0)
                    nz++;
            }
        }

        // Ai and Ax are both of length nz
        Ai = new int32_t[nz];
        Ax = new double[nz];

        // Populate the sparse matrix representation.
        // FIXME: Is this iteration direction bad? We may be able to do it in row major since it is symmetric.
        size_t nonzero_index = 0;
        for (size_t c = 0; c < n; c++) {
            Ap[c] = nonzero_index;
            for (size_t r = 0; r < m; r++) {
                if (dense_matrix[r][c] != 0.0) {
                    Ax[nonzero_index] = dense_matrix[r][c];
                    Ai[nonzero_index] = r;
                    nonzero_index++;
                }
            }
        }
        Ap[n] = nonzero_index;
    }

    ~csc_matrix() {
        delete[] Ap;
        delete[] Ai;
        delete[] Ax;
    }
};

// t_pl_loc stores positions as integers. Created my own node type for now.
struct t_node_pos {
    double x = UNKNOWN_POS;
    double y = UNKNOWN_POS;
};

} // namespace

static inline bool net_is_ignored_for_placement(const AtomNetlist& netlist,
                                                const AtomNetId &net_id) {
    // Nets that are not routed (like clocks) can be ignored for placement.
    if (netlist.net_is_ignored(net_id))
        return true;
    // TODO: should we need to check for fanout <= 1 nets?
    // TODO: do we need to check if the driver is invalid?
    // TODO: do we need to check if the net has sinks?
    return false;
}

static inline bool is_moveable_node(size_t node_id, size_t num_moveable_nodes) {
    return node_id < num_moveable_nodes;
}

static inline double get_HPWL(const AtomNetlist& netlist,
                              std::map<AtomBlockId, size_t> &block_id_to_node_id,
                              std::vector<t_node_pos> &node_locs) {
    double hpwl = 0.0;
    for (AtomNetId net_id : netlist.nets()) {
        // FIXME: Confirm if this should be here.
        if (net_is_ignored_for_placement(netlist, net_id))
            continue;
        double min_x = std::numeric_limits<double>::max();
        double max_x = std::numeric_limits<float>::lowest();
        double min_y = std::numeric_limits<double>::max();
        double max_y = std::numeric_limits<float>::lowest();
        for (AtomPinId pin_id : netlist.net_pins(net_id)) {
            AtomBlockId blk_id = netlist.pin_block(pin_id);
            size_t node_id = block_id_to_node_id[blk_id];
            VTR_ASSERT(node_locs[node_id].x != UNKNOWN_POS);
            VTR_ASSERT(node_locs[node_id].y != UNKNOWN_POS);
            if (node_locs[node_id].x > max_x)
                max_x = node_locs[node_id].x;
            if (node_locs[node_id].x < min_x)
                min_x = node_locs[node_id].x;
            if (node_locs[node_id].y > max_y)
                max_y = node_locs[node_id].y;
            if (node_locs[node_id].y < min_y)
                min_y = node_locs[node_id].y;
        }
        VTR_ASSERT(max_x >= min_x);
        VTR_ASSERT(max_y >= min_y);
        hpwl += max_x - min_x + max_y - min_y;
    }
    return hpwl;
}

// TODO: There are a lot of parameters. Perhaps this can be made a member method of a class.
static inline void QP_clique_formulation(const AtomNetlist& netlist, 
                                         std::map<AtomBlockId, size_t> &block_id_to_node_id,
                                         size_t num_moveable_nodes,
                                         std::vector<t_node_pos> &node_locs) {
    // Create a 2D matrix of zeros.
    // FIXME: This uses WAY too much memory since the matrix is known to be sparse.
    //        should be stored in the sparse form somehow.
    Eigen::MatrixXd A = Eigen::MatrixXd::Zero(num_moveable_nodes, num_moveable_nodes);
    // printf("Matrix initialized to size: %zu, %zu\n", T.rows(), T.cols());
    // printf("Element (1, 1) = %f\n", T(1,1));
    // vtr::NdMatrix<double, 2> A({num_moveable_nodes, num_moveable_nodes}, 0.0);
    Eigen::VectorXd b_x = Eigen::VectorXd::Zero(num_moveable_nodes);
    Eigen::VectorXd b_y = Eigen::VectorXd::Zero(num_moveable_nodes);
    // std::vector<double> b_x(num_moveable_nodes, 0.0);
    // std::vector<double> b_y(num_moveable_nodes, 0.0);

    for (AtomNetId net_id : netlist.nets()) {
        if (net_is_ignored_for_placement(netlist, net_id))
            continue;

        int num_pins = netlist.net_pins(net_id).size();
        VTR_ASSERT(num_pins > 1);
        // Using the weight from FastPlace
        double w = 1.0 / static_cast<double>(num_pins - 1);

        for (int ipin = 0; ipin < num_pins; ipin++) {
            // FIXME: Is it possible for two pins to be connected to the same block?
            //        I am wondering if this doesnt matter because it would appear as tho
            //        this block really wants to be connected lol.
            for (int jpin = ipin + 1; jpin < num_pins; jpin++) {
                AtomBlockId first_block_id = netlist.net_pin_block(net_id, ipin);
                AtomBlockId second_block_id = netlist.net_pin_block(net_id, jpin);
                size_t first_node_id = block_id_to_node_id[first_block_id];
                size_t second_node_id = block_id_to_node_id[second_block_id];
                // Make sure that the first node is moveable. This makes creating the connection easier.
                if (!is_moveable_node(first_node_id, num_moveable_nodes)) {
                    if (!is_moveable_node(second_node_id, num_moveable_nodes)) {
                        continue;
                    }
                    std::swap(first_node_id, second_node_id);
                }
                if (is_moveable_node(second_node_id, num_moveable_nodes)) {
                    A(first_node_id, first_node_id) += w;
                    A(second_node_id, second_node_id) += w;
                    A(first_node_id, second_node_id) -= w;
                    A(second_node_id, first_node_id) -= w;
                } else {
                    A(first_node_id, first_node_id) += w;
                    b_x(first_node_id) += w * node_locs[second_node_id].x;
                    b_y(first_node_id) += w * node_locs[second_node_id].y;
                }
            }
        }
    }

    // Convert the matrix A into sparse form
    // FIXME: See above. Ideally things should just be in the sparse form.
    // csc_matrix A_sparse = csc_matrix(A);
    Eigen::SparseMatrix<double> A_sparse = A.sparseView();

    // Solve using a sparse solver
    Eigen::VectorXd x, y;
    {
        vtr::ScopedStartFinishTimer timer("Clique Initial Placement");
        Eigen::ConjugateGradient<Eigen::SparseMatrix<double>, Eigen::Lower|Eigen::Upper> cg;
        cg.compute(A_sparse);
        x = cg.solve(b_x);
        y = cg.solve(b_y);
    }
    for (size_t node_id = 0; node_id < num_moveable_nodes; node_id++) {
        node_locs[node_id].x = x[node_id];
        node_locs[node_id].y = y[node_id];
    }

    // Compute the HPWL
    double hpwl = get_HPWL(netlist, block_id_to_node_id, node_locs);
    VTR_LOG("Clique HPWL = %f\n", hpwl);
}

static inline void QP_star_formulation(const AtomNetlist& netlist, 
                                       std::map<AtomBlockId, size_t> &block_id_to_node_id,
                                       size_t num_moveable_nodes,
                                       std::vector<t_node_pos> &node_locs) {
    size_t num_nets = 0;
    for (AtomNetId net_id : netlist.nets()) {
        if (net_is_ignored_for_placement(netlist, net_id))
            continue;
        num_nets++;
    }
    Eigen::MatrixXd A = Eigen::MatrixXd::Zero(num_moveable_nodes + num_nets, num_moveable_nodes + num_nets);
    Eigen::VectorXd b_x = Eigen::VectorXd::Zero(num_moveable_nodes + num_nets);
    Eigen::VectorXd b_y = Eigen::VectorXd::Zero(num_moveable_nodes + num_nets);
    size_t net_num = 0;
    for (AtomNetId net_id : netlist.nets()) {
        if (net_is_ignored_for_placement(netlist, net_id))
            continue;
        int num_pins = netlist.net_pins(net_id).size();
        VTR_ASSERT(num_pins > 1);
        // Using the weight from FastPlace
        double w = static_cast<double>(num_pins) / static_cast<double>(num_pins - 1);
        size_t star_node_id = num_moveable_nodes + net_num;
        for (AtomPinId pin_id : netlist.net_pins(net_id)) {
            AtomBlockId blk_id = netlist.pin_block(pin_id);
            size_t node_id = block_id_to_node_id[blk_id];
            // Note: the star node is always moveable
            if (is_moveable_node(node_id, num_moveable_nodes)) {
                A(star_node_id, star_node_id) += w;
                A(node_id, node_id) += w;
                A(star_node_id, node_id) -= w;
                A(node_id, star_node_id) -= w;
            } else {
                A(star_node_id, star_node_id) += w;
                b_x(star_node_id) += w * node_locs[node_id].x;
                b_y(star_node_id) += w * node_locs[node_id].y;
            }
        }
        net_num++;
    }

    Eigen::SparseMatrix<double> A_sparse = A.sparseView();

    // Solve using a sparse solver
    Eigen::VectorXd x, y;
    {
        vtr::ScopedStartFinishTimer timer("Star Initial Placement");
        Eigen::ConjugateGradient<Eigen::SparseMatrix<double>, Eigen::Lower|Eigen::Upper> cg;
        cg.compute(A_sparse);
        x = cg.solve(b_x);
        y = cg.solve(b_y);
    }
    for (size_t node_id = 0; node_id < num_moveable_nodes; node_id++) {
        node_locs[node_id].x = x[node_id];
        node_locs[node_id].y = y[node_id];
    }

    // Compute the HPWL
    double hpwl = get_HPWL(netlist, block_id_to_node_id, node_locs);
    VTR_LOG("Star HPWL = %f\n", hpwl);
}

static inline void QP_hybrid_formulation(const AtomNetlist& netlist, 
                                         std::map<AtomBlockId, size_t> &block_id_to_node_id,
                                         size_t num_moveable_nodes,
                                         std::vector<t_node_pos> &node_locs) {
    size_t num_star_nodes = 0;
    for (AtomNetId net_id : netlist.nets()) {
        if (net_is_ignored_for_placement(netlist, net_id))
            continue;
        if (netlist.net_pins(net_id).size() > 3)
            num_star_nodes++;
    }
    Eigen::MatrixXd A = Eigen::MatrixXd::Zero(num_moveable_nodes + num_star_nodes, num_moveable_nodes + num_star_nodes);
    Eigen::VectorXd b_x = Eigen::VectorXd::Zero(num_moveable_nodes + num_star_nodes);
    Eigen::VectorXd b_y = Eigen::VectorXd::Zero(num_moveable_nodes + num_star_nodes);
    size_t star_node_offset = 0;
    for (AtomNetId net_id : netlist.nets()) {
        if (net_is_ignored_for_placement(netlist, net_id))
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
                size_t node_id = block_id_to_node_id[blk_id];
                // Note: the star node is always moveable
                if (is_moveable_node(node_id, num_moveable_nodes)) {
                    A(star_node_id, star_node_id) += w;
                    A(node_id, node_id) += w;
                    A(star_node_id, node_id) -= w;
                    A(node_id, star_node_id) -= w;
                } else {
                    A(star_node_id, star_node_id) += w;
                    b_x(star_node_id) += w * node_locs[node_id].x;
                    b_y(star_node_id) += w * node_locs[node_id].y;
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
                    size_t first_node_id = block_id_to_node_id[first_block_id];
                    size_t second_node_id = block_id_to_node_id[second_block_id];
                    // Make sure that the first node is moveable. This makes creating the connection easier.
                    if (!is_moveable_node(first_node_id, num_moveable_nodes)) {
                        if (!is_moveable_node(second_node_id, num_moveable_nodes)) {
                            continue;
                        }
                        std::swap(first_node_id, second_node_id);
                    }
                    if (is_moveable_node(second_node_id, num_moveable_nodes)) {
                        A(first_node_id, first_node_id) += w;
                        A(second_node_id, second_node_id) += w;
                        A(first_node_id, second_node_id) -= w;
                        A(second_node_id, first_node_id) -= w;
                    } else {
                        A(first_node_id, first_node_id) += w;
                        b_x(first_node_id) += w * node_locs[second_node_id].x;
                        b_y(first_node_id) += w * node_locs[second_node_id].y;
                    }
                }
            }
        }
    }

    Eigen::SparseMatrix<double> A_sparse = A.sparseView();

    // Solve using a sparse solver
    Eigen::VectorXd x, y;
    {
        vtr::ScopedStartFinishTimer timer("Hybrid Initial Placement");
        Eigen::ConjugateGradient<Eigen::SparseMatrix<double>, Eigen::Lower|Eigen::Upper> cg;
        cg.compute(A_sparse);
        x = cg.solve(b_x);
        y = cg.solve(b_y);
    }
    for (size_t node_id = 0; node_id < num_moveable_nodes; node_id++) {
        node_locs[node_id].x = x[node_id];
        node_locs[node_id].y = y[node_id];
    }

    // Compute the HPWL
    double hpwl = get_HPWL(netlist, block_id_to_node_id, node_locs);
    VTR_LOG("Hybrid HPWL = %f\n", hpwl);
}

namespace analytical_placement {

bool initial_place() {
    VTR_LOG("I got here!\n");

    // Mark the fixed block locations from the placement constraints.
    // mark_fixed_blocks();

    const AtomNetlist& netlist = g_vpr_ctx.atom().nlist;

    VTR_LOG("Number of blocks: %zu\n", netlist.blocks().size());
    VTR_LOG("Number of nets: %zu\n", netlist.nets().size());

    // for (AtomBlockId blk_id : netlist.blocks()) {
    //     const std::string& block_name = netlist.block_name(blk_id);
    //     printf("Block: %s\n", block_name.c_str());
    // }

    for (AtomBlockId blk_id : netlist.blocks()) {
        AtomBlockType blk_type = netlist.block_type(blk_id);
        if (blk_type != AtomBlockType::INPAD && blk_type != AtomBlockType::OUTPAD)
            continue;
        const std::string& blk_name = netlist.block_name(blk_id);
        if (blk_type == AtomBlockType::INPAD)
            printf("IN:  ");
        else
            printf("OUT: ");
        printf("Block: %s\n", blk_name.c_str());
    }

    std::set<AtomBlockId> moveable_blocks;
    std::set<AtomBlockId> fixed_blocks;
    // FIXME: This should probably iterate over all nets in the netlist instead of blocks.
    //        we may want to remove certain nets (such as clocks).
    //        - within the netlist class there is a net_is_ignored method that may play a part in this.
    //        - see line 607 of the previous analytical placer; there may be other reasons to ignore nets.
    // for (AtomBlockId blk_id : netlist.blocks()) {
    //     AtomBlockType blk_type = netlist.block_type(blk_id);
    //     if (blk_type == AtomBlockType::INPAD || blk_type == AtomBlockType::OUTPAD)
    //         fixed_blocks.insert(blk_id);
    //     else
    //         moveable_blocks.insert(blk_id);
    // }
    for (AtomNetId net_id : netlist.nets()) {
        if (net_is_ignored_for_placement(netlist, net_id))
            continue;
        for (AtomPinId pin_id : netlist.net_pins(net_id)) {
            AtomBlockId blk_id = netlist.pin_block(pin_id);
            AtomBlockType blk_type = netlist.block_type(blk_id);
            if (blk_type == AtomBlockType::INPAD || blk_type == AtomBlockType::OUTPAD)
                fixed_blocks.insert(blk_id);
            else
                moveable_blocks.insert(blk_id);
        }
    }

    VTR_LOG("Number of fixed blocks: %zu\n", fixed_blocks.size());
    VTR_LOG("Number of moveable blocks: %zu\n", moveable_blocks.size());

    // The node_id is a number from 0 to the number of nodes that identify the blocks in
    // the netlist with the moveable blocks coming before the fixed blocks.
    size_t num_nodes = moveable_blocks.size() + fixed_blocks.size();
    std::vector<AtomBlockId> node_id_to_block_id(num_nodes);
    std::map<AtomBlockId, size_t> block_id_to_node_id;
    size_t curr_node_id = 0;
    for (AtomBlockId moveable_blk_id : moveable_blocks) {
        node_id_to_block_id[curr_node_id] = moveable_blk_id;
        block_id_to_node_id[moveable_blk_id] = curr_node_id;
        curr_node_id++;
    }
    for (AtomBlockId fixed_blk_id : fixed_blocks) {
        node_id_to_block_id[curr_node_id] = fixed_blk_id;
        block_id_to_node_id[fixed_blk_id] = curr_node_id;
        curr_node_id++;
    }

    // Stored the fixed node locations.
    // TODO: There has got to be a better way to do this.
    std::vector<t_node_pos> node_locs(num_nodes);
    // Here we are assuming that the SA placer already ran so we can get the positions of the IO.
    const PlacementContext& SA_placement_ctx = g_vpr_ctx.placement();
    const ClusteredNetlist& clustered_netlist = g_vpr_ctx.clustering().clb_nlist;
    ClusterAtomsLookup clusterBlockToAtomBlockLookup;
    clusterBlockToAtomBlockLookup.init_lookup();
    for (ClusterBlockId cluster_blk_id : clustered_netlist.blocks()) {
        for (AtomBlockId atom_blk_id : clusterBlockToAtomBlockLookup.atoms_in_cluster(cluster_blk_id)) {
            // Only transfer the fixed block locations
            if (fixed_blocks.find(atom_blk_id) != fixed_blocks.end()) {
                size_t node_id = block_id_to_node_id[atom_blk_id];
                node_locs[node_id].x = SA_placement_ctx.block_locs[cluster_blk_id].loc.x;
                node_locs[node_id].y = SA_placement_ctx.block_locs[cluster_blk_id].loc.y;
            }
        }
    }

    for (size_t i = moveable_blocks.size(); i < num_nodes; i++) {
        // FIXME: What is the proper way to check that all fixed blocks have a location?
        VTR_ASSERT(node_locs[i].x != UNKNOWN_POS);
        // printf("(%d, %d)\n", node_locs[i].x, node_locs[i].y);
    }

    QP_clique_formulation(netlist, block_id_to_node_id, moveable_blocks.size(), node_locs);
    QP_star_formulation(netlist, block_id_to_node_id, moveable_blocks.size(), node_locs);
    QP_hybrid_formulation(netlist, block_id_to_node_id, moveable_blocks.size(), node_locs);

    return true;
}

} // analytical_placement