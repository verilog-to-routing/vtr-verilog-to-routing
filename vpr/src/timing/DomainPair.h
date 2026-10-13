#pragma once

#include <algorithm>
#include <tuple>
#include <vector>

#include "tatum/TimingGraphFwd.hpp"
#include "vtr_assert.h"

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
        : num_domains_(num_domains)
        , valid_(num_domains * num_domains, false)
        , values_(num_domains * num_domains) {}

    /// @brief Returns true if a value has been set for the domain pair.
    bool contains(DomainPair pair) const { return valid_[index_(pair)]; }

    /// @brief Returns true if no value has been set for any domain pair.
    bool empty() const { return num_valid_ == 0; }

    /// @brief Returns the value set for the domain pair.
    const T& operator[](DomainPair pair) const {
        VTR_ASSERT_SAFE(contains(pair));
        return values_[index_(pair)];
    }

    /// @brief Sets the value for the domain pair.
    void set(DomainPair pair, const T& value) {
        size_t idx = index_(pair);
        if (!valid_[idx]) {
            valid_[idx] = true;
            ++num_valid_;
        }
        values_[idx] = value;
    }

    /// @brief Marks every entry invalid.
    void clear() {
        std::ranges::fill(valid_, false);
        num_valid_ = 0;
    }

    /// @brief Two tables are equal when the same domain pairs are set and their values are equal.
    /// Declared as a friend so it can read the private members of both operands.
    friend bool operator==(const DomainPairTable& lhs, const DomainPairTable& rhs) {
        if (lhs.num_domains_ != rhs.num_domains_ || lhs.valid_ != rhs.valid_) {
            return false;
        }

        for (size_t idx = 0; idx < lhs.valid_.size(); ++idx) {
            if (lhs.valid_[idx] && !(lhs.values_[idx] == rhs.values_[idx])) {
                return false;
            }
        }

        return true;
    }

  private:
    size_t index_(DomainPair pair) const {
        VTR_ASSERT_SAFE(size_t(pair.launch) < num_domains_ && size_t(pair.capture) < num_domains_);
        return size_t(pair.launch) * num_domains_ + size_t(pair.capture);
    }

    /// Number of clock domains covered by the table
    size_t num_domains_ = 0;

    /// Number of domain pairs with a value set
    size_t num_valid_ = 0;

    /// Whether each domain pair has a value set
    std::vector<char> valid_;

    /// The value of each domain pair
    std::vector<T> values_;
};
