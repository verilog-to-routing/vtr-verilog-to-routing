#include "catch.hpp"

#include "vtr_array_view.h"
#include <array>

TEST_CASE("Array view", "[array_view/array_view]") {
    std::array<uint16_t, 10> arr;
    vtr::array_view<uint16_t> arr_view(arr.data(), arr.size());

    const vtr::array_view<uint16_t>& carr_view = arr_view;
    const vtr::array_view<uint16_t> carr_view2 = arr_view;
    const vtr::array_view<uint16_t> carr_view3(arr_view);

    REQUIRE(arr.size() == arr_view.size());
    REQUIRE(arr.data() == arr_view.data());
    REQUIRE(arr.data() == carr_view.data());
    REQUIRE(arr.data() == carr_view2.data());
    REQUIRE(arr.data() == carr_view3.data());

    for (size_t i = 0; i < arr.size(); ++i) {
        arr[i] = i;
    }

    for (size_t i = 0; i < arr_view.size(); ++i) {
        REQUIRE(arr_view[i] == i);
        REQUIRE(carr_view[i] == i);
        REQUIRE(carr_view2[i] == i);
        REQUIRE(carr_view3[i] == i);
    }

    for (size_t i = 0; i < arr.size(); ++i) {
        REQUIRE(&arr[i] == &arr_view[i]);
        REQUIRE(&arr.at(i) == &arr_view.at(i));
        REQUIRE(&arr[i] == &carr_view[i]);
        REQUIRE(&arr.at(i) == &carr_view.at(i));
        REQUIRE(&arr[i] == &carr_view2[i]);
        REQUIRE(&arr.at(i) == &carr_view2.at(i));
        REQUIRE(&arr[i] == &carr_view3[i]);
        REQUIRE(&arr.at(i) == &carr_view3.at(i));
    }

    for (size_t i = 0; i < arr_view.size(); ++i) {
        arr_view[i] = arr_view.size() - i;
    }

    for (size_t i = 0; i < arr.size(); ++i) {
        REQUIRE(arr[i] == (arr_view.size() - i));
        REQUIRE(carr_view[i] == (arr_view.size() - i));
        REQUIRE(carr_view2[i] == (arr_view.size() - i));
        REQUIRE(carr_view3[i] == (arr_view.size() - i));
    }
}
