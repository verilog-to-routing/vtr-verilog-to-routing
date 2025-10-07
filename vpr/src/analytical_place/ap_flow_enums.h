#pragma once
/**
 * @file
 * @author  Alex Singer
 * @date    February 2025
 * @brief   Enumerations used by the Analytical Placement Flow.
 */

/**
 * @brief The type of an Analytical Solver.
 *
 * The Analytical Placement flow may implement different Analytical Solvers as
 * part of the Global Placer. This enum can select between these different
 * Analytical Solvers.
 */
enum class e_ap_analytical_solver {
    Identity,  ///< The identity analytical solver, which does not perform any optimizations on the placement. Used as a placeholder when solving should not be used.
    QP_Hybrid, ///< Analytical Solver which uses the hybrid net model to optimize the quadratic HPWL objective.
    LP_B2B     ///< Analytical Solver which uses the B2B net model to optimize the linear HPWL objective.
};

/**
 * @brief The type of a Partial Legalizer.
 *
 * The Analytical Placement flow may implement different Partial Legalizer as
 * part of the Global Placer. This enum can select between these different
 * Partial Legalizers.
 */
enum class e_ap_partial_legalizer {
    Identity,       ///< Partial Legalizer which does not perform any legalization on the placement. Used as a placeholder when partial legalization should not be used.
    BiPartitioning, ///< Partial Legalizer which forms minimum windows around dense regions and uses bipartitioning to spread blocks over windows.
    FlowBased       ///< Partial Legalizer which flows blocks from overfilled bins to underfilled bins.
};

/**
 * @brief The type of a Full Legalizer.
 *
 * The Analytical Placement flow may implement different Full Legalizers. This
 * enum can select between these different Full Legalizers.
 */
enum class e_ap_full_legalizer {
    Naive,    ///< The Naive Full Legalizer, which clusters atoms placed in the same tile and tries to place them in that tile according to the flat placement.
    APPack,   ///< The APPack Full Legalizer, which uses the flat placement to improve the Packer and Placer.
    FlatRecon ///< The Flat Placement Reconstruction Full Legalizer, which tries to reconstruct a clustered placement (or the in-memory global placement) that is as close to the incoming flat placement as it can.
};

/**
 * @brief The type of a Detailed Placer.
 *
 * The Analytical Placement flow may implement different Detailed Placers. This
 * enum can select between these different Detailed Placers.
 */
enum class e_ap_detailed_placer {
    Identity, ///< The Identity Detailed Placer, which does not perform any optimizations on the legalized placement. Needed as a placeholder.
    Annealer  ///< The Annealer Detailed Placer, which runs the annealer found in the Place part of the VPR flow (using the same options as the Placement stage).
};
