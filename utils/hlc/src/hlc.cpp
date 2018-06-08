
#include <algorithm>    // std::find

#include "hlc.h"

/* HLC Edges */
bool t_hlc_edge::operator==(const t_hlc_edge &o) const {
    if (dst != o.dst) {
        return false;
    } else if (src != o.src) {
        return false;
    } else if (sw != o.sw) {
        return false;
    }
    return true;
}

bool t_hlc_edge::operator!=(const t_hlc_edge &o) const {
    return !(*this == o);
}

bool t_hlc_edge::operator<(const t_hlc_edge &o) const {
    if (dst != o.dst) {
        return dst < o.dst;
    } else if (src != o.src) {
        return src < o.src;
    } else {
        return sw < o.sw;
    }
}

/* HLC Cell Key */
bool t_hlc_property::operator==(const t_hlc_property &o) const {
    if (name != o.name) {
        return false;
    } else if (value != o.value) {
        return false;
    }
    return true;
}

bool t_hlc_property::operator!=(const t_hlc_property &o) const {
    return !(*this == o);
}

bool t_hlc_property::operator<(const t_hlc_property &o) const {
    if (name != o.name) {
        return name < o.name;
    } else {
        return value < o.value;
    }
}

/* HLC Cell Key */
const t_hlc_cell_key global_key("global", 0);

t_hlc_cell_key parse_cell_name(std::string fullname) {
    t_hlc_cell_key k(global_key);

    std::string::size_type n = fullname.find("_");
    if (n != std::string::npos) {
        try {
            k.i = std::stoi(fullname.substr(n+1));
            k.name = fullname.substr(0, n);
        } catch (const std::exception&) {}
    }
    return k;
}

bool t_hlc_cell_key::operator==(const t_hlc_cell_key &o) const {
    if (name != o.name) {
        return false;
    } else if (i != o.i) {
        return false;
    }
    return true;
}

bool t_hlc_cell_key::operator!=(const t_hlc_cell_key &o) const {
    return !(*this == o);
}

bool t_hlc_cell_key::operator<(const t_hlc_cell_key &o) const {
    if (name != o.name) {
        return name < o.name;
    } else {
        return i < o.i;
    }
}

/* HLC Cell */
t_hlc_cell::t_hlc_cell(std::string fullname) : k(), comments(), edges(), properties() {
    k = parse_cell_name(fullname);
}

t_hlc_cell::t_hlc_cell(std::string nname, std::size_t ni) : k(nname, ni), comments(), edges(), properties() {}

t_hlc_cell::t_hlc_cell(t_hlc_cell_key nk) : k(nk), comments(), edges(), properties() {}
t_hlc_cell::t_hlc_cell(const t_hlc_cell& o) : k(o.k), comments(o.comments.str()), edges(o.edges), properties(o.properties) {}

bool t_hlc_cell::operator==(const t_hlc_cell &o) const {
    return k == o.k;
}

bool t_hlc_cell::operator!=(const t_hlc_cell &o) const {
    return !(*this == o);
}

bool t_hlc_cell::operator<(const t_hlc_cell &o) const {
    return k < o.k;
}

std::ostream& t_hlc_cell::enable(std::string key, std::string value) {
    t_hlc_property new_property(key, value);
    auto found = std::find(properties.begin(), properties.end(), new_property);
    if (found == properties.end()) {
        properties.push_back(new_property);
        return properties.back().comments;
    } else {
        return found->comments;
    }
}

std::ostream& t_hlc_cell::enable_edge(std::string src, std::string dst, t_hlc_sw_type sw) {
    t_hlc_edge new_edge(/* psrc.second */ src, /* pdst.second */ dst, sw);
    std::cout << k.name << "_" << k.i << " (" << new_edge.src << "->" << new_edge.dst << ") ";
    auto found = std::find(edges.begin(), edges.end(), new_edge);
    if (found == edges.end()) {
        std::cout << "NEW" << std::endl;
        edges.push_back(new_edge);
        return edges.back().comments;
    } else {
        std::cout << "OLD" << std::endl;
        return found->comments;
    }
}

/* HLC Tile */
std::pair<t_hlc_cell_key, std::string> parse_net_name(std::string fullname) {
    std::string::size_type n = fullname.find("/");
    if (n == std::string::npos) {
        return std::make_pair(t_hlc_cell_key(global_key), fullname);
    }
    return std::make_pair(parse_cell_name(fullname.substr(0, n)), fullname.substr(n+1));
}

t_hlc_cell* t_hlc_tile::get_cell(std::string fullname) {
    return get_cell(parse_cell_name(fullname));
}

t_hlc_cell* t_hlc_tile::get_cell(std::string cellname, std::size_t i) {
    return get_cell(t_hlc_cell_key(cellname, i));
}

t_hlc_cell* t_hlc_tile::get_cell(t_hlc_cell_key k) {
    if (cells.count(k) == 0) {
        cells[k] = new t_hlc_cell(k);
    }
    return cells[k];
}

t_hlc_cell* t_hlc_tile::get_global_cell() {
    return get_cell(global_key);
}

std::ostream& t_hlc_tile::enable(std::string key, std::string value) {
    auto pkey = parse_net_name(key);
    auto cell = get_cell(pkey.first);
    return cell->enable(pkey.second, value);
}

std::ostream& t_hlc_tile::enable_edge(std::string src, std::string dst, t_hlc_sw_type sw) {
    auto psrc = parse_net_name(src);
    auto pdst = parse_net_name(dst);

    std::cout << name << " (" << pos.x << "," << pos.y << ") enable_edge " << src << "->" << dst << std::endl;

    auto cell = get_cell(psrc.first);
    if (cell->k == global_key) {
        cell = get_cell(pdst.first);
    }

    // FIXME: HLC parser barfs
    // -- https://github.com/cliffordwolf/icestorm/issues/145
    return cell->enable_edge(/* psrc.second */ src, /* pdst.second */ dst, sw);
}

void print_indent(std::ostream& os, std::string indent, std::string str) {
    std::string::size_type s = 0;
    std::string::size_type e = str.size();

    while (s < str.size()) {
        std::string::size_type n = str.find('\n', s);
        if (n != std::string::npos) {
            e = n+1;
        } else {
            e = str.size();
        }
        os << indent << str.substr(s, e-s);
        s = e;
    }
}

void t_hlc_file::print_edges(std::ostream& os, std::string indent, std::list<t_hlc_edge>& edges) {
    std::vector<std::vector<t_hlc_edge>> groups;

    std::string last_dst;
    for (auto e : edges) {
        if (last_dst != e.src) {
            groups.push_back(std::vector<t_hlc_edge>());
        }
        groups.back().push_back(e);
        last_dst = e.dst;
    }

    for (auto g : groups) {
        for (auto e : g) {
            if (e.comments.str().size() > 0) {
                print_indent(os, indent+"#", e.comments.str());
                os << std::endl;
            }
        }

        os << indent;
        os << g.front().src;
        for (auto e : g) {
            os << " " << hlc_sw_typenames[e.sw];
            os << " " << e.dst;
        }
        os << std::endl;
    }
}

void t_hlc_file::print_properties(std::ostream& os, std::string indent, std::list<t_hlc_property>& properties) {
    for (auto p : properties) {
        if (p.comments.str().size() > 0) {
            print_indent(os, indent+"#", p.comments.str());
            os << std::endl;
        }

        os << indent;
        os << p.name;
        if (p.value.size() > 0) {
            os << " = " << p.value;
        }
        os << std::endl;
    }
}

void t_hlc_file::print(std::ostream& os) {
    if (comments.str().size() > 0) {
        print_indent(os, "#", comments.str());
        os << std::endl;
    }
    os << "device \"" << device << "\" " << size.x << " " << size.y << std::endl;
    os << std::endl;
    os << "warmboot = " << (warmboot ? std::string("on") : std::string("off")) << std::endl;
    os << std::endl;

    for (auto i : tiles) {
        auto pos = i.first;
        auto tile = i.second;
        // VTR_ASSERT(pos.x == tile->pos.x);
        // VTR_ASSERT(pos.y == tile->pos.y);
        os << tile->name // hlc_tile_typename[tile->type]
            << " " << tile->pos.x << " " << tile->pos.y
            << " {" << std::endl;
        if (tile->comments.str().size() > 0) {
            print_indent(os, "    #", tile->comments.str());
            os << std::endl;
        }

        auto r = tile->get_cell(global_key);
        print_edges(os, std::string("    "), r->edges);
        print_properties(os, std::string("    "), r->properties);

        for (auto jt = tile->cells.begin(); jt != tile->cells.end(); ++jt) {
            auto c = jt->second;
            if (c->k == global_key) {
                continue;
            }
            os << std::endl;
            os << "    " << c->k.name << "_" << c->k.i << " {" << std::endl;
            if (c->comments.str().size() > 0) {
                print_indent(os, "        #", c->comments.str());
                os << std::endl;
            }
            print_edges(os, std::string("        "), c->edges);
            print_properties(os, std::string("        "), c->properties);
            os << "    }" << std::endl;
        }

        os << "}" << std::endl;
        os << std::endl;
    }
}
