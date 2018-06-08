#include "catch.hpp"

#include "hlc.h"

TEST_CASE("coord_operator_eq", "[hlc]") {
    t_hlc_coord c1(1, 2);
    t_hlc_coord c2(1, 2);
    t_hlc_coord c3(2, 3);
    t_hlc_coord c4(3, 4);

    // Check the coords are equal when equal.
    REQUIRE(c1 == c1);
    REQUIRE(c1 == c2);
    REQUIRE(c1 != c3);
    REQUIRE(c1 != c4);

    REQUIRE(c2 == c1);
    REQUIRE(c2 == c2);
    REQUIRE(c2 != c3);
    REQUIRE(c2 != c4);

    REQUIRE(c3 != c1);
    REQUIRE(c3 != c2);
    REQUIRE(c3 == c3);
    REQUIRE(c3 != c4);

    REQUIRE(c4 != c1);
    REQUIRE(c4 != c2);
    REQUIRE(c4 != c3);
    REQUIRE(c4 == c4);
}

TEST_CASE("edge_operator_eq", "[hlc]") {
    t_hlc_edge e1("a", "b", HLC_SW_END);
    t_hlc_edge e2("a", "b", HLC_SW_END);
    t_hlc_edge e3("a", "b", HLC_SW_SHORT);
    t_hlc_edge e4("a", "c", HLC_SW_END);
    t_hlc_edge e5("c", "d", HLC_SW_END);

    // Check the edges are equal when equal.
    REQUIRE(e1 == e1);
    REQUIRE(e1 == e2);
    REQUIRE(e1 != e3);
    REQUIRE(e1 != e4);
    REQUIRE(e1 != e5);

    REQUIRE(e2 == e1);
    REQUIRE(e2 == e2);
    REQUIRE(e2 != e3);
    REQUIRE(e2 != e4);
    REQUIRE(e2 != e5);

    REQUIRE(e3 != e1);
    REQUIRE(e3 != e2);
    REQUIRE(e3 == e3);
    REQUIRE(e3 != e4);
    REQUIRE(e3 != e5);

    REQUIRE(e4 != e1);
    REQUIRE(e4 != e2);
    REQUIRE(e4 != e3);
    REQUIRE(e4 == e4);
    REQUIRE(e4 != e5);

    REQUIRE(e5 != e1);
    REQUIRE(e5 != e2);
    REQUIRE(e5 != e3);
    REQUIRE(e5 != e4);
    REQUIRE(e5 == e5);
}

TEST_CASE("edge_operator_gt", "[hlc]") {
    t_hlc_edge e1("a", "b", HLC_SW_SHORT);
    t_hlc_edge e2("a", "b", HLC_SW_END);
    t_hlc_edge e3("a", "b", HLC_SW_END);
    t_hlc_edge e4("a", "c", HLC_SW_END);
    t_hlc_edge e5("c", "d", HLC_SW_END);

    // Check the edges are equal when equal.
    REQUIRE(!(e1 < e1));
    REQUIRE( (e1 < e2));
    REQUIRE( (e1 < e3));
    REQUIRE( (e1 < e4));
    REQUIRE( (e1 < e5));

    REQUIRE(!(e2 < e1));
    REQUIRE(!(e2 < e2));
    REQUIRE(!(e2 < e3));
    REQUIRE( (e2 < e4));
    REQUIRE( (e2 < e5));

    REQUIRE(!(e3 < e1));
    REQUIRE(!(e3 < e2));
    REQUIRE(!(e3 < e3));
    REQUIRE( (e3 < e4));
    REQUIRE( (e3 < e5));

    REQUIRE(!(e4 < e1));
    REQUIRE(!(e4 < e2));
    REQUIRE(!(e4 < e3));
    REQUIRE(!(e4 < e4));
    REQUIRE( (e4 < e5));

    REQUIRE(!(e5 < e1));
    REQUIRE(!(e5 < e2));
    REQUIRE(!(e5 < e3));
    REQUIRE(!(e5 < e4));
    REQUIRE(!(e5 < e5));
}

TEST_CASE("edge_operator_hash", "[hlc]") {
    t_hlc_edge e1("a", "b", HLC_SW_SHORT);
    t_hlc_edge e2("a", "b", HLC_SW_END);
    t_hlc_edge e3("a", "b", HLC_SW_END);
    t_hlc_edge e4("a", "c", HLC_SW_END);
    t_hlc_edge e5("c", "d", HLC_SW_END);
    t_hlc_edge e6("a", "bb", HLC_SW_END);

    std::size_t h1 = std::hash<t_hlc_edge>{}(e1);
    std::size_t h2 = std::hash<t_hlc_edge>{}(e2);
    std::size_t h3 = std::hash<t_hlc_edge>{}(e3);
    std::size_t h4 = std::hash<t_hlc_edge>{}(e4);
    std::size_t h5 = std::hash<t_hlc_edge>{}(e5);
    std::size_t h6 = std::hash<t_hlc_edge>{}(e6);

    // Check the edges are equal when equal.
    REQUIRE(h1 == h1);
    REQUIRE(h1 == h2);
    REQUIRE(h1 == h3);
    REQUIRE(h1 != h4);
    REQUIRE(h1 != h5);
    REQUIRE(h1 != h6);

    REQUIRE(h2 == h1);
    REQUIRE(h2 == h2);
    REQUIRE(h2 == h3);
    REQUIRE(h2 != h4);
    REQUIRE(h2 != h5);
    REQUIRE(h2 != h6);

    REQUIRE(h3 == h1);
    REQUIRE(h3 == h2);
    REQUIRE(h3 == h3);
    REQUIRE(h3 != h4);
    REQUIRE(h3 != h5);
    REQUIRE(h3 != h6);

    REQUIRE(h4 != h1);
    REQUIRE(h4 != h2);
    REQUIRE(h4 != h3);
    REQUIRE(h4 == h4);
    REQUIRE(h4 != h5);
    REQUIRE(h4 != h6);

    REQUIRE(h5 != h1);
    REQUIRE(h5 != h2);
    REQUIRE(h5 != h3);
    REQUIRE(h5 != h4);
    REQUIRE(h5 == h5);
    REQUIRE(h5 != h6);

    REQUIRE(h6 != h1);
    REQUIRE(h6 != h2);
    REQUIRE(h6 != h3);
    REQUIRE(h6 != h4);
    REQUIRE(h6 != h5);
    REQUIRE(h6 == h6);

    std::set<t_hlc_edge> s;
    REQUIRE(s.empty());
    // Adding elements should increase the size.
    REQUIRE(s.size() == 0);
    REQUIRE(s.count(e1) == 0);
    REQUIRE(s.count(e2) == 0);
    REQUIRE(s.count(e3) == 0);
    REQUIRE(s.count(e4) == 0);
    REQUIRE(s.count(e5) == 0);
    REQUIRE(s.count(e6) == 0);

    s.insert(e1);
    REQUIRE(s.size() == 1);
    REQUIRE(s.count(e1) == 1);
    REQUIRE(s.count(e2) == 0);
    REQUIRE(s.count(e3) == 0);
    REQUIRE(s.count(e4) == 0);
    REQUIRE(s.count(e5) == 0);
    REQUIRE(s.count(e6) == 0);

    s.insert(e2);
    REQUIRE(s.size() == 2);
    REQUIRE(s.count(e1) == 1);
    REQUIRE(s.count(e2) == 1);
    REQUIRE(s.count(e3) == 1);
    REQUIRE(s.count(e4) == 0);
    REQUIRE(s.count(e5) == 0);
    REQUIRE(s.count(e6) == 0);

    // Adding element already in the set shouldn't increase the number.
    s.insert(e3);
    REQUIRE(s.size() == 2);

    REQUIRE(s.count(e1) == 1);
    REQUIRE(s.count(e2) == 1);
    REQUIRE(s.count(e3) == 1);
    REQUIRE(s.count(e4) == 0);
    REQUIRE(s.count(e5) == 0);
    REQUIRE(s.count(e6) == 0);

    // Adding elements should increase the size.
    s.insert(e4);
    REQUIRE(s.size() == 3);

    REQUIRE(s.count(e1) == 1);
    REQUIRE(s.count(e2) == 1);
    REQUIRE(s.count(e3) == 1);
    REQUIRE(s.count(e4) == 1);
    REQUIRE(s.count(e5) == 0);
    REQUIRE(s.count(e6) == 0);

    // Adding elements should increase the size.
    s.insert(e5);
    REQUIRE(s.size() == 4);

    REQUIRE(s.count(e1) == 1);
    REQUIRE(s.count(e2) == 1);
    REQUIRE(s.count(e3) == 1);
    REQUIRE(s.count(e4) == 1);
    REQUIRE(s.count(e5) == 1);
    REQUIRE(s.count(e6) == 0);

    // Adding elements should increase the size.
    s.erase(e2);
    REQUIRE(s.size() == 3);

    REQUIRE(s.count(e1) == 1);
    REQUIRE(s.count(e2) == 0);
    REQUIRE(s.count(e3) == 0);
    REQUIRE(s.count(e4) == 1);
    REQUIRE(s.count(e5) == 1);
    REQUIRE(s.count(e6) == 0);
}

TEST_CASE("cell_operator_eq", "[hlc]") {
    t_hlc_cell c1("a", 0);
    t_hlc_cell c2("a", 0);
    t_hlc_cell c3("a", 1);
    t_hlc_cell c4("c", 0);

    // Check the cells are equal when equal.
    REQUIRE(c1 == c1);
    REQUIRE(c1 == c2);
    REQUIRE(c1 != c3);
    REQUIRE(c1 != c4);

    REQUIRE(c2 == c1);
    REQUIRE(c2 == c2);
    REQUIRE(c2 != c3);
    REQUIRE(c2 != c4);

    REQUIRE(c3 != c1);
    REQUIRE(c3 != c2);
    REQUIRE(c3 == c3);
    REQUIRE(c3 != c4);

    REQUIRE(c4 != c1);
    REQUIRE(c4 != c2);
    REQUIRE(c4 != c3);
    REQUIRE(c4 == c4);
}

TEST_CASE("cell_operator_hash", "[hlc]") {
    t_hlc_cell c1("a", 0);
    t_hlc_cell c2("a", 0);
    t_hlc_cell c3("a", 1);
    t_hlc_cell c4("c", 0);

    std::size_t h1 = std::hash<t_hlc_cell>{}(c1);
    std::size_t h2 = std::hash<t_hlc_cell>{}(c2);
    std::size_t h3 = std::hash<t_hlc_cell>{}(c3);
    std::size_t h4 = std::hash<t_hlc_cell>{}(c4);

    // Check the cells are equal when equal.
    REQUIRE(h1 == h1);
    REQUIRE(h1 == h2);
    REQUIRE(h1 != h3);
    REQUIRE(h1 != h4);

    REQUIRE(h2 == h1);
    REQUIRE(h2 == h2);
    REQUIRE(h2 != h3);
    REQUIRE(h2 != h4);

    REQUIRE(h3 != h1);
    REQUIRE(h3 != h2);
    REQUIRE(h3 == h3);
    REQUIRE(h3 != h4);

    REQUIRE(h4 != h1);
    REQUIRE(h4 != h2);
    REQUIRE(h4 != h3);
    REQUIRE(h4 == h4);

    std::set<t_hlc_cell> s;
    REQUIRE(s.empty());
    // Adding elements should increase the size.
    REQUIRE(s.size() == 0);
    REQUIRE(s.count(c1) == 0);
    REQUIRE(s.count(c2) == 0);
    REQUIRE(s.count(c3) == 0);
    REQUIRE(s.count(c4) == 0);

    s.insert(c1);
    REQUIRE(s.size() == 1);
    REQUIRE(s.count(c1) == 1);
    REQUIRE(s.count(c2) == 1);
    REQUIRE(s.count(c3) == 0);
    REQUIRE(s.count(c4) == 0);

    // Adding element already in the set shouldn't increase the number.
    s.insert(c2);
    REQUIRE(s.size() == 1);

    REQUIRE(s.count(c1) == 1);
    REQUIRE(s.count(c2) == 1);
    REQUIRE(s.count(c3) == 0);
    REQUIRE(s.count(c4) == 0);

    // Adding elements should increase the size.
    s.insert(c3);
    REQUIRE(s.size() == 2);

    REQUIRE(s.count(c1) == 1);
    REQUIRE(s.count(c2) == 1);
    REQUIRE(s.count(c3) == 1);
    REQUIRE(s.count(c4) == 0);

    // Adding elements should increase the size.
    s.erase(c2);
    REQUIRE(s.size() == 1);

    REQUIRE(s.count(c1) == 0);
    REQUIRE(s.count(c2) == 0);
    REQUIRE(s.count(c3) == 1);
    REQUIRE(s.count(c4) == 0);
}

TEST_CASE("parse_cell_name", "[hlc]") {
    auto k0 = parse_cell_name("abc");
    REQUIRE(k0.name == std::string("global"));
    REQUIRE(k0.i == 0);

    auto k1 = parse_cell_name("abc_0");
    REQUIRE(k1.name == std::string("abc"));
    REQUIRE(k1.i == 0);

    auto k2 = parse_cell_name("def_2");
    REQUIRE(k2.name == std::string("def"));
    REQUIRE(k2.i == 2);

    auto k3 = parse_cell_name("a_b_c");
    REQUIRE(k3.name == std::string("global"));
    REQUIRE(k3.i == 0);
}

TEST_CASE("parse_net_name", "[hlc]") {
    auto c0 = parse_net_name("netname");
    auto c1 = parse_net_name("cell_2/netname");

    REQUIRE(c0.first.name == "global");
    REQUIRE(c0.first.i == 0);
    REQUIRE(c0.second == "netname");

    REQUIRE(c1.first.name == "cell");
    REQUIRE(c1.first.i == 2);
    REQUIRE(c1.second == "netname");
}

TEST_CASE("indent", "[hlc]") {
    std::string s0("");
    std::stringstream os0;
    print_indent(os0, "^", s0);
    REQUIRE(std::string("") == os0.str());

    std::string s1(" ");
    std::stringstream os1;
    print_indent(os1, "^", s1);
    REQUIRE(std::string("^ ") == os1.str());

    std::string s3("\n\n");
    std::stringstream os3;
    print_indent(os3, "-", s3);
    REQUIRE(std::string("-\n-\n") == os3.str());

    std::string s4("a\n\n123");
    std::stringstream os4;
    print_indent(os4, "#", s4);
    REQUIRE(std::string("#a\n#\n#123") == os4.str());

    std::string s5("b\nc\n");
    std::stringstream os5;
    print_indent(os5, "--", s5);
    REQUIRE(std::string("--b\n--c\n") == os5.str());
}


TEST_CASE("test", "[hlc]") {
    t_hlc_file f;
    f.comments << "File comment";

    auto t1 = f.get_tile(0, 0);
    //t1->type = HLC_TILE_LOGIC;
    t1->name = "logic_tile";
    // Path which goes from one cell to another via a local track.
    t1->enable_edge("lutff_5/out", "local_g0_5", HLC_SW_BUFFER)
        << " Edge comment 1";
    t1->enable_edge("local_g0_5", "lutff_6/in_1", HLC_SW_BUFFER);

    // Path which leaves the via two different span tracks.
    t1->enable_edge("local_g0_5", "span12_y7_g14_0", HLC_SW_BUFFER)
        << " Edge comment 2";
    t1->enable_edge("span12_y7_g14_0", "span12_x2_g12_0", HLC_SW_ROUTING)
        << " Edge comment 3a" << std::endl
        << " Edge comment 3b";
    t1->enable_edge("span12_y7_g14_0", "span12_x3_g19_0", HLC_SW_ROUTING);
    // Enable an edge which already enabled..
    t1->enable_edge("span12_y7_g14_0", "span12_x3_g19_0", HLC_SW_ROUTING);
    // Enable an edge which already enabled with another comment
    t1->enable_edge("span12_y7_g14_0", "span12_x3_g19_0", HLC_SW_ROUTING)
        << " Edge comment 4";

    auto c15 = t1->get_cell("lutff_5");
    c15->enable("out", "in_3 ^ (in_0 & in_1 & in_2)");
    c15->enable("enable_dff")
        << " Enable the flip flop";

    auto t2 = f.get_tile(0, 1);
    //t2->type = HLC_TILE_IO;
    t2->name = "io_tile";
    t2->comments << " Tile comment";
    t2->enable("io_0/disable_input");
    t2->enable_edge("span4_right_g5_3", "local_g0_7", HLC_SW_BUFFER);
    t2->enable_edge("local_g0_7", "io_1/D_OUT_0", HLC_SW_BUFFER);
    auto c21 = t2->get_cell("io", 1);
    c21->enable("enable_input");

    std::stringstream o;
    f.print(o);
    std::string f1_actual = o.str();
    std::string f1_expected = ""
        "#File comment\n"
        "device \"\" 0 0\n"
        "\n"
        "warmboot = off\n"
        "\n"
        "io_tile 0 1 {\n"
        "    # Tile comment\n"
        "    span4_right_g5_3 -> local_g0_7\n"
        "\n"
        "    io_0 {\n"
        "        disable_input\n"
        "    }\n"
        "\n"
        "    io_1 {\n"
        //"        local_g0_7 -> D_OUT_0\n"
        "        local_g0_7 -> io_1/D_OUT_0\n"
        "        enable_input\n"
        "    }\n"
        "}\n"
        "\n"
        "logic_tile 0 0 {\n"
        "    # Edge comment 2\n"
        "    # Edge comment 3a\n"
        "    # Edge comment 3b\n"
        "    local_g0_5 -> span12_y7_g14_0 ~> span12_x2_g12_0\n"
        "    # Edge comment 4\n"
        "    span12_y7_g14_0 ~> span12_x3_g19_0\n"
        "\n"
        "    lutff_5 {\n"
        //"        out -> local_g0_5\n"
        "        # Edge comment 1\n"
        "        lutff_5/out -> local_g0_5\n"
        "        out = in_3 ^ (in_0 & in_1 & in_2)\n"
        "        # Enable the flip flop\n"
        "        enable_dff\n"
        "    }\n"
        "\n"
        "    lutff_6 {\n"
        "        local_g0_5 -> lutff_6/in_1\n"
        //"        local_g0_5 -> in_1\n"
        "    }\n"
        "}\n"
        "\n";
    REQUIRE(f1_actual == f1_expected);
}
