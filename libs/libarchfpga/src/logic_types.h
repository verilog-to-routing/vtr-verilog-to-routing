#pragma once
/*
 * Data types describing the logic (technology-mapped) models that the architecture can implement.
 * Logic models include LUT (.names), flipflop (.latch), inpad, outpad, memory slice, etc.
 * Logic models are from the internal VPR library, or can be user-defined (both defined in .blif)
 *
 * Date: February 19, 2009
 * Authors: Jason Luu and Kenneth Kent
 *
 * Updated with the LogicalModels data structure by Alex Singer
 * Date: April, 2025
 */

#include "vtr_assert.h"
#include "vtr_list.h"
#include "vtr_memory.h"
#include "vtr_range.h"
#include "vtr_strong_id.h"
#include "vtr_util.h"
#include "vtr_vector_map.h"
#include <unordered_map>
#include <vector>
#include <string>

/**
 * @brief The type of the parallel axis.
 */
enum class e_parallel_axis {
    /** X_AXIS: Data that describes an x-directed wire segment (CHANX) */
    X_AXIS,
    /** Y_AXIS: Data that describes an y-directed wire segment (CHANY) */
    Y_AXIS,
    /** BOTH_AXIS: Data that can be applied to both x-directed and y-directed wire segment */
    BOTH_AXIS,
    /** Z_AXIS: Data that describes an z-directed wire segment (CHANZ) */
    Z_AXIS
};

/*
 * Logic model data types
 * A logic model is described by its I/O ports and function name
 */
enum PORTS {
    IN_PORT,
    OUT_PORT,
    INOUT_PORT,
    ERR_PORT
};

struct t_model_ports {
    enum PORTS dir = ERR_PORT;                         /* port direction */
    char* name = nullptr;                              /* name of this port */
    int size = 0;                                      /* maximum number of pins */
    int min_size = 0;                                  /* minimum number of pins */
    bool is_clock = false;                             /* clock? */
    bool is_non_clock_global = false;                  /* not a clock but is a special, global, control signal (eg global asynchronous reset, etc) */
    std::string clock;                                 /* The clock associated with this pin (if the pin is sequential) */
    std::vector<std::string> combinational_sink_ports; /* The other ports on this model which are combinationally driven by this port */

    t_model_ports* next = nullptr; /* next port */

    int index = -1; /* indexing for array look-up */
};

/**
 * @brief Struct containing the information stored for a logical model in the
 *        LogicalModels storage class below.
 */
struct t_model {
    char* name = nullptr;                   ///< name of this logic model
    t_model_ports* inputs = nullptr;        ///< linked list of input/clock ports
    t_model_ports* outputs = nullptr;       ///< linked list of output ports
    void* instances = nullptr;              ///< TODO: Remove this. This is only used in the Parmys plugin and should be moved into there.
    int used = 0;                           ///< TODO: Remove this. This is only used in the Parmys plugin and should be moved into there.
    vtr::t_linked_vptr* pb_types = nullptr; ///< Physical block types that implement this model
    bool never_prune = false;               ///< Don't remove from the netlist even if a block of this type has no output ports used and, therefore, unconnected to the rest of the netlist
};

// A unique ID that represents a logical model in the architecture.
typedef vtr::StrongId<struct logical_model_id_tag, size_t> LogicalModelId;

/**
 * @brief A storage class containing all of the logical models in an FPGA
 *        architecture.
 *
 * This class manages creating, storing, and destroying logical models. It also
 * contains helper methods to parse the logical models.
 *
 * A logical model is the definition of a type of primitive block that can occur
 * in the atom netlist for a given FPGA architecture; it stores data that all
 * block instances of that type share.
 *
 * There are two types of logical models:
 *  1) Library Models: These are models that all architectures share. These are
 *                     created in the construtor of this class.
 *  2) User Models: These are models defined by the user and are created outside
 *                  of this class (usually by parsing an architecture file).
 */
class LogicalModels {
  public:
    // The total number of predefined blif models.
    static constexpr size_t NUM_MODELS_IN_LIBRARY = 4;

    // Built-in library model names.
    static constexpr const char* MODEL_NAMES = ".names";
    static constexpr const char* MODEL_LATCH = ".latch";
    static constexpr const char* MODEL_INPUT = ".input";
    static constexpr const char* MODEL_OUTPUT = ".output";

    // The IDs of each of the library models. These are known ahead of time,
    // and making these constexpr can save having to look them up in this class.
    static constexpr LogicalModelId MODEL_INPUT_ID = LogicalModelId(0);
    static constexpr LogicalModelId MODEL_OUTPUT_ID = LogicalModelId(1);
    static constexpr LogicalModelId MODEL_LATCH_ID = LogicalModelId(2);
    static constexpr LogicalModelId MODEL_NAMES_ID = LogicalModelId(3);

    // Iterator for the logical model IDs array.
    typedef typename vtr::vector_map<LogicalModelId, LogicalModelId>::const_iterator model_iterator;

    // A range of model IDs within the logical model IDs array.
    typedef typename vtr::Range<model_iterator> model_range;

  public:
    // Since this class maintaines pointers and these pointers are freed upon
    // destruction, this class cannot (and should not) be copied.
    LogicalModels(const LogicalModels&) = delete;
    LogicalModels& operator=(const LogicalModels&) = delete;

    /**
     * @brief The constructor of the LogicalModels class.
     *
     * This populates the library models.
     */
    LogicalModels();

    ~LogicalModels() {
        // Free the data of all models.
        clear_models();
    }

    /**
     * @brief Returns a range of logical model IDs representing all models in
     *        the architecture (both library and user models).
     */
    inline model_range all_models() const {
        return vtr::make_range(logical_model_ids_.begin(), logical_model_ids_.end());
    }

    /**
     * @brief Returns a range of logical model IDs representing all library
     *        models in the architecture.
     */
    inline model_range library_models() const {
        VTR_ASSERT_SAFE_MSG(logical_model_ids_.size() >= NUM_MODELS_IN_LIBRARY,
                            "Library models missing");
        // The library models are created in the constructor, thus they must be
        // the first L models in the IDs (where L is the number of library models).
        return vtr::make_range(logical_model_ids_.begin(),
                               logical_model_ids_.begin() + NUM_MODELS_IN_LIBRARY);
    }

    /**
     * @brief Returns a range of logical model IDs representing all user models
     *        in the architecture.
     */
    inline model_range user_models() const {
        VTR_ASSERT_SAFE_MSG(logical_model_ids_.size() >= NUM_MODELS_IN_LIBRARY,
                            "Library models missing");

        // The user models will always be located after the library models since
        // the library models were added in the constructor.
        return vtr::make_range(logical_model_ids_.begin() + NUM_MODELS_IN_LIBRARY,
                               logical_model_ids_.end());
    }

    /**
     * @brief Returns true if the given model ID represents a library model.
     */
    inline bool is_library_model(LogicalModelId model_id) const {
        VTR_ASSERT_SAFE_MSG(model_id.is_valid(),
                            "Invalid model ID");
        // The first L model IDs must be the library models. Where L is the
        // number of models in the library
        if ((size_t)model_id < NUM_MODELS_IN_LIBRARY)
            return true;

        return false;
    }

    /**
     * @brief Create a logical model with the given name.
     *
     * This method will construct a t_model object with the given name. This
     * object can be accessed and modified using the get_model method.
     *
     *  @return The ID of the newly created model.
     */
    inline LogicalModelId create_logical_model(const std::string& model_name) {
        VTR_ASSERT_MSG(model_name_to_logical_model_id_.count(model_name) == 0,
                       "A model with the given name already exists");
        // Create the new model.
        t_model new_model;
        new_model.name = vtr::strdup(model_name.c_str());

        // Create the new model's ID
        LogicalModelId new_model_id = LogicalModelId(logical_model_ids_.size());

        // Update the internal state.
        logical_models_.push_back(std::move(new_model));
        logical_model_ids_.push_back(new_model_id);
        model_name_to_logical_model_id_[model_name] = new_model_id;

        return new_model_id;
    }

    /**
     * @brief Immutable accessor to the underlying model data structure for the
     *        given model ID.
     */
    inline const t_model& get_model(LogicalModelId model_id) const {
        VTR_ASSERT_SAFE_MSG(model_id.is_valid(),
                            "Cannot get model of invalid model ID");
        return logical_models_[model_id];
    }

    /**
     * @brief Mutable accessor to the underlying model data structure for the
     *        given model ID.
     */
    inline t_model& get_model(LogicalModelId model_id) {
        VTR_ASSERT_SAFE_MSG(model_id.is_valid(),
                            "Cannot get model of invalid model ID");
        return logical_models_[model_id];
    }

    /**
     * @brief Returns the name of the given model.
     */
    inline std::string model_name(LogicalModelId model_id) const {
        VTR_ASSERT_SAFE_MSG(model_id.is_valid(),
                            "Cannot get name of invalid model ID");
        return logical_models_[model_id].name;
    }

    /**
     * @brief Returns the ID of the model with the given name. If no model has
     *        the given name, the invalid model ID will be returned.
     *
     * This method has O(1) time complexity.
     */
    inline LogicalModelId get_model_by_name(std::string model_name) const {
        auto itr = model_name_to_logical_model_id_.find(model_name);
        if (itr == model_name_to_logical_model_id_.end())
            return LogicalModelId::INVALID();
        return itr->second;
    }

    /**
     * @brief Destroys all of the models. This frees all internal model data.
     */
    void clear_models() {
        // Free the model data of all models.
        for (LogicalModelId model_id : all_models()) {
            free_model_data(logical_models_[model_id]);
        }
        // Clear all data structures.
        logical_model_ids_.clear();
        logical_models_.clear();
        model_name_to_logical_model_id_.clear();
    }

  private:
    /**
     * @brief Helper method for freeing the internal data of the given model.
     */
    void free_model_data(t_model& model);

  private:
    /// @brief A list of all logical model IDs.
    vtr::vector_map<LogicalModelId, LogicalModelId> logical_model_ids_;

    /// @brief A list of a logical models.
    vtr::vector_map<LogicalModelId, t_model> logical_models_;

    /// @brief A lookup between the name of a logical model and its ID.
    std::unordered_map<std::string, LogicalModelId> model_name_to_logical_model_id_;
};
