#pragma once

#include <map>
#include <tuple>
#include <vector>

#include "tatum/TimingGraphFwd.hpp"
#include "vtr_assert.h"

// A pair of clock domains
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

/// @brief Values stored for (launch, capture) clock domain pairs.
///
/// Pairs with the same launch and capture domain are stored densely in a vector indexed by domain.
/// Pairs with different domains are stored in a std::map keyed by the pair.
/// Every entry starts invalid. Reading an entry that has not been set is an error.
template<typename T>
class DomainPairTable {
  public:
    DomainPairTable() = default;

    /// @brief Creates a table covering num_domains clock domains with every entry invalid.
    explicit DomainPairTable(size_t num_domains)
        : same_domain_valid_(num_domains, false)
        , same_domain_values_(num_domains) {}

    /// @brief Returns true if a value has been set for the domain pair.
    bool contains(DomainPair pair) const {
        if (pair.launch == pair.capture) {
            return same_domain_valid_[size_t(pair.launch)];
        }
        return cross_domain_values_.count(pair) > 0;
    }

    /// @brief Returns true if no value has been set for any domain pair.
    bool empty() const { return num_valid_ == 0; }

    /// @brief Returns the value set for the domain pair.
    const T& operator[](DomainPair pair) const {
        if (pair.launch == pair.capture) {
            VTR_ASSERT_SAFE(same_domain_valid_[size_t(pair.launch)]);
            return same_domain_values_[size_t(pair.launch)];
        }

        auto it = cross_domain_values_.find(pair);
        VTR_ASSERT_SAFE(it != cross_domain_values_.end());
        return it->second;
    }

    /// @brief Sets the value for the domain pair.
    void set(DomainPair pair, const T& value) {
        if (pair.launch == pair.capture) {
            size_t domain = size_t(pair.launch);
            if (!same_domain_valid_[domain]) {
                same_domain_valid_[domain] = true;
                ++num_valid_;
            }
            same_domain_values_[domain] = value;
        } else {
            auto result = cross_domain_values_.insert_or_assign(pair, value);
            if (result.second) {
                ++num_valid_;
            }
        }
    }

    /// @brief Marks every entry invalid.
    void clear() {
        same_domain_valid_.assign(same_domain_valid_.size(), false);
        cross_domain_values_.clear();
        num_valid_ = 0;
    }

    /// @brief Two tables are equal when the same domain pairs are set and their values are equal.
    /// Declared as a friend so it can read the private members of both operands.
    friend bool operator==(const DomainPairTable& lhs, const DomainPairTable& rhs) {
        if (lhs.same_domain_valid_ != rhs.same_domain_valid_) {
            return false;
        }

        for (size_t domain = 0; domain < lhs.same_domain_valid_.size(); ++domain) {
            if (lhs.same_domain_valid_[domain] && !(lhs.same_domain_values_[domain] == rhs.same_domain_values_[domain])) {
                return false;
            }
        }

        return lhs.cross_domain_values_ == rhs.cross_domain_values_;
    }

  private:
    /// Number of domain pairs with a value set
    size_t num_valid_ = 0;

    /// Whether each domain has a value set for the pair (domain, domain)
    std::vector<bool> same_domain_valid_;

    /// The value of each (domain, domain) pair
    std::vector<T> same_domain_values_;

    /// The value of each set pair whose launch and capture domains differ
    std::map<DomainPair, T> cross_domain_values_;
};
