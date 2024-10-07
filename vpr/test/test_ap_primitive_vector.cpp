/**
 * @file
 * @author  Alex Singer
 * @date    October 2024
 * @brief   Unit tests for the PrimitiveVector object
 *
 * Very quick functionality checks to make sure that the methods inside of the
 * PrimitiveVector object are working as expected.
 */

#include "catch2/catch_test_macros.hpp"
#include "primitive_vector.h"

namespace {

TEST_CASE("test_ap_primitive_vector_verify", "[vpr_ap]") {
    SECTION("Test getters and setters") {
        PrimitiveVector vec;
        // Default value in the vector should be zero.
        REQUIRE(vec.get_dim_val(42) == 0.f);
        // Able to set a random dim to a value.
        vec.set_dim_val(42, 2.f);
        REQUIRE(vec.get_dim_val(42) == 2.f);
        // Able to add a value to a dim.
        vec.add_val_to_dim(10.f, 42);
        REQUIRE(vec.get_dim_val(42) == 12.f);
        // Try a negative number.
        vec.set_dim_val(0, -2.f);
        REQUIRE(vec.get_dim_val(0) == -2.f);
        vec.add_val_to_dim(-4.f, 42);
        REQUIRE(vec.get_dim_val(42) == 8.f);
        // Try setting to zero.
        vec.set_dim_val(42, 0.f);
        REQUIRE(vec.get_dim_val(42) == 0.f);

        // Test clear method.
        vec.clear();
        REQUIRE(vec.get_dim_val(42) == 0.f);
        REQUIRE(vec.get_dim_val(0) == 0.f);
    }
    SECTION("Test operators") {
        PrimitiveVector vec1, vec2;

        // Equality:
        // Two empty vectors should be equal.
        REQUIRE(vec1 == vec2);
        vec1.set_dim_val(0, 0.f);
        vec1.set_dim_val(1, 1.f);
        vec1.set_dim_val(2, 2.f);
        // Compare with self.
        REQUIRE(vec1 == vec1);
        // Set vec2 indirectly to vec 1
        vec2.set_dim_val(0, 0.f);
        vec2.set_dim_val(1, 1.f);
        vec2.set_dim_val(2, 2.f);
        REQUIRE(vec1 == vec2);
        // Check commutivity
        REQUIRE(vec2 == vec1);
        // Check copy constructor.
        PrimitiveVector vec3 = vec1;
        REQUIRE(vec1 == vec3);
        // Check strange corner-case where 1 vec has more dims set than another.
        PrimitiveVector vec4 = vec1;
        vec4.set_dim_val(10, 100.f);
        REQUIRE(!(vec4 == vec1));
        REQUIRE(!(vec1 == vec4));

        // Inequality:
        // Set vec2 to not be equal
        vec2.set_dim_val(0, 3.f);
        REQUIRE(!(vec1 == vec2));
        REQUIRE(vec1 != vec2);
        REQUIRE(vec2 != vec1);
        vec2.set_dim_val(0, 0.f);
        vec2.set_dim_val(3, 3.f);
        REQUIRE(!(vec1 == vec2));
        REQUIRE(vec1 != vec2);
        // Set a random dim to 0. By default all dims are 0.
        vec2 = vec1;
        vec2.set_dim_val(10, 0.f);
        REQUIRE(vec1 == vec2);

        // Accumulation:
        vec1.clear();
        REQUIRE(vec1 == PrimitiveVector());
        vec1.set_dim_val(0, 0.f);
        vec1.set_dim_val(1, 1.f);
        vec1.set_dim_val(2, 2.f);
        vec2.clear();
        vec2.set_dim_val(0, 3.f);
        vec2.set_dim_val(1, 4.f);
        vec2.set_dim_val(2, 5.f);
        vec1 += vec2;
        PrimitiveVector res;
        res.set_dim_val(0, 3.f);
        res.set_dim_val(1, 5.f);
        res.set_dim_val(2, 7.f);
        REQUIRE(vec1 == res);
        // accumulate different dims
        vec1.clear();
        vec1.set_dim_val(0, 10.f);
        vec2.clear();
        vec2.set_dim_val(1, 20.f);
        vec1 += vec2;
        REQUIRE(vec1.get_dim_val(0) == 10.f);
        REQUIRE(vec1.get_dim_val(1) == 20.f);

        // Subtraction:
        vec1 -= vec2;
        REQUIRE(vec1.get_dim_val(0) == 10.f);
        REQUIRE(vec1.get_dim_val(1) == 0.f);
        res = vec1;
        res -= vec2;
        REQUIRE(vec1 - vec2 == res);

        // Element-wise multiplication:
        vec1.clear();
        vec1.set_dim_val(0, 0.f);
        vec1.set_dim_val(1, 1.f);
        vec1.set_dim_val(2, 2.f);
        vec1 *= 2.f;
        REQUIRE(vec1.get_dim_val(0) == 0.f);
        REQUIRE(vec1.get_dim_val(1) == 2.f);
        REQUIRE(vec1.get_dim_val(2) == 4.f);
    }
    SECTION("Test comparitors") {
        PrimitiveVector vec1, vec2;
        // empty vector.
        vec2.set_dim_val(0, 10.f);
        vec2.set_dim_val(1, 20.f);
        REQUIRE(vec1 < vec2);
        // 1D case.
        vec1.clear();
        vec2.clear();
        vec1.set_dim_val(0, 1.f);
        vec2.set_dim_val(0, 2.f);
        REQUIRE(vec1 < vec2);
        vec1.set_dim_val(0, 2.f);
        REQUIRE(!(vec1 < vec2));
        vec1.set_dim_val(0, 3.f);
        REQUIRE(!(vec1 < vec2));
        // 2D case.
        vec1.clear();
        vec2.clear();
        vec1.set_dim_val(0, 1.f);
        vec1.set_dim_val(1, 1.f);
        vec2.set_dim_val(0, 2.f);
        vec2.set_dim_val(1, 2.f);
        REQUIRE(vec1 < vec2);
        // NOTE: This is somewhat special. Since 1 dimension is less for vec1
        //       it should still be less.
        vec1.set_dim_val(0, 3.f);
        REQUIRE(vec1 < vec2);
        vec1.set_dim_val(1, 3.f);
        REQUIRE(!(vec1 < vec2));
    }
    SECTION("Test methods") {
        PrimitiveVector vec1;
        // is_zero:
        // The default vector is zero.
        REQUIRE(vec1.is_zero());
        // Setting an element of the zero-vector to 0 is still a zero vector.
        vec1.set_dim_val(0, 0.f);
        REQUIRE(vec1.is_zero());
        vec1.set_dim_val(42, 0.f);
        REQUIRE(vec1.is_zero());
        vec1.set_dim_val(42, 1.f);
        REQUIRE(!vec1.is_zero());
        REQUIRE(vec1.is_non_zero());
        vec1.set_dim_val(42, 0.f);
        REQUIRE(vec1.is_zero());
        REQUIRE(!vec1.is_non_zero());

        // relu:
        vec1.clear();
        // Relu of the zero vector is still the zero vector.
        vec1.relu();
        REQUIRE(vec1.is_zero());
        // Relu of a negative vector is the zero vector.
        vec1.set_dim_val(0, -1.f);
        vec1.set_dim_val(1, -2.f);
        vec1.relu();
        REQUIRE(vec1.is_zero());
        // Relu of a positive vector is the same vector.
        vec1.set_dim_val(0, 1.f);
        vec1.set_dim_val(1, 2.f);
        PrimitiveVector vec2 = vec1;
        vec1.relu();
        REQUIRE(vec1 == vec2);
        // Standard Relu test.
        vec1.set_dim_val(0, 1.f);
        vec1.set_dim_val(1, 0.f);
        vec1.set_dim_val(2, -4.f);
        vec1.set_dim_val(3, 2.f);
        vec1.set_dim_val(4, -5.f);
        vec2 = vec1;
        vec1.relu();
        vec2.set_dim_val(2, 0.f);
        vec2.set_dim_val(4, 0.f);
        REQUIRE(vec1 == vec2);

        // is_non_negative:
        vec1.clear();
        // The zero vector is non-negative.
        REQUIRE(vec1.is_non_negative());
        vec1.set_dim_val(0, 0.f);
        REQUIRE(vec1.is_non_negative());
        // Postive vector is non-negative
        vec1.set_dim_val(0, 1.f);
        REQUIRE(vec1.is_non_negative());
        vec1.set_dim_val(1, 2.f);
        REQUIRE(vec1.is_non_negative());
        // Negative vector is negative.
        vec2.clear();
        vec2.set_dim_val(0, -1.f);
        REQUIRE(!vec2.is_non_negative());
        vec2.set_dim_val(1, -2.f);
        REQUIRE(!vec2.is_non_negative());
        // Mixed positive and negative vector is not non-negative.
        vec2.set_dim_val(1, 2.f);
        REQUIRE(!vec2.is_non_negative());
        vec2.set_dim_val(0, 1.f);
        REQUIRE(vec1.is_non_negative());

        // manhattan_norm:
        vec1.clear();
        // Manhatten norm of the zero vector is zero.
        REQUIRE(vec1.manhattan_norm() == 0.f);
        // Manhatten norm of a non-negative vector is the sum of its dims.
        vec1.set_dim_val(0, 1.f);
        REQUIRE(vec1.manhattan_norm() == 1.f);
        vec1.set_dim_val(1, 2.f);
        vec1.set_dim_val(2, 3.f);
        vec1.set_dim_val(3, 4.f);
        vec1.set_dim_val(4, 5.f);
        REQUIRE(vec1.manhattan_norm() == 15.f);
        // Manhatten norm of a negative vector is the sum of the absolute value
        // of its dims.
        vec2 = vec1;
        vec2 *= -1.f;
        REQUIRE(vec2.manhattan_norm() == vec1.manhattan_norm());

        // Projection:
        // Basic example:
        vec1.clear();
        vec1.set_dim_val(0, 12.f);
        vec1.set_dim_val(1, 32.f);
        vec1.set_dim_val(2, 8.f);
        vec1.set_dim_val(3, 2.f);
        vec2.clear();
        vec2.set_dim_val(0, 2.f);
        vec2.set_dim_val(2, 2.f);
        vec1.project(vec2);
        PrimitiveVector res;
        res.set_dim_val(0, 12.f);
        res.set_dim_val(2, 8.f);
        REQUIRE(vec1 == res);
        // Projecting onto the same vector again should give the same answer.
        vec1.project(vec2);
        REQUIRE(vec1 == res);
        // Projecting onto the same dimensions should not change the vector.
        vec1.clear();
        vec1.set_dim_val(0, 1.f);
        vec1.set_dim_val(1, 2.f);
        vec2.clear();
        vec2.set_dim_val(0, 3.f);
        vec2.set_dim_val(1, 4.f);
        res = vec1;
        vec1.project(vec2);
        REQUIRE(vec1 == res);
        // Projecting onto higher dimensions should not change the vector.
        vec2.set_dim_val(2, 5.f);
        res = vec1;
        vec1.project(vec2);
        REQUIRE(vec1 == res);

        // Max of two vectors:
        // The max of the zero vectors is the zero vector.
        vec1.clear();
        vec2.clear();
        res = PrimitiveVector::max(vec1, vec2);
        REQUIRE(res.is_zero());
        // The max of a non-negative vector with the zero vector is the non-
        // negative vector.
        vec1.set_dim_val(0, 1.f);
        res = PrimitiveVector::max(vec1, vec2);
        REQUIRE(res == vec1);
        res = PrimitiveVector::max(vec2, vec1);
        REQUIRE(res == vec1);
        // The max of a negative vector with the zero vector is the zero vector.
        vec1.set_dim_val(0, -1.f);
        res = PrimitiveVector::max(vec1, vec2);
        REQUIRE(res.is_zero());
        // Basic test:
        // max(<5, 9, 0>, <3, 10, -2>) = <5, 10, 0>
        vec1.clear();
        vec1.set_dim_val(0, 5.f);
        vec1.set_dim_val(1, 9.f);
        vec1.set_dim_val(2, 0.f);
        vec2.clear();
        vec2.set_dim_val(0, 3.f);
        vec2.set_dim_val(1, 10.f);
        vec2.set_dim_val(2, -2.f);
        PrimitiveVector golden;
        golden.set_dim_val(0, 5.f);
        golden.set_dim_val(1, 10.f);
        golden.set_dim_val(2, 0.f);
        res = PrimitiveVector::max(vec1, vec2);
        REQUIRE(res == golden);
        res = PrimitiveVector::max(vec2, vec1);
        REQUIRE(res == golden);
    }
}

} // namespace

