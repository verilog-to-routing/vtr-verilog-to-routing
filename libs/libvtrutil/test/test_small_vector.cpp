#include "catch.hpp"

#include "vtr_small_vector.h"
#include <vector>

namespace vtr {

//Must be delcared in namespace for argument dependent lookup to work with clang
template<class T>
bool operator==(const std::vector<T>& lhs, const vtr::small_vector<T>& rhs) {
    if (lhs.size() != rhs.size()) return false;

    for (size_t i = 0; i < lhs.size(); ++i) {
        if (lhs[i] != rhs[i]) return false;
    }
    return true;
}

} // namespace vtr

TEST_CASE("Basic", "[vtr_small_vector]") {
    std::vector<int> ref;
    vtr::small_vector<int> vec;

    //Create the vectors the same way
    int i;
    for (i = 0; i < 100; i++) {
        ref.push_back(i);
        vec.push_back(i);

        REQUIRE(ref == vec);
    }

    //Check forward iteration
    auto vec_itr = vec.begin();
    for (auto ref_itr = ref.begin(); ref_itr != ref.end(); ++ref_itr, ++vec_itr) {
        REQUIRE(*ref_itr == *vec_itr);

        int dist = std::distance(ref.begin(), ref_itr);
        REQUIRE(ref[dist] == vec[dist]);
    }

    //Check backward iteration
    auto vec_rev_itr = vec.rbegin();
    for (auto ref_itr = ref.rbegin(); ref_itr != ref.rend(); ++ref_itr, ++vec_rev_itr) {
        REQUIRE(*ref_itr == *vec_rev_itr);
    }

    //Check front/back
    REQUIRE(ref.front() == vec.front());
    REQUIRE(ref.back() == vec.back());

    //Push/Emplace/Pop back
    ref.push_back(i);
    vec.push_back(i);
    REQUIRE(ref == vec);
    ++i;

    ref.emplace_back(i);
    vec.emplace_back(i);
    REQUIRE(ref == vec);
    ++i;

    ref.pop_back();
    vec.pop_back();
    REQUIRE(ref == vec);

    //Test the short (internal storage) transition
    size_t inplace_cap = vtr::small_vector<int>::INPLACE_CAPACITY;
    REQUIRE(inplace_cap > 1);

    //From long to short
    ref.resize(inplace_cap + 1);
    vec.resize(inplace_cap + 1);
    REQUIRE(ref == vec);
    REQUIRE(vec.size() > inplace_cap);

    ref.pop_back();
    vec.pop_back();
    REQUIRE(ref == vec);
    REQUIRE(vec.size() == inplace_cap);

    ref.pop_back();
    vec.pop_back();
    REQUIRE(ref == vec);
    REQUIRE(vec.size() < inplace_cap);

    //From short to long
    ref.push_back(i);
    vec.push_back(i);
    REQUIRE(ref == vec);
    REQUIRE(vec.size() == inplace_cap);
    ++i;

    ref.push_back(i);
    vec.push_back(i);
    REQUIRE(ref == vec);
    REQUIRE(vec.size() > inplace_cap);
    ++i;

#if 0
    //Emplace at position
    auto ref_itr = ref.begin() + ref.size() / 2;
    ref.emplace(ref_itr, i);
    vec_itr = vec.begin() + vec.size() / 2;
    vec.emplace(vec_itr, i);
    i++;
    REQUIRE(ref == vec);
#endif

    //Insert single at position
    auto ref_itr = ref.begin() + ref.size() / 2;
    ref.insert(ref_itr, i);
    vec_itr = vec.begin() + vec.size() / 2;
    vec.insert(vec_itr, i);
    i++;
    REQUIRE(ref == vec);

    //Insert K at position
    int k = 5;
    ref_itr = ref.begin() + ref.size() / 2;
    ref.insert(ref_itr, k, i);
    vec_itr = vec.begin() + vec.size() / 2;
    vec.insert(vec_itr, k, i);
    i++;
    REQUIRE(ref == vec);

    //Range insert
    std::vector<int> range_values = {5, 4, 3, 2, 1};
    ref_itr = ref.begin() + ref.size() / 2;
    ref.insert(ref_itr, range_values.begin(), range_values.end());
#if 0
    vec_itr = vec.begin() + vec.size() / 2;
    vec.insert(vec_itr, range_values.begin(), range_values.end());
    REQUIRE(ref == vec);
#endif

    //Clear
    ref.clear();
    vec.clear();
    REQUIRE(ref == vec);

    //Add after clear
    ref.push_back(i);
    vec.push_back(i);
    REQUIRE(ref == vec);
    ++i;
}
