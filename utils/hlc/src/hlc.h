#ifndef HLC_H
#define HLC_H

#include <iostream>
#include <list>
#include <map>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

/* HLC Tile types */
typedef enum e_hlc_tile_type {
    HLC_TILE_NULL = 0,

    HLC_TILE_LOGIC,
    HLC_TILE_IO,
    HLC_TILE_RAM_TOP,
    HLC_TILE_RAM_BOTTOM,

    // End sentinel
    NUM_HLC_TILE_TYPES
} t_hlc_tile_type;

constexpr std::array<t_hlc_tile_type, NUM_HLC_TILE_TYPES> HLC_TILE_TYPES = { {
    HLC_TILE_NULL,
    HLC_TILE_LOGIC,
    HLC_TILE_IO,
    HLC_TILE_RAM_TOP,
    HLC_TILE_RAM_BOTTOM,
} };

constexpr std::array<const char*, NUM_HLC_TILE_TYPES> hlc_tile_typename = { {
    "null_tile", "logic_tile", "io_tile", "ramt_tile", "ramb_tile"
} };


/* HLC Switch types */
typedef enum e_hlc_sw_type {
    HLC_SW_NULL = 0,

    HLC_SW_SHORT,
    HLC_SW_BUFFER,
    HLC_SW_ROUTING,
    HLC_SW_END,

    // End sentinel
    NUM_HLC_SW_TYPES
} t_hlc_sw_type;

constexpr std::array<t_hlc_sw_type, NUM_HLC_SW_TYPES> HLC_SW_TYPES = { {
    HLC_SW_NULL,
    HLC_SW_SHORT,
    HLC_SW_BUFFER,
    HLC_SW_ROUTING,
    HLC_SW_END,
} };

constexpr std::array<const char*, NUM_HLC_SW_TYPES> hlc_sw_typenames = { {
    "!!!", "--", "->", "~>", ""
} };

/* HLC Coordinates */
struct t_hlc_coord {
    short x;
    short y;

    t_hlc_coord() : x(0), y(0) {}
    t_hlc_coord(int nx, int ny) : x(nx), y(ny) {}

    bool operator==(const t_hlc_coord &o) const { return (x == o.x) && (y == o.y); }
    bool operator!=(const t_hlc_coord &o) const { return !(*this == o); }
};

namespace std {
    template <>
    struct hash<t_hlc_coord> {
        std::size_t operator()(const t_hlc_coord& p) const noexcept {
            return ((std::size_t)(p.x) << 32) | p.y;
        }
    };
}

/* HLC Edge */
struct t_hlc_edge {
    std::string src;
    std::string dst;
    t_hlc_sw_type sw;
    std::stringstream comments;

    t_hlc_edge() : src(), dst(), sw(HLC_SW_NULL), comments() {}
    t_hlc_edge(const t_hlc_edge& o) : src(o.src), dst(o.dst), sw(o.sw), comments(o.comments.str()) {}
    t_hlc_edge(std::string nsrc, std::string ndst, t_hlc_sw_type nsw, std::string ncomments = "") : src(nsrc), dst(ndst), sw(nsw), comments(ncomments) {}

    bool operator==(const t_hlc_edge &o) const;
    bool operator!=(const t_hlc_edge &o) const;
    bool operator<(const t_hlc_edge &o) const;
};

namespace std {
    template <>
    struct hash<t_hlc_edge> {
        std::size_t operator()(const t_hlc_edge& e) const noexcept {
            std::size_t h1 = std::hash<std::string>{}(e.src);
            std::size_t h2 = std::hash<std::string>{}(e.dst);
            return h1 ^ (h2 << 1);
        }
    };
}

/* HLC Property */
struct t_hlc_property {
    std::string name;
    std::string value;
    std::stringstream comments;

    t_hlc_property() : name(), value(), comments() {}
    t_hlc_property(const t_hlc_property& o) : name(o.name), value(o.value), comments(o.comments.str()) {}
    t_hlc_property(std::string nname, std::string nvalue, std::string ncomments = "") : name(nname), value(nvalue), comments(ncomments) {}

    bool operator==(const t_hlc_property &o) const;
    bool operator!=(const t_hlc_property &o) const;
    bool operator<(const t_hlc_property &o) const;
};

namespace std {
    template <>
    struct hash<t_hlc_property> {
        std::size_t operator()(const t_hlc_property& c) const noexcept {
            std::size_t h1 = std::hash<std::string>{}(c.name);
            std::size_t h2 = std::hash<std::string>{}(c.value);
            return h1 ^ (h2 << 1);
        }
    };
}

/* HLC Cells */
struct t_hlc_cell_key {
    std::string name;
    std::size_t i;

    t_hlc_cell_key() : name(""), i(0) {}
    t_hlc_cell_key(std::string nname, std::size_t ni) : name(nname), i(ni) {}

    bool operator==(const t_hlc_cell_key &o) const;
    bool operator!=(const t_hlc_cell_key &o) const;
    bool operator<(const t_hlc_cell_key &o) const;
};

namespace std {
    template <>
    struct hash<t_hlc_cell_key> {
        std::size_t operator()(const t_hlc_cell_key& c) const noexcept {
            std::size_t h1 = std::hash<std::string>{}(c.name);
            std::size_t h2 = std::hash<std::size_t>{}(c.i);
            return h1 ^ (h2 << 1);
        }
    };
}

extern const t_hlc_cell_key global_key;

struct t_hlc_cell {
    t_hlc_cell_key k;
    std::stringstream comments;
    std::list<t_hlc_edge> edges;
    std::list<t_hlc_property> properties;

    t_hlc_cell(std::string fullname);
    t_hlc_cell(std::string name, std::size_t i);
    t_hlc_cell(t_hlc_cell_key k);
    t_hlc_cell(const t_hlc_cell &o);

    bool operator==(const t_hlc_cell &o) const;
    bool operator!=(const t_hlc_cell &o) const;
    bool operator<(const t_hlc_cell &o) const;

    inline std::ostream& enable(std::string key) { return enable(key, ""); }
    std::ostream& enable(std::string key, std::string value);
    std::ostream& enable_edge(std::string src, std::string dst, t_hlc_sw_type sw);
};

namespace std {
    template <>
    struct hash<t_hlc_cell> {
        std::size_t operator()(const t_hlc_cell& c) const noexcept {
            return std::hash<t_hlc_cell_key>{}(c.k);
        }
    };
}

constexpr std::array<const char*, NUM_HLC_TILE_TYPES> hlc_cell_prefix = { {
    "null", "lutff_", "io_"
} };

/* Tiles */
struct t_hlc_file;
struct t_hlc_tile {
    //t_hlc_tile_type type;
    std::string name;
    t_hlc_coord pos;

    std::stringstream comments;
    std::map<t_hlc_cell_key, t_hlc_cell*> cells;

    t_hlc_tile(int x, int y) : name(""), pos(x, y), comments(), cells() {}

    t_hlc_cell* get_cell(std::string fullname);
    t_hlc_cell* get_cell(std::string cellname, std::size_t i);
    t_hlc_cell* get_global_cell();

    inline std::ostream& enable(std::string key) { return enable(key, ""); }
    std::ostream& enable(std::string key, std::string value);
    std::ostream& enable_edge(std::string src, std::string dst, t_hlc_sw_type sw);

 private:
    t_hlc_cell* get_cell(t_hlc_cell_key k);

    friend t_hlc_file;
};

/** Whole file
 */
struct t_hlc_file {
    std::stringstream comments;
    std::string device;
    t_hlc_coord size;
    bool warmboot;
    std::unordered_map<t_hlc_coord, t_hlc_tile*> tiles;

    t_hlc_file() : comments(), device(""), size(0, 0), warmboot(false), tiles() {}

    t_hlc_tile* get_tile(t_hlc_coord p) {
        return get_tile(p.x, p.y);
    }

    t_hlc_tile* get_tile(int x, int y) {
        // VPR_ASSERT(x >= 0);
        // VPR_ASSERT(x < size.x);
        // VPR_ASSERT(y >= 0);
        // VPR_ASSERT(y < size.y);
        t_hlc_coord p(x, y);
        if (tiles.count(p) == 0) {
            tiles[p] = new t_hlc_tile(x, y);
        }
        return tiles[p];
    }

    void print(std::ostream& os);
private:
    void print_edges(std::ostream& os, std::string indent, std::list<t_hlc_edge>& edges);
    void print_properties(std::ostream& os, std::string indent, std::list<t_hlc_property>& properties);
};

/** */
t_hlc_cell_key parse_cell_name(std::string fullname);
std::pair<t_hlc_cell_key, std::string> parse_net_name(std::string fullname);

void print_indent(std::ostream& os, std::string indent, std::string s);



#endif  // HLC_H
