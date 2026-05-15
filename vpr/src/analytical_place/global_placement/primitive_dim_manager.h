#pragma once
/**
 * @file
 * @author  Alex Singer
 * @date    June 2025
 * @brief   Declaration of the primitive dim manager class. This class manages
 *          the mapping between logical models in the architecture and the dimension
 *          in the primitive vector they correspond to.
 *
 * This class is populated by another class, the Mass Calculator, which decides
 * the meaning of each dimension. The Mass Calculator will pick dimensions that
 * make the most sense for the mass abstraction it hopes to create, and uses this
 * class to keep track of the dimensions which have been created. This class is
 * also passed throughout the partial legalizer in order to query about the
 * dimensions in the primitive vector.
 */

#include <string>
#include "logic_types.h"
#include "primitive_vector_fwd.h"
#include "vtr_assert.h"
#include "vtr_range.h"
#include "vtr_vector_map.h"

/**
 * @brief A manager class for keeping track of the mapping between logical models
 *        and the dimension in the primitive vector data structure they represent.
 *
 * This class allows the order of models to be changed within the primitive vector
 * data structure and allows multiple models to map to the same dimension. This
 * can help improve the efficiency and quality of partial legalization.
 */
class PrimitiveDimManager {
  public:
    // Iterator for the primitive vector dims.
    typedef typename vtr::vector_map<PrimitiveVectorDim, PrimitiveVectorDim>::const_iterator dim_iterator;

    // Range for the primitive vector dims.
    typedef typename vtr::Range<dim_iterator> dim_range;

  public:
    /**
     * @brief Returns a list of all valid primitive vector dimensions.
     */
    inline dim_range dims() const {
        return vtr::make_range(dims_.begin(), dims_.end());
    }

    /**
     * @brief Create an empty primitive vector dimension with the given name.
     */
    inline PrimitiveVectorDim create_empty_dim(const std::string& name) {
        PrimitiveVectorDim new_dim = static_cast<PrimitiveVectorDim>(dims_.size());
        dims_.push_back(new_dim);
        dim_name_.push_back(name);
        return new_dim;
    }

    /**
     * @brief Add the given model ID to the given primitive vector dim.
     *
     * It is assumed that the given model is not part of any other dimension.
     */
    inline void add_model_to_dim(LogicalModelId model_id, PrimitiveVectorDim dim) {
        VTR_ASSERT_SAFE_MSG(model_id.is_valid(),
                            "Cannot add an invalid model to a dim");
        VTR_ASSERT_SAFE_MSG(dim.is_valid(),
                            "Cannot add a model to an invalid dim");
        model_dim_.insert(model_id, dim);
    }

    /**
     * @brief Create a mapping between the given logical model and a new dimension.
     *
     * The name is used only for printing debug information on this dimension.
     */
    inline PrimitiveVectorDim create_dim(LogicalModelId model_id, const std::string& name) {
        PrimitiveVectorDim new_dim = create_empty_dim(name);
        add_model_to_dim(model_id, new_dim);
        return new_dim;
    }

    /**
     * @brief Get the primitive vector dim for the given model.
     */
    inline PrimitiveVectorDim get_model_dim(LogicalModelId model_id) const {
        VTR_ASSERT_SAFE_MSG(model_id.is_valid(),
                            "Cannot create a dim for an invalid model");
        if (model_dim_.count(model_id) == 0)
            return PrimitiveVectorDim();

        return model_dim_[model_id];
    }

    /**
     * @brief Get the name of the given primitive vector dim.
     */
    inline const std::string& get_dim_name(PrimitiveVectorDim dim) const {
        VTR_ASSERT_SAFE_MSG(dim.is_valid(),
                            "Cannot get the name of an invalid dim");
        return dim_name_[dim];
    }

  private:
    /// @brief All of the valid primitive vector dims.
    vtr::vector_map<PrimitiveVectorDim, PrimitiveVectorDim> dims_;

    /// @brief A lookup between logical models and their primitive dim.
    vtr::vector_map<LogicalModelId, PrimitiveVectorDim> model_dim_;

    /// @brief A lookup between primitive dims and their names.
    vtr::vector_map<PrimitiveVectorDim, std::string> dim_name_;
};
