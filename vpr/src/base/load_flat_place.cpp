#include <cstring>
#include <string.h>
#include <fstream>
#include "cluster_placement.h"
#include "cluster_router.h"
#include "cluster_util.h"
#include "clustered_netlist_utils.h"
#include "globals.h"
#include "load_flat_place.h"
#include "pack.h"
#include "place_util.h"
#include "prepack.h"
#include "re_cluster_util.h"
#include "read_place.h"
#include "vtr_digest.h"
#include "vtr_time.h"


/* indicates whether to force a site index or let VPR choose */
enum e_force_site {
    VPR_SITE = 0,
    USER_SITE = 1,
};

/* indicates which orphan clustering method to use */
enum e_orphan_cluster_type {
    SINGLES = 0,
    ANNEX = 1,
    DENSE = 2,
};

/* This struct is populated for each flat placement file entry, but
 * is only saved if the molecule is an orphan; else it is overwritten
 */
typedef struct molecule_placement {

    bool valid;

    /* information from the flat placement file entry */
    int lineno;
    std::string name;
    t_pl_loc loc;
    int site;

    /* information from the corresponding vpr prepacked molecule */
    t_pack_molecule* molecule;
    AtomBlockId root;
    std::string root_type_name;
    int actual_size;
    int array_size;

    /* This field contains one of the following:
     * - id of the existing cluster this molecule should join
     * - ClusterBlockId::INVALID() if the target cluster is not yet started
     */
    ClusterBlockId target_cluster;

    /* compatible cluster types at the target cluster location */
    std::vector<t_logical_block_type_ptr> candidate_types_at_loc;

} t_molecule_placement;

/* file scope variables */

/* mapping between primitive type and cluster type */
std::map<const t_model*, std::vector<t_logical_block_type_ptr>> primitive_candidate_block_types;

/* interim orphans, stored in a set for each cluster */
std::map<const ClusterBlockId, std::set<t_molecule_placement*>> cluster_orphans;

/* orphans due to bad cluster placement coordinates (no target cluster) */
std::set<t_molecule_placement*> permanent_orphans;

/* legalization stats */
std::map<int, int> initial_cluster_failures_histogram;
std::map<int, int> clusters_not_fully_repaired_histogram;

/* orphan cluster info */
int num_orphan_clusters;
int num_orphans_reclustered;
int max_orphan_cluster_size;
int cur_orphan_cluster_size;

int verbosity;

/* file scope functions */

/* @brief init relevant vpr clustering and placement data structures */
static void legalizer_init(const char* flat_place_file);

/* @brief finds and processes the next valid flat placement file entry */
static int get_next_molecule_placement(std::ifstream& placement_file,
                                 int* lineno, t_molecule_placement** curptr);

/* @brief marks a molecule placement valid if it is the root of
 * an as yet unplaced molecule.
 */
static void process_and_check_flat_placement_entry(t_molecule_placement* cur);

/* @brief finds atom's pack molecule, determines whether or not atom is root;
 *  if root, populates relevant info in molecule placement and marks valid */
static bool find_atom_molecule_root(AtomBlockId atom,
                                    t_molecule_placement* cur);

/* @brief records max molecule size and counts actual atoms in molecule. */
static void get_molecule_size(t_molecule_placement* cur);

/*  @brief if there is an existing cluster at the target coordinates, sets it as
 *         the molecule's target cluster; else, sets target to INVALID.   */
static void set_molecule_target_cluster(t_molecule_placement* cur);

/* @brief checks whether or not the molecule's target cluster coordinates are in bounds */
static bool check_cluster_loc(t_molecule_placement* cur);

/* @brief finds any cluster types compatible with the molecule and its target loc */
static bool find_compatible_cluster_types_at_loc(t_molecule_placement* cur);

/* @brief adds a previously unclustered molecule to a new or existing cluster. if success,
 *        returns the block id of the target cluster, else returns INVALID_BLOCK_ID    */
static ClusterBlockId add_new_mol_to_cluster(t_molecule_placement* cur,
                                             t_clustering_data& clustering_data,
                                             enum e_force_site force_site,
                                             enum e_detailed_routing_stages detailed_routing_stage,
                                             bool enable_pin_feasibility_filter,
                                             bool is_orphan_cluster = false);

/* @brief adds a new cluster to place_ctx and adds placement
 * coordinates (if this is not an orphan cluster)
 */
static void place_new_cluster(t_molecule_placement* cur,
                              ClusterBlockId iblk,
                              bool is_orphan);

/* @brief attempts intra-cluster routing on a given intra-cluster placement;
 *        if successful, clustering_data.intra_lb_routing[iblk] contains the
 *        successful routing solution.
 */
static bool try_route_cluster(ClusterBlockId iblk,
                              t_clustering_data& clustering_data,
                              int verb);

/* @brief removes all molecules from cluster, rebuilds
 *        using detailed incremental legality checks
 */
static void dissolve_and_rebuild_cluster(ClusterBlockId iblk,
                                         t_clustering_data& clustering_data,
                                         enum e_detailed_routing_stages detailed_routing_stage,
                                         bool enable_pin_feasibility_filter);

/* @brief returns primitive site index of atom placed in a cluster */
static int get_placed_atom_site(AtomBlockId atom);

/* @brief prints cluster repair stats */
static void print_legalizer_stats(void);

/* @brief Prints flat placement file entries for the atoms in one placed cluster. */
static void print_flat_cluster(FILE* fp, ClusterBlockId iblk,
                               std::vector<AtomBlockId>& atoms);

/* function implementations */

static void legalizer_init(const char* flat_place_file) {
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();
    auto& atom_ctx = g_vpr_ctx.mutable_atom();
    auto& helper_ctx = g_vpr_ctx.mutable_cl_helper();
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    auto& block_locs = place_ctx.mutable_block_locs();
    auto& device_ctx = g_vpr_ctx.device();

    GridBlock& grid_blocks = place_ctx.mutable_grid_blocks();

    /* init relevant vpr clustering and placement data structures */
    cluster_ctx.clb_nlist = ClusteredNetlist(flat_place_file,
                                       vtr::secure_digest_file(flat_place_file));
    atom_ctx.prepacker.init(atom_ctx.nlist, device_ctx.logical_block_types); //kate

    helper_ctx.cluster_placement_stats = alloc_and_load_cluster_placement_stats();
    primitive_candidate_block_types = identify_primitive_candidate_block_types();
    int max_pb_depth = 0;
    int max_molecule_size = 50;
    get_max_cluster_size_and_pb_depth(helper_ctx.max_cluster_size, max_pb_depth);
    helper_ctx.primitives_list = new t_pb_graph_node*[max_molecule_size];
    helper_ctx.primitives_list[0] = nullptr;
    init_placement_context(block_locs, grid_blocks);
    helper_ctx.total_clb_num = 0;
}

static int get_next_molecule_placement(std::ifstream& placement_file,
                                       int* lineno, t_molecule_placement** curptr) {

    /* if the last molecule placement was saved as an orphan, re-allocate */
    if (*curptr == nullptr) {
        *curptr = (t_molecule_placement*) vtr::calloc(1, sizeof(t_molecule_placement));
    }
    t_molecule_placement* cur = *curptr;

    /* parse until a valid line can be returned */
    std::string line;
    while (std::getline(placement_file, line)) {
        ++(*lineno);
        std::vector<std::string> tokens = vtr::split(line, " \t\n");
        if (tokens.empty()) {
            continue; /* skip empty lines */

        } else if (tokens[0][0] == '#') {
            continue; /* skip commented lines */

        } else if (tokens.size() >= 5) {

            /* format: name X Y subtile site (#optional comment) */
            cur->name = tokens[0];
            cur->loc = t_pl_loc(vtr::atoi(tokens[1]), vtr::atoi(tokens[2]),
                                          vtr::atoi(tokens[3]), 0);
            cur->site = vtr::atoi(tokens[4]);
            cur->lineno = *lineno;
            cur->valid = false;

            /* sets cur->valid if this atom is root
             * of a valid, as yet unplaced molecule */
            process_and_check_flat_placement_entry(cur);

            /* if current entry is not valid (e.g. already
             * placed and/or not root), go to next line    */
            if (!cur->valid) {
                continue;
            }

            if (cur->root_type_name == "LUT6") {
                cur->site = cur->site - 1;
            }

            /* return if cur holds a valid molecule placement */
            return 0;

        } else {
            VTR_LOG("Legalizer: line %d - too few arguments!\n", *lineno);
            continue;
        }
    }
    free(*curptr);
    *curptr = nullptr;
    return 1;
}

static void process_and_check_flat_placement_entry(t_molecule_placement* cur) {

    auto& atom_ctx = g_vpr_ctx.mutable_atom();

    /* find (by name) and check atom */
    auto atom_blk_id = atom_ctx.nlist.find_block(cur->name);
    if (atom_blk_id == AtomBlockId::INVALID()) {
        VTR_LOG("Legalizer: line %d: skipping unrecognized primitive: %s!\n",
                 cur->lineno, cur->name);
        return;
    }
    /* skip if already placed (as part of a complex molecule) */
    if (atom_ctx.lookup.atom_clb(atom_blk_id) != ClusterBlockId::INVALID()) {
        return;
    }
    /* find molecule, check if atom is root */
    if(!find_atom_molecule_root(atom_blk_id, cur)) {
        return;
    }
    /* this is a valid molecule placement */
    cur->valid = true;
}

static bool find_atom_molecule_root(AtomBlockId atom, t_molecule_placement* cur) {

        auto& atom_ctx = g_vpr_ctx.atom();
        t_pack_molecule* mol = atom_ctx.prepacker.get_atom_molecule(atom);
        AtomBlockId root = mol->atom_block_ids[mol->root];

        if (root != atom) {
            std::string type_name = atom_ctx.nlist.block_model(atom)->name;
            if (type_name != "IBUF") {
                return false;
            }
        }

        cur->molecule = mol;
        cur->root = root;
        get_molecule_size(cur);
        cur->root_type_name = atom_ctx.nlist.block_model(root)->name;
        return true;
}

static void get_molecule_size(t_molecule_placement* cur) {
    int actual_size = 0;
    int max_size = get_array_size_of_molecule(cur->molecule);
    for (int idx = 0; idx < max_size; idx++) {
        if (cur->molecule->atom_block_ids[idx]) {
            actual_size++;
        }
    }
    cur->actual_size = actual_size;
    cur->array_size = max_size;
}

static void set_molecule_target_cluster(t_molecule_placement* cur) {
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    if (place_ctx.grid_blocks().is_sub_tile_empty({cur->loc.x, cur->loc.y, 0},
                                                 cur->loc.sub_tile)) {
        cur->target_cluster = ClusterBlockId::INVALID();
    } else {
        cur->target_cluster = place_ctx.grid_blocks().block_at_location(cur->loc);
    }
}

static bool check_cluster_loc(t_molecule_placement* cur) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;
    t_pl_loc loc = cur->loc;
    t_physical_tile_type_ptr physical_type = grid.get_physical_type({loc.x, loc.y, loc.layer});

    if( loc.x < 0 || loc.x > (int) grid.width()-1  ||
        loc.y < 0 || loc.y > (int) grid.height()-1 ||
        loc.sub_tile < 0 || loc.sub_tile > physical_type->capacity-1 ) {
        VTR_LOG("Legalizer: line %d: placement coords out of device range.\n",
                 cur->lineno);
        return false;
    }
    return true;
}

static bool find_compatible_cluster_types_at_loc(t_molecule_placement* cur) {
    auto& atom_ctx = g_vpr_ctx.mutable_atom();
    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;

    /* get all cluster types compatible with this molecule */
    auto itr = primitive_candidate_block_types.find(atom_ctx.nlist.block_model(cur->root));
    if (itr == primitive_candidate_block_types.end()) {
        VTR_LOG("Legalizer: line %d no candidate block types for this molecule!\n", cur->lineno);
        return false;
    }
    std::vector<t_logical_block_type_ptr> mol_cluster_types = itr->second;

    /* check against each cluster type compatible with the target subtile */
    t_physical_tile_type_ptr physical_type = grid.get_physical_type({cur->loc.x, cur->loc.y, cur->loc.layer});
    const t_sub_tile* sub_tile = nullptr;
    for (auto& st : physical_type->sub_tiles) {
        if (st.capacity.is_in_range(cur->loc.sub_tile)) {
            sub_tile = &st;
            break;
        }
    }
    for (t_logical_block_type_ptr subtile_cluster_type : sub_tile->equivalent_sites) {
        auto result = std::find(mol_cluster_types.begin(),
                                mol_cluster_types.end(), subtile_cluster_type);
        if (result != mol_cluster_types.end()) {
            cur->candidate_types_at_loc.push_back(subtile_cluster_type);
        }
    }
    return !(cur->candidate_types_at_loc.empty());
}

static void place_new_cluster(t_molecule_placement* cur,
                              ClusterBlockId iblk, bool is_orphan) {

    /* add cluster to place_ctx */
    auto& block_locs = g_vpr_ctx.mutable_placement().mutable_block_locs();
    block_locs.resize(g_vpr_ctx.placement().block_locs().size() + 1);

    if (is_orphan) {
        block_locs[iblk].is_fixed = false;
    } else {
        /* add fixed loc if not an orphan */
        g_vpr_ctx.mutable_placement().mutable_blk_loc_registry().set_block_location(iblk, cur->loc);
        block_locs[iblk].is_fixed = true;
    }
}

static ClusterBlockId add_new_mol_to_cluster(t_molecule_placement* cur,
                                             t_clustering_data& clustering_data,
                                             enum e_force_site force_site,
                                             enum e_detailed_routing_stages detailed_routing_stage,
                                             bool enable_pin_feasibility_filter,
                                             bool is_orphan_cluster) {
    auto& atom_ctx = g_vpr_ctx.mutable_atom();
    auto& helper_ctx = g_vpr_ctx.mutable_cl_helper();
    bool is_new_cluster = (cur->target_cluster == ClusterBlockId::INVALID());
    int site = (force_site == USER_SITE) ? cur->site : -1;
    t_lb_router_data* router_data = nullptr;
    bool success = false;

    if (!is_new_cluster) {
        /* add to existing cluster */
        std::unordered_set<AtomBlockId>& atoms = cluster_to_mutable_atoms(cur->target_cluster);
        success = pack_mol_in_existing_cluster(cur->molecule,
                                               cur->array_size,
                                               cur->target_cluster,
                                               atoms,
                                               true,
                                               clustering_data,
                                               router_data,
                                               detailed_routing_stage,
                                               enable_pin_feasibility_filter,
                                               site);
        free_router_data(router_data);
        router_data = nullptr;
        if (success) {
            return cur->target_cluster;
        }
    } else {
        /* create new cluster */
        PartitionRegion tmp_pr;
        NocGroupId tmp_noc = NocGroupId::INVALID();
        std::vector<t_logical_block_type_ptr>* candidate_types;

        /* for orphans, candidate block types are not limited to a specific subtile */
        if (is_orphan_cluster) {
            auto itr = primitive_candidate_block_types.find(atom_ctx.nlist.block_model(cur->root));
            if (itr == primitive_candidate_block_types.end()) {
                VTR_LOG("Legalizer: no candidate block types!\n");
                return ClusterBlockId::INVALID();
            }
            candidate_types = &itr->second;
        } else {
            candidate_types = &cur->candidate_types_at_loc;
        }
        ClusterBlockId new_cluster(helper_ctx.total_clb_num);
        for (auto type : *candidate_types) {
            for (int mode = 0; mode < type->pb_type->num_modes; mode++) {
               success = start_new_cluster_for_mol(cur->molecule,
                                                    type,
                                                    mode,
                                                    helper_ctx.feasible_block_array_size,
                                                    enable_pin_feasibility_filter,
                                                    new_cluster,
                                                    true,
                                                    verbosity,
                                                    clustering_data,
                                                    &router_data,
                                                    tmp_pr,
                                                    tmp_noc,
                                                    detailed_routing_stage,
                                                    site);
                if (success) {
                    place_new_cluster(cur, new_cluster, is_orphan_cluster);
                    return new_cluster;
                }
            }
        }
    }
    return ClusterBlockId::INVALID();
}

bool try_route_cluster(ClusterBlockId iblk,
                       t_clustering_data& clustering_data, int verb) {

    auto& helper_ctx = g_vpr_ctx.mutable_cl_helper();
    t_mode_selection_status mode_status;
    std::unordered_set<AtomBlockId>& atoms = cluster_to_mutable_atoms(iblk);
    t_lb_router_data* router_data = lb_load_router_data(helper_ctx.lb_type_rr_graphs, iblk, atoms);
    bool is_legal = false;

    do {
        reset_intra_lb_route(router_data);
        is_legal = try_intra_lb_route(router_data, verb, &mode_status);

    } while (mode_status.is_mode_issue());

    if (is_legal) {
        free_intra_lb_nets(clustering_data.intra_lb_routing[iblk]);
        clustering_data.intra_lb_routing[iblk] = router_data->saved_lb_nets;
        router_data->saved_lb_nets = nullptr;
    }
    free_router_data(router_data);
    return is_legal;
}

static int get_placed_atom_site(AtomBlockId atom) {
    auto& atom_ctx = g_vpr_ctx.atom();
    return atom_ctx.lookup.atom_pb(atom)->pb_graph_node->flat_site_index;
}

static void dissolve_and_rebuild_cluster(ClusterBlockId iblk,
                                  t_clustering_data& clustering_data,
                                  enum e_detailed_routing_stages detailed_routing_stage,
                                  bool enable_pin_feasibility_filter) {

    auto& atom_ctx = g_vpr_ctx.atom();
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();
    auto& block_locs = g_vpr_ctx.placement().block_locs();
    std::vector<t_molecule_placement*> saved_molecule_placements;
    std::unordered_set<AtomBlockId>& atoms = cluster_to_mutable_atoms(iblk);
    int saved_mode = cluster_ctx.clb_nlist.block_pb(iblk)->mode;

    /* record all cluster molecule placements */
    t_molecule_placement* tmp = nullptr;
    for (auto atom : atoms) {
        tmp = (t_molecule_placement*) vtr::calloc(1, sizeof(t_molecule_placement));
        tmp->valid = false;

        /* skip non-root atoms */
        if (!find_atom_molecule_root(atom, tmp)) {
            continue;
        }
        tmp->valid =true;
        tmp->lineno = 0;
        tmp->name = atom_ctx.nlist.block_name(tmp->root);
        tmp->loc.x = block_locs[iblk].loc.x;
        tmp->loc.y = block_locs[iblk].loc.y;
        tmp->loc.sub_tile = block_locs[iblk].loc.sub_tile;
        tmp->loc.layer = block_locs[iblk].loc.layer;
        tmp->site = get_placed_atom_site(atom);
        tmp->target_cluster = iblk;
        saved_molecule_placements.push_back(tmp);
    }
    if (tmp->valid == false) {
        free(tmp);
    }

    /* remove all molecules from cluster */
    auto& helper_ctx = g_vpr_ctx.mutable_cl_helper();
    t_lb_router_data* router_data = lb_load_router_data(helper_ctx.lb_type_rr_graphs, iblk, atoms);
    for (auto cur : saved_molecule_placements) {
        atoms = cluster_to_mutable_atoms(iblk);
        remove_mol_from_cluster(cur->molecule, cur->array_size,
                                iblk, atoms, false, router_data);
        commit_mol_removal(cur->molecule, cur->array_size, iblk,
                           true, router_data, clustering_data);
    }

    /* try to re-add each molecule */
    cluster_ctx.clb_nlist.block_pb(iblk)->mode = saved_mode;
    for (auto cur : saved_molecule_placements) {
        ClusterBlockId res = add_new_mol_to_cluster(cur, clustering_data,
                                                    USER_SITE, detailed_routing_stage,
                                                    enable_pin_feasibility_filter);
        if (ClusterBlockId::INVALID() == res) {
            VTR_LOG("Legalizer: re-add failure: %s ",
                     cur->root_type_name.c_str());
            VTR_LOG("%s %d %d %d %d\n", cur->name.c_str(), cur->loc.x,
                     cur->loc.y, cur->loc.sub_tile, cur->site);
            cluster_orphans[iblk].insert(cur);
            cur = nullptr;
        } else {
            free(cur);
        }
    }
}

static void print_legalizer_stats(void) {
    // initial stats
    int total = 0;
    int three_plus = 0;
    for (auto x : initial_cluster_failures_histogram) {
        total += (x.second * x.first);
        if (x.first < 3)
            VTR_LOG("Legalizer: INITIAL ERRORS: %d clusters with %d initial errors\n",
                     x.second, x.first);
        else
            three_plus += x.second;
    }
    if (three_plus > 0) {
        VTR_LOG("Legalizer: INITIAL ERRORS: %d clusters with 3+ initial errors\n",
             three_plus);
    }
    VTR_LOG("Legalizer: INITIAL ERRORS: %d total initial errors\n", total);

    // final stats
    three_plus = 0;
    for (auto x : clusters_not_fully_repaired_histogram) {
        if (x.first < 3)
          VTR_LOG("Legalizer: NOT FULLY REPAIRED: %d clusters with %d initial errors not fully repaired\n",
                   x.second, x.first);
        else
            three_plus += x.second;
    }
    if (three_plus > 0) {
        VTR_LOG("Legalizer: NOT FULLY REPAIRED: %d clusters with 3+ initial errors not fully repaired\n",
             three_plus);
    }
    VTR_LOG("Legalizer: ORPHANS: %d total orphans reclustered\n", num_orphans_reclustered);
    VTR_LOG("Legalizer: ORPHANS: added %d orphan clusters\n", num_orphan_clusters);
}

static void print_flat_cluster(FILE* fp, ClusterBlockId iblk,
                               std::vector<AtomBlockId>& atoms) {

    auto& atom_ctx = g_vpr_ctx.atom();
    auto& block_locs = g_vpr_ctx.placement().block_locs();
    t_pl_loc loc = block_locs[iblk].loc;

    for (auto atom : atoms) {
        t_pb_graph_node* atom_pbgn = atom_ctx.lookup.atom_pb(atom)->pb_graph_node;
	int site = atom_pbgn->flat_site_index;
	std::string type_name = atom_ctx.nlist.block_model(atom)->name;
        if (type_name == "LUT6") {
            site = site + 1;
        }
        fprintf(fp, "%s  %d %d %d %d #%zu %s\n", atom_ctx.nlist.block_name(atom).c_str(),
                                                loc.x, loc.y, loc.sub_tile,
                                                site, //atom_pbgn->flat_site_index,
                                                size_t(iblk),
                                                atom_pbgn->pb_type->name);
    }
}

/* prints a flat placement file */
void print_flat_placement(const char* flat_place_file) {

    FILE* fp;
    ClusterAtomsLookup atoms_lookup;
    auto& cluster_ctx = g_vpr_ctx.clustering();

    if (!g_vpr_ctx.placement().block_locs().empty()) {
        fp = fopen(flat_place_file, "w");
        for (auto iblk : cluster_ctx.clb_nlist.blocks()) {
            auto atoms = atoms_lookup.atoms_in_cluster(iblk);
            print_flat_cluster(fp, iblk, atoms);
        }
        fclose(fp);
    }
}

static void print_pl_cluster(FILE* fp, ClusterBlockId iblk,
                               std::vector<AtomBlockId>& atoms) {

    auto& atom_ctx = g_vpr_ctx.atom();
    auto& block_locs = g_vpr_ctx.placement().block_locs();
    t_pl_loc loc = block_locs[iblk].loc;

    for (auto atom : atoms) {
        t_pb_graph_node* atom_pbgn = atom_ctx.lookup.atom_pb(atom)->pb_graph_node;
	int site = atom_pbgn->flat_site_index;
	std::string type_name = atom_ctx.nlist.block_model(atom)->name;
	if ((type_name == ".input") || (type_name == ".output")) {
            continue;
        }
        else if ((type_name == "IBUF") || (type_name == "OBUF") || (type_name == "BUFGCE")) {
            continue;
        }
        fprintf(fp, "%s  %d %d %d #%s\n", atom_ctx.nlist.block_name(atom).c_str(),
                                                loc.x, loc.y, site, type_name.c_str());
    }
}

/* prints a flat placement file for ISPD16 Vivado placement and routing */
void print_pl_file(const char* pl_file) {

    FILE* fp;
    ClusterAtomsLookup atoms_lookup;
    auto& cluster_ctx = g_vpr_ctx.clustering();

    if (!g_vpr_ctx.placement().block_locs().empty()) {
        fp = fopen(pl_file, "w");
        for (auto iblk : cluster_ctx.clb_nlist.blocks()) {
            auto atoms = atoms_lookup.atoms_in_cluster(iblk);
            print_pl_cluster(fp, iblk, atoms);
        }
        fclose(fp);
    }
}

/* ingests and legalizes a flat placement file, returns num new clusters created  */
int load_flat_placement(t_vpr_setup& vpr_setup, const t_arch& arch) {

    auto flat_place_file = vpr_setup.FileNameOpts.FlatPlaceFile;
    VTR_LOG("\nLoading flat placement file: %s, ", flat_place_file.c_str());

    /* settings (TODO:  offer as command line options)
     * lucky: presumed legal input placement, skip initial pin counting checks
     * orphan_cluster_type: SINGLES | ANNEX | DENSE
     * max_orphan_cluster_size: max orphan cluster size (not used for SINGLES)
     * verbosity: passed to try_intra_lb_route to optionally print diagnostic info
     */
    bool lucky = false;
    enum e_orphan_cluster_type orphan_cluster_type = SINGLES; //= DENSE; //ANNEX; //SINGLES;
    max_orphan_cluster_size = 8;
    verbosity = 0;

    /* init relevant vpr clustering and placement data structures */
    legalizer_init(flat_place_file.c_str());
    t_clustering_data clustering_data;

    /* Open the flat placement file */
    VTR_LOG("Legalizer: opening file '%s'.\n", flat_place_file.c_str());
    std::ifstream infile(flat_place_file.c_str());
    if (!infile) {
        VPR_FATAL_ERROR(VPR_ERROR_PLACE_F,
                        "Legalizer: Cannot open file '%s'.\n",
                        flat_place_file.c_str());
        return -1;
    }

    /* initial pass: attempt to place each mol on original site in original cluster */
    VTR_LOG("Legalizer: starting first pass\n");
    t_molecule_placement* cur = nullptr;
    int lineno = 0;

    vtr::Timer parse_loop_timer;

    /* check each line, enter loop body only when cur holds a valid molecule */
    while (!get_next_molecule_placement(infile, &lineno, &cur)) {

        /* if molecule is valid but cluster loc is not, save for orphan reclustering */
        if (!check_cluster_loc(cur) || !find_compatible_cluster_types_at_loc(cur)) {
            VTR_LOG("Legalizer: line %d (%s): incompatible cluster placement coordinates.\n",
                     cur->lineno, cur->root_type_name.c_str());
            cur->target_cluster = ClusterBlockId::INVALID();
            permanent_orphans.insert(cur);
            cur = nullptr;
            continue;
        }

       /* add molecule to its target cluster (start new if the subtile is empty) */
        set_molecule_target_cluster(cur);
        ClusterBlockId res = add_new_mol_to_cluster(cur, clustering_data, USER_SITE,
                                                     E_DETAILED_ROUTE_AT_END_ONLY, !lucky);

       /* if add/create fails, save molecule for cluster repair, else reset */
       if (ClusterBlockId::INVALID() == res) {
           cluster_orphans[cur->target_cluster].insert(cur);
           VTR_LOG("Legalizer: failed to add %s %s to site %d in cluster %d.\n",
                         cur->root_type_name.c_str(), cur->name.c_str(), cur->site, (int) cur->target_cluster);

           cur = nullptr;
       } else {
           //VTR_LOG("Legalizer: successfully added %s %s to site %d in cluster %d.\n",
           //              cur->root_type_name.c_str(), cur->name.c_str(), cur->site, (int) res);
           cur->valid = false;
           cur->candidate_types_at_loc.clear();
       }
    }
    infile.close();

    /* process molecules for which creating a new cluster failed. If
     * the cluster has since been created, add molecule to it orphan list
     * if the cluster has not been created, re-attempt without forced site */
    VTR_LOG("Legalizer: processing stray orphans.\n");
    auto it = cluster_orphans[ClusterBlockId::INVALID()].begin();
    while (it != cluster_orphans[ClusterBlockId::INVALID()].end()) {
        auto mol = *it;
        set_molecule_target_cluster(mol);
        if (mol->target_cluster == ClusterBlockId::INVALID()) {
            // attempt to create the cluster without a forced site
            VTR_LOG("Legalizer: re-attempting new cluster creation for %s.\n",
                     mol->root_type_name.c_str());
            ClusterBlockId res = add_new_mol_to_cluster(mol, clustering_data, VPR_SITE,
                                                         E_DETAILED_ROUTE_AT_END_ONLY, !lucky);
            if (ClusterBlockId::INVALID() == res) {
                // failure, add to permanent orphans list
                VTR_LOG("Legalizer: failed to create new cluster for %s.\n",
                         mol->root_type_name.c_str());
                mol->target_cluster = ClusterBlockId::INVALID();
                permanent_orphans.insert(mol);
                it = cluster_orphans[ClusterBlockId::INVALID()].erase(it);
            } else { // success
                it = cluster_orphans[ClusterBlockId::INVALID()].erase(it);
                free(mol);
            }
        } else {
            // target cluster exists, add to mol orphan list
            cluster_orphans[mol->target_cluster].insert(mol);
        }
        it = cluster_orphans[ClusterBlockId::INVALID()].erase(it);
    }
    if (cluster_orphans[ClusterBlockId::INVALID()].size() == 0) {
        cluster_orphans.erase(ClusterBlockId::INVALID());
    } else {
        VTR_LOG("Legalizer: some empty block orphans were not handled!\n");
    }

    VTR_LOG("Legalizer: starting routing-based cluster legality checks.\n");
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();
    for (auto iblk : cluster_ctx.clb_nlist.blocks()) {
        bool success = try_route_cluster(iblk, clustering_data, verbosity);
        if (!success) {
            VTR_LOG("Legalizer: cluster #%lu failed intra-cluster routing - dissolving and rebuilding", iblk);
            dissolve_and_rebuild_cluster(iblk, clustering_data,
                                         E_DETAILED_ROUTE_FOR_EACH_ATOM, lucky);
        }
    }

    VTR_LOG("Legalizer: starting cluster repair.\n");
    num_orphans_reclustered = 0;

    // for each cluster with orphans, attempt repairs, then recluster remaining orphans
    auto itblk = cluster_orphans.begin();
    while (itblk != cluster_orphans.end()) {
        auto iblk = itblk->first;
        VTR_LOG("Legalizer: processing %d orphans for cluster %zu\n",
                   cluster_orphans[iblk].size(), size_t(iblk));

        /* repair stats */
        int initial = cluster_orphans[iblk].size();
        int final = initial;
        ++initial_cluster_failures_histogram[initial];

        /* for each orphan, attempt repair; recluster upon failure */
        auto itmol = cluster_orphans[iblk].begin();
        while (itmol != cluster_orphans[iblk].end()) {

            auto mol = *itmol;
            bool success = false;

            /* attempt to add the molecule to the target cluster on any available site */
            ClusterBlockId res = ClusterBlockId::INVALID();
            res = add_new_mol_to_cluster(mol, clustering_data, VPR_SITE,
                                         E_DETAILED_ROUTE_FOR_EACH_ATOM, lucky);

            if (res == mol->target_cluster) {
                VTR_LOG("Legalizer: successfully added %s on vpr site in original cluster %zu \norig placement: %s %d %d %d\n",
                           mol->name.c_str(), size_t(res),
                           mol->name.c_str(), mol->loc.x, mol->loc.y, mol->site);
                success = true;
                --final;
            }

            if (success) {
                free(mol);
                itmol = cluster_orphans[iblk].erase(itmol);
            } else {
                VTR_LOG("Legalizer: failed to add %s to original cluster.\n",
                                 mol->name.c_str());
                ++itmol;
                ++num_orphans_reclustered;
            }
        }
        if (final > 0) {
            ++clusters_not_fully_repaired_histogram[initial];
        }
        if (cluster_orphans[iblk].size() == 0) {
            itblk = cluster_orphans.erase(itblk);
        } else {
            VTR_LOG("Legalizer: some orphans not accommodated in cluster %zu\n", size_t(iblk));
            ++itblk;
        }
    }

    VTR_LOG("Legalizer: starting orphan reclustering.\n");
    ClusterBlockId orphan_cluster = ClusterBlockId::INVALID();
    num_orphan_clusters = 0;
    cur_orphan_cluster_size = 0;

    // for each cluster with orphans, attempt repairs, then recluster remaining orphans
    itblk = cluster_orphans.begin();
    while (itblk != cluster_orphans.end()) {
        auto iblk = itblk->first;
        VTR_LOG("Legalizer: reclustering %d orphans for cluster %zu\n",
                   cluster_orphans[iblk].size(), size_t(iblk));

        /* for annex reclustering, reset orphan cluster for each iblk */
        if (ANNEX == orphan_cluster_type) {
	    if (orphan_cluster != ClusterBlockId::INVALID()) {
                orphan_cluster = ClusterBlockId::INVALID();
                cur_orphan_cluster_size = 0;
            }
        }

        /* for each orphan, attempt repair; recluster upon failure */
        auto itmol = cluster_orphans[iblk].begin();
        while (itmol != cluster_orphans[iblk].end()) {

            auto mol = *itmol;
            bool success = false;

            // attempt to add to existing orphan cluster
            if (orphan_cluster != ClusterBlockId::INVALID()) {
                //VTR_LOG("Legalizer: attempting to add %s to orphan cluster %zu.\n",
                //        mol->name.c_str(), size_t(orphan_cluster));
                mol->target_cluster = orphan_cluster;
                ClusterBlockId res = ClusterBlockId::INVALID();
                res = add_new_mol_to_cluster(mol, clustering_data,
                       VPR_SITE, E_DETAILED_ROUTE_FOR_EACH_ATOM, true, true);
                if (res != ClusterBlockId::INVALID()) {
                    VTR_LOG("Legalizer: added %s to an orphan cluster %zu (orphan_cluster = %zu).\n",
                            mol->name.c_str(), res,
                                size_t(orphan_cluster));
                    success = true;
                 }// else {
                  //    VTR_LOG("Legalizer: failed to add %s to orphan cluster %zu.\n",
                  //            mol->name.c_str(), size_t(orphan_cluster));
                //}
            }

            // if add failed or there is no current orphan cluster, create a new one
            if (!success) {
                //VTR_LOG("Legalizer: attempting to add %s to a new orphan cluster.\n",
                //        mol->name.c_str());
                mol->target_cluster = ClusterBlockId::INVALID();
                ClusterBlockId res = ClusterBlockId::INVALID();
                res = add_new_mol_to_cluster(mol, clustering_data,
                   VPR_SITE, E_DETAILED_ROUTE_FOR_EACH_ATOM, true, true);
                if (res != ClusterBlockId::INVALID()) {
                      VTR_LOG("Legalizer: added %s to a new orphan cluster %zu (orphan_cluster = %zu).\n",
                                mol->name.c_str(), size_t(res), size_t(orphan_cluster));
                    ++num_orphan_clusters;
                    success = true;
                    if (orphan_cluster_type != SINGLES) {
                        orphan_cluster = res;
                    }
                } //else {
                  //      VTR_LOG("Legalizer: failed to add %s to a new orphan cluster.\n",
                  //               mol->name.c_str());
                //}
            }
            if (success) {
                cur_orphan_cluster_size += mol->actual_size;
                if (cur_orphan_cluster_size >= max_orphan_cluster_size) {
                    orphan_cluster = ClusterBlockId::INVALID();
                    cur_orphan_cluster_size = 0;
                }
                free(mol);
            } else {
                VTR_LOG("Legalizer: failed to add %s to an orphan cluster.\n",
                                 mol->name.c_str());
                permanent_orphans.insert(mol);
            }
            itmol = cluster_orphans[iblk].erase(itmol);
        }

        if (cluster_orphans[iblk].size() == 0) {
            itblk = cluster_orphans.erase(itblk);
        } else {
            VTR_LOG("Legalizer: some orphans from cluster %zu not reclustered\n", size_t(iblk));
            ++itblk;
        }

    }

    /* TODO: recluster bad placement orphans (not present in any current test cases) */

    /* print legalizer stats */
    print_legalizer_stats();

    /* print output files */
    VTR_LOG("Legalizer: printing output files\n");

    VTR_LOG("\nLegalizer: printing clustered netlist file: %s\n",
               vpr_setup.FileNameOpts.NetFile.c_str());
    std::unordered_set<AtomNetId> is_clock = alloc_and_load_is_clock();
    check_and_output_clustering(vpr_setup.PackerOpts, is_clock, &arch,
                                g_vpr_ctx.cl_helper().total_clb_num,
                                clustering_data.intra_lb_routing);

    VTR_LOG("\nLegalizer: printing fix clusters file: %s\n",
               vpr_setup.FileNameOpts.write_constraints_file.c_str());
    print_place(nullptr, nullptr,
                vpr_setup.FileNameOpts.write_constraints_file.c_str(),
                g_vpr_ctx.mutable_placement().mutable_blk_loc_registry().block_locs(), false);

    return num_orphan_clusters;
}
