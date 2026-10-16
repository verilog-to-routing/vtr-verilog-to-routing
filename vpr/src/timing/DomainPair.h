#pragma once

#include <tuple>

#include "tatum/TimingGraphFwd.hpp"
#include "vtr_assert.h"
#include "vtr_ndmatrix.h"

//A pair of clock domains
struct DomainPair {
    DomainPair(tatum::DomainId l, tatum::DomainId c)
        : launch(l)
        , capture(c) {}

    tatum::DomainId launch;
    tatum::DomainId capture;

    friend bool operator<(const DomainPair& lhs, const DomainPair& rhs) {
        return std::tie(lhs.launch, lhs.capture) < std::tie(rhs.launch, rhs.capture);
    }

    friend bool operator==(const DomainPair& lhs, const DomainPair& rhs) {
        return std::tie(lhs.launch, lhs.capture) == std::tie(rhs.launch, rhs.capture);
    }
};

/// @brief Values stored densely for every (launch, capture) clock domain pair.
///
/// Every entry starts invalid. Reading an entry that has not been set is an error.
template<typename T>
class DomainPairTable {
  public:
    DomainPairTable() = default;

    /// @brief Creates a table covering num_domains clock domains with every entry invalid.
    explicit DomainPairTable(size_t num_domains)
        : valid_({num_domains, num_domains}, false)
        , values_({num_domains, num_domains}) {}

    /// @brief Returns true if a value has been set for the domain pair.
    bool contains(DomainPair pair) const { return valid_[size_t(pair.launch)][size_t(pair.capture)]; }

    /// @brief Returns true if no value has been set for any domain pair.
    bool empty() const { return num_valid_ == 0; }

    /// @brief Returns the value set for the domain pair.
    const T& operator[](DomainPair pair) const {
        VTR_ASSERT_SAFE(contains(pair));
        return values_[size_t(pair.launch)][size_t(pair.capture)];
    }

    /// @brief Sets the value for the domain pair.
    void set(DomainPair pair, const T& value) {
        bool& valid = valid_[size_t(pair.launch)][size_t(pair.capture)];
        if (!valid) {
            valid = true;
            ++num_valid_;
        }
        values_[size_t(pair.launch)][size_t(pair.capture)] = value;
    }

    /// @brief Marks every entry invalid.
    void clear() {
        valid_.fill(false);
        num_valid_ = 0;
    }

    /// @brief Two tables are equal when the same domain pairs are set and their values are equal.
    /// Declared as a friend so it can read the private members of both operands.
    friend bool operator==(const DomainPairTable& lhs, const DomainPairTable& rhs) {
        if (lhs.valid_.dim_size(0) != rhs.valid_.dim_size(0)) {
            return false;
        }

        for (size_t idx = 0; idx < lhs.valid_.size(); ++idx) {
            if (lhs.valid_.get(idx) != rhs.valid_.get(idx)) {
                return false;
            }
            if (lhs.valid_.get(idx) && !(lhs.values_.get(idx) == rhs.values_.get(idx))) {
                return false;
            }
        }

        return true;
    }

  private:
    /// Number of domain pairs with a value set
    size_t num_valid_ = 0;

    /// Whether each (launch, capture) domain pair has a value set
    vtr::NdMatrix<bool, 2> valid_;

    /// The value of each (launch, capture) domain pair
    vtr::NdMatrix<T, 2> values_;
};
