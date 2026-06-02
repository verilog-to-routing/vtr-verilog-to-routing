#include "read_grid_file.h"

#include <cstddef>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "grid_types.h"
#include "interposer_types.h"
#include "physical_types.h"
#include "vpr_error.h"
#include "vtr_ndmatrix.h"

// VTR DeviceGrid .grid file format
//
// Human-readable text file describing the FPGA device layout.
// Lines starting with '#' are comments. Blank lines are ignored.
//
// Header (required, any order before tile/cut sections):
//   name: <grid_name>
//   layers: <num_layers>
//   width: <grid_width>
//   height: <grid_height>
//
// Interposer cuts (one line per layer; cut locations are integers):
//   interposer_cuts layer=<n> horz: <y0>, <y1>, ... vert: <x0>, <x1>, ...
// Empty cut lists omit values after the colon, e.g. "horz: vert:".
// No spaces are allowed before or after '=' in layer=<n>.
//
// Tiles (one section per layer; every grid location must appear exactly once):
//   tiles layer=<n>
//     <x> <y> <tile_type_name> <width_offset> <height_offset>
//
// Tile metadata is not supported in this format.

namespace { // grid_file_parsing

/// Remove '#' and everything after it from a .grid file line.
void strip_comment(std::string& line) {
    const auto comment_pos = line.find('#');
    if (comment_pos != std::string::npos) {
        line.erase(comment_pos);
    }
}

/// @brief Strip leading and trailing whitespace from a string in place.
void trim(std::string& str) {
    const auto start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        str.clear();
        return;
    }
    const auto end = str.find_last_not_of(" \t\r\n");
    str = str.substr(start, end - start + 1);
}

/// @brief Report a fatal .grid parse error; line_num is 1-based, or 0 if the error is not tied to a line.
[[noreturn]] void grid_file_fatal_error(const std::string& filepath, int line_num, const std::string& msg) {
    std::string full_msg = "Error parsing grid file '" + filepath + "'";
    if (line_num > 0) {
        full_msg += " at line " + std::to_string(line_num);
    }
    full_msg += ": " + msg;
    VPR_FATAL_ERROR(VPR_ERROR_ARCH, "%s", full_msg.c_str());
}

/// @brief Parse a comma-separated list of integers (e.g. interposer cut locations).
std::vector<int> parse_cut_list(const std::string& list_str) {
    std::vector<int> cuts;
    std::stringstream ss(list_str);
    std::string token;
    while (std::getline(ss, token, ',')) {
        trim(token);
        if (token.empty()) {
            continue;
        }
        cuts.push_back(std::stoi(token));
    }
    return cuts;
}

/// @brief Parse a "key: value" header line; returns nullopt if the line does not start with key.
std::optional<std::string> parse_key_value(const std::string& line, const std::string& key) {
    const std::string prefix = key + ":";
    // rfind(prefix, 0) matches only at index 0. Checks that line starts with prefix.
    if (line.rfind(prefix, 0) != 0) {
        return std::nullopt;
    }
    // Take everything after "key:" as the value.
    std::string value = line.substr(prefix.size());
    trim(value);
    return value;
}

/// @brief Parse the layer index from a directive line such as "tiles layer=0" or "interposer_cuts layer=1 ...".
/// @param line The trimmed .grid file line to parse.
/// @param directive The directive name without the layer suffix (e.g. "tiles", "interposer_cuts").
/// @param filepath Path to the .grid file, included in fatal error messages.
/// @param line_num Line number in the file, included in fatal error messages.
/// @return The parsed layer index.
int parse_layer_from_directive(const std::string& line, const std::string& directive, const std::string& filepath, int line_num) {
    const std::string prefix = directive + " layer=";
    if (line.rfind(prefix, 0) != 0) {
        grid_file_fatal_error(filepath, line_num, "Expected '" + directive + " layer=<n>' directive");
    }

    const std::string rest = line.substr(prefix.size());
    const auto layer_end = rest.find(' ');
    std::string layer_str = layer_end == std::string::npos ? rest : rest.substr(0, layer_end);
    trim(layer_str);
    if (layer_str.empty()) {
        grid_file_fatal_error(filepath, line_num, "Missing layer number in '" + directive + "' directive");
    }

    return std::stoi(layer_str);
}

/// @brief Parse an "interposer_cuts layer=<n> horz: ... vert: ..." line into per-layer cut lists.
/// @param line The trimmed .grid file line to parse.
/// @param num_layers Number of layers in the grid, used to validate the layer index.
/// @param horizontal_cuts Per-layer horizontal cut lists to update for the parsed layer.
/// @param vertical_cuts Per-layer vertical cut lists to update for the parsed layer.
/// @param filepath Path to the .grid file, included in fatal error messages.
/// @param line_num Line number in the file, included in fatal error messages.
void parse_interposer_cuts_line(const std::string& line,
                                size_t num_layers,
                                std::vector<std::vector<int>>& horizontal_cuts,
                                std::vector<std::vector<int>>& vertical_cuts,
                                const std::string& filepath,
                                int line_num) {
    // Labels in interposer_cuts lines; value parsing starts after the label.
    constexpr std::string_view interposer_horz_label = "horz:";
    constexpr std::string_view interposer_vert_label = "vert:";

    const int layer = parse_layer_from_directive(line, "interposer_cuts", filepath, line_num);
    if (layer < 0 || static_cast<size_t>(layer) >= num_layers) {
        grid_file_fatal_error(filepath, line_num, "Layer index out of range");
    }

    // Both horz: and vert: must be present; either list may be empty.
    const auto horz_pos = line.find(interposer_horz_label);
    const auto vert_pos = line.find(interposer_vert_label);
    if (horz_pos == std::string::npos || vert_pos == std::string::npos) {
        grid_file_fatal_error(filepath, line_num, "Missing 'horz:' or 'vert:' in interposer_cuts directive");
    }

    const size_t horz_value_pos = horz_pos + interposer_horz_label.size();
    std::string horz_str;
    // Extract the horz cut list: if vert: follows, stop before it; otherwise take the rest of the line.
    if (horz_pos < vert_pos) {
        horz_str = line.substr(horz_value_pos, vert_pos - horz_value_pos);
    } else {
        horz_str = line.substr(horz_value_pos);
    }
    trim(horz_str);

    std::string vert_str = line.substr(vert_pos + interposer_vert_label.size());
    trim(vert_str);

    horizontal_cuts[layer] = parse_cut_list(horz_str);
    vertical_cuts[layer] = parse_cut_list(vert_str);
}

/// @brief Append a t_grid_loc_def for a root tile parsed from a .grid file.
/// @param layer_def Layer definition to receive the new location specification.
/// @param x_root Root x coordinate of the tile (width_offset == 0).
/// @param y_root Root y coordinate of the tile (height_offset == 0).
/// @param tile_type Physical tile type at the root location.
/// @param grid_width Device grid width, used as the x repeat expression so only one instance is placed.
/// @param grid_height Device grid height, used as the y repeat expression so only one instance is placed.
void add_root_tile_loc_def(t_layer_def& layer_def,
                           int x_root,
                           int y_root,
                           t_physical_tile_type_ptr tile_type,
                           size_t grid_width,
                           size_t grid_height) {
    const int w = tile_type->width;
    const int h = tile_type->height;

    t_grid_loc_def loc_def(tile_type->name, /*priority_val=*/1);
    loc_def.x.start_expr = std::to_string(x_root);
    loc_def.y.start_expr = std::to_string(y_root);
    loc_def.x.end_expr = std::to_string(x_root + w - 1);
    loc_def.y.end_expr = std::to_string(y_root + h - 1);
    // Use full grid dimensions for repeat so each loc_def places exactly one tile.
    loc_def.x.repeat_expr = std::to_string(grid_width);
    loc_def.y.repeat_expr = std::to_string(grid_height);
    loc_def.x.incr_expr = std::to_string(w);
    loc_def.y.incr_expr = std::to_string(h);
    layer_def.loc_defs.push_back(std::move(loc_def));
}

void populate_interposer_cuts(t_layer_def& layer_def,
                            const std::vector<int>& horizontal_cuts,
                            const std::vector<int>& vertical_cuts) {
    for (int cut : horizontal_cuts) {
        t_interposer_cut_inf cut_inf;
        cut_inf.dim = e_interposer_cut_type::HORZ;
        cut_inf.loc = std::to_string(cut);
        layer_def.interposer_cuts.push_back(cut_inf);
    }
    for (int cut : vertical_cuts) {
        t_interposer_cut_inf cut_inf;
        cut_inf.dim = e_interposer_cut_type::VERT;
        cut_inf.loc = std::to_string(cut);
        layer_def.interposer_cuts.push_back(cut_inf);
    }
}

} // end of anonymous namespace grid_file_parsing

t_grid_def read_grid_file(const std::string& grid_filepath,
                          const std::vector<t_physical_tile_type>& physical_tile_types) {
    std::unordered_map<std::string, t_physical_tile_type_ptr> tile_type_map;
    for (const t_physical_tile_type& tile_type : physical_tile_types) {
        tile_type_map[tile_type.name] = &tile_type;
    }

    std::ifstream in(grid_filepath);
    if (!in) {
        VPR_FATAL_ERROR(VPR_ERROR_ARCH, "Failed to open grid file '%s' for reading", grid_filepath.c_str());
    }

    std::string name;
    size_t num_layers = 0;
    size_t grid_width = 0;
    size_t grid_height = 0;
    bool have_name = false;
    bool have_layers = false;
    bool have_width = false;
    bool have_height = false;

    std::vector<std::vector<int>> horizontal_cuts;
    std::vector<std::vector<int>> vertical_cuts;

    t_grid_def grid_def;
    bool have_tiles = false;

    // Tracks which (layer, x, y) locations have been read from the file; indexed as [layer][x][y].
    vtr::NdMatrix<bool, 3> tile_seen;

    std::string line;
    int line_num = 0;
    int current_tile_layer = -1;

    while (std::getline(in, line)) {
        ++line_num;
        strip_comment(line);
        trim(line);
        if (line.empty()) {
            continue;
        }

        if (auto name_value = parse_key_value(line, "name")) {
            name = *name_value;
            have_name = true;
        } else if (auto layers_value = parse_key_value(line, "layers")) {
            num_layers = std::stoull(*layers_value);
            have_layers = true;
        } else if (auto width_value = parse_key_value(line, "width")) {
            grid_width = std::stoull(*width_value);
            have_width = true;
        } else if (auto height_value = parse_key_value(line, "height")) {
            grid_height = std::stoull(*height_value);
            have_height = true;
        } else if (line.rfind("interposer_cuts layer=", 0) == 0) {
            if (!have_layers || !have_width || !have_height) {
                grid_file_fatal_error(grid_filepath, line_num, "Header must specify layers, width, and height before interposer_cuts");
            }
            if (horizontal_cuts.empty()) {
                horizontal_cuts.resize(num_layers);
                vertical_cuts.resize(num_layers);
            }
            parse_interposer_cuts_line(line, num_layers, horizontal_cuts, vertical_cuts, grid_filepath, line_num);
        } else if (line.rfind("tiles layer=", 0) == 0) {
            if (!have_name || !have_layers || !have_width || !have_height) {
                grid_file_fatal_error(grid_filepath, line_num, "Header must specify name, layers, width, and height before tiles");
            }
            if (!have_tiles) {
                grid_def.layers.resize(num_layers);
                tile_seen = vtr::NdMatrix<bool, 3>({num_layers, grid_width, grid_height}, false);
                if (horizontal_cuts.empty()) {
                    horizontal_cuts.resize(num_layers);
                    vertical_cuts.resize(num_layers);
                }
                have_tiles = true;
            }
            current_tile_layer = parse_layer_from_directive(line, "tiles", grid_filepath, line_num);
            if (current_tile_layer < 0 || static_cast<size_t>(current_tile_layer) >= num_layers) {
                grid_file_fatal_error(grid_filepath, line_num, "Layer index out of range");
            }
        } else {
            if (current_tile_layer < 0) {
                grid_file_fatal_error(grid_filepath, line_num, "Unexpected line before tiles section");
            }

            std::istringstream tile_line(line);
            int x = 0;
            int y = 0;
            std::string tile_name;
            int width_offset = 0;
            int height_offset = 0;
            if (!(tile_line >> x >> y >> tile_name >> width_offset >> height_offset)) {
                grid_file_fatal_error(grid_filepath, line_num, "Malformed tile entry");
            }

            if (x < 0 || y < 0 || static_cast<size_t>(x) >= grid_width || static_cast<size_t>(y) >= grid_height) {
                grid_file_fatal_error(grid_filepath, line_num, "Tile coordinates out of range");
            }

            if (tile_seen[current_tile_layer][x][y]) {
                grid_file_fatal_error(grid_filepath, line_num, "Duplicate tile entry");
            }
            tile_seen[current_tile_layer][x][y] = true;

            const auto tile_type_iter = tile_type_map.find(tile_name);
            if (tile_type_iter == tile_type_map.end()) {
                grid_file_fatal_error(grid_filepath, line_num, "Unknown tile type '" + tile_name + "'");
            }

            if (width_offset == 0 && height_offset == 0) {
                add_root_tile_loc_def(grid_def.layers[current_tile_layer],
                                      x,
                                      y,
                                      tile_type_iter->second,
                                      grid_width,
                                      grid_height);
            }
        }
    }

    if (!have_name || !have_layers || !have_width || !have_height) {
        grid_file_fatal_error(grid_filepath, 0, "Missing required header fields");
    }
    if (!have_tiles) {
        grid_file_fatal_error(grid_filepath, 0, "Missing tile data");
    }

    for (size_t layer = 0; layer < num_layers; ++layer) {
        for (size_t x = 0; x < grid_width; ++x) {
            for (size_t y = 0; y < grid_height; ++y) {
                if (!tile_seen[layer][x][y]) {
                    const std::string msg = "Missing tile entry for layer " + std::to_string(layer)
                                            + " at (" + std::to_string(x) + ", " + std::to_string(y) + ")";
                    grid_file_fatal_error(grid_filepath, 0, msg);
                }
            }
        }
    }

    grid_def.grid_type = e_grid_def_type::FIXED;
    grid_def.name = name;
    grid_def.width = static_cast<int>(grid_width);
    grid_def.height = static_cast<int>(grid_height);
    grid_def.aspect_ratio = float(grid_width) / float(grid_height);

    for (size_t layer = 0; layer < num_layers; ++layer) {
        populate_interposer_cuts(grid_def.layers[layer], horizontal_cuts[layer], vertical_cuts[layer]);
    }

    return grid_def;
}
