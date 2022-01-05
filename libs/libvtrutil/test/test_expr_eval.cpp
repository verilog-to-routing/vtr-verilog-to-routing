#include <limits>

#include "catch2/catch_test_macros.hpp"

#include "vtr_expr_eval.h"

TEST_CASE("Simple Expressions", "[vtr_expr_eval]") {
    vtr::FormulaParser parser;
    vtr::t_formula_data vars;

    REQUIRE(parser.parse_formula("0", vars) == 0);
    REQUIRE(parser.parse_formula("42", vars) == 42);

    REQUIRE(parser.parse_formula("5 + 2", vars) == 7);
    REQUIRE(parser.parse_formula("5 + 10", vars) == 15);
    REQUIRE(parser.parse_formula("5 - 2", vars) == 3);
    REQUIRE(parser.parse_formula("5 - 10", vars) == -5);

    REQUIRE(parser.parse_formula("5 * 5", vars) == 25);
    REQUIRE(parser.parse_formula("5 / 5", vars) == 1);

    //Floor arithmetic
    REQUIRE(parser.parse_formula("5 / 10", vars) == 0);
    REQUIRE(parser.parse_formula("10 / 9", vars) == 1);

    REQUIRE(parser.parse_formula("5 % 10", vars) == 5);
    REQUIRE(parser.parse_formula("10 % 9", vars) == 1);

    REQUIRE(parser.parse_formula("5 < 10", vars) == 1);
    REQUIRE(parser.parse_formula("20 < 10", vars) == 0);

    REQUIRE(parser.parse_formula("5 > 10", vars) == 0);
    REQUIRE(parser.parse_formula("20 > 10", vars) == 1);
}

TEST_CASE("Negative Literals", "[vtr_expr_eval]") {
    //TODO: Currently unsupported, should support in the future...
    //REQUIRE(parser.parse_formula("-5 + 10", vars) == 5);
    //REQUIRE(parser.parse_formula("-10 + 5", vars) == -5);
    //REQUIRE(parser.parse_formula("-1", vars) == -1);
}

TEST_CASE("Bracket Expressions", "[vtr_expr_eval]") {
    vtr::FormulaParser parser;
    vtr::t_formula_data vars;

    REQUIRE(parser.parse_formula("20 / (4 + 1)", vars) == 4);
    REQUIRE(parser.parse_formula("(20 / 5) + 1", vars) == 5);
    REQUIRE(parser.parse_formula("20 / 5 + 1", vars) == 5);
}

TEST_CASE("Variable Expressions", "[vtr_expr_eval]") {
    vtr::FormulaParser parser;
    vtr::t_formula_data vars;
    vars.set_var_value("x", 5);
    vars.set_var_value("y", 10);

    REQUIRE(parser.parse_formula("x", vars) == 5);
    REQUIRE(parser.parse_formula("y", vars) == 10);

    REQUIRE(parser.parse_formula("x + y", vars) == 15);
    REQUIRE(parser.parse_formula("y + x", vars) == 15);

    REQUIRE(parser.parse_formula("x - y", vars) == -5);
    REQUIRE(parser.parse_formula("y - x", vars) == 5);

    REQUIRE(parser.parse_formula("x * y", vars) == 50);
    REQUIRE(parser.parse_formula("y * x", vars) == 50);

    REQUIRE(parser.parse_formula("x / y", vars) == 0);
    REQUIRE(parser.parse_formula("y / x", vars) == 2);
}

TEST_CASE("Function Expressions", "[vtr_expr_eval]") {
    vtr::FormulaParser parser;
    vtr::t_formula_data vars;

    REQUIRE(parser.parse_formula("min(5, 2)", vars) == 2);
    REQUIRE(parser.parse_formula("min(2, 5)", vars) == 2);
    //REQUIRE(parser.parse_formula("min(-5, 2)", vars) == -5); //Negative literals currently unsupported
    //REQUIRE(parser.parse_formula("min(-2, 5)", vars) == -2);

    REQUIRE(parser.parse_formula("max(5, 2)", vars) == 5);
    REQUIRE(parser.parse_formula("max(2, 5)", vars) == 5);
    //REQUIRE(parser.parse_formula("max(-5, 2)", vars) == 2); //Negative literals currently unsupported
    //REQUIRE(parser.parse_formula("max(-2, 5)", vars) == 5);

    REQUIRE(parser.parse_formula("gcd(20, 25)", vars) == 5);
    REQUIRE(parser.parse_formula("lcm(20, 25)", vars) == 100);
}
