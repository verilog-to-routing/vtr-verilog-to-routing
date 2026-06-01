#include "device_grid.h"

#include <cstddef>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
#include "physical_types.h"
#include "vtr_expr_eval.h"
#include "vtr_ndmatrix.h"
#include "grid_util.h"
#include "vpr_error.h"

/**
 * @brief checks the horizontal_interposer_cuts_ and vertical_interposer_cuts_ fields of the DeviceGrid object
 * to see if it has any interposer cuts. This function is used in the constructor of the DeviceGrid to cache the result.
 */
static bool check_if_grid_has_interposer_cuts(const DeviceGrid& grid);

DeviceGrid::DeviceGrid(const t_grid_def& grid_def,
                       vtr::NdMatrix<t_grid_tile, 3> grid)
    : name_(grid_def.name)
    , grid_(std::move(grid)) {
    vtr::FormulaParser p;
    std::tie(horizontal_interposer_cuts_, vertical_interposer_cuts_) = resolve_interposer_cut_locations(*this, grid_def, p);
    has_interposer_cuts_ = check_if_grid_has_interposer_cuts(*this);
    count_instances();
    initialize_multi_die_data_structures();
}

DeviceGrid::DeviceGrid(const t_grid_def& grid_def,
                       vtr::NdMatrix<t_grid_tile, 3> grid,
                       std::vector<t_logical_block_type_ptr> limiting_res)
    : DeviceGrid(grid_def, std::move(grid)) {
    limiting_resources_ = std::move(limiting_res);
}

size_t DeviceGrid::num_instances(t_physical_tile_type_ptr type, int layer_num) const {
    size_t count = 0;
    //instance_counts_ is not initialized
    if (instance_counts_.empty()) {
        return 0;
    }

    int num_layers = (int)grid_.dim_size(0);

    if (layer_num == -1) {
        //Count all layers
        for (int curr_layer_num = 0; curr_layer_num < num_layers; ++curr_layer_num) {
            auto iter = instance_counts_[curr_layer_num].find(type);
            if (iter != instance_counts_[curr_layer_num].end()) {
                count += iter->second;
            }
        }
        return count;
    } else {
        auto iter = instance_counts_[layer_num].find(type);
        if (iter != instance_counts_[layer_num].end()) {
            //Return count
            count = iter->second;
        }
    }

    return count;
}

void DeviceGrid::clear() {
    grid_.clear();
    instance_counts_.clear();
}

void DeviceGrid::count_instances() {
    int num_layers = (int)grid_.dim_size(0);
    instance_counts_.clear();
    instance_counts_.resize(num_layers);

    //Initialize the instance counts
    for (int layer_num = 0; layer_num < num_layers; ++layer_num) {
        for (size_t x = 0; x < width(); ++x) {
            for (size_t y = 0; y < height(); ++y) {
                auto type = grid_[layer_num][x][y].type;

                if (grid_[layer_num][x][y].width_offset == 0 && grid_[layer_num][x][y].height_offset == 0) {
                    //Add capacity only if this is the root location
                    instance_counts_[layer_num][type] = 0;
                }
            }
        }
    }

    //Count the number of blocks in the grid
    for (int layer_num = 0; layer_num < num_layers; ++layer_num) {
        for (size_t x = 0; x < width(); ++x) {
            for (size_t y = 0; y < height(); ++y) {
                auto type = grid_[layer_num][x][y].type;

                if (grid_[layer_num][x][y].width_offset == 0 && grid_[layer_num][x][y].height_offset == 0) {
                    //Add capacity only if this is the root location
                    instance_counts_[layer_num][type] += type->capacity;
                }
            }
        }
    }
}

void DeviceGrid::initialize_multi_die_data_structures() {
    const size_t num_layers = grid_.dim_size(0);
    const size_t device_width = grid_.dim_size(1);
    const size_t device_height = grid_.dim_size(2);

    // Build the unique Ids for each die. In 2D architectures there's only a single die but this is not the case in 2.5D and 3D architectures.
    short die_region_counter = 0;
    for (size_t layer = 0; layer < num_layers; layer++) {
        const std::vector<int>& horizontal_interposers = horizontal_interposer_cuts_[layer];
        const std::vector<int>& vertical_interposers = vertical_interposer_cuts_[layer];

        vtr::NdMatrix<DeviceDieId, 2> layer_reduced_die_id_matrix({vertical_interposers.size() + 1, horizontal_interposers.size() + 1});

        for (size_t i = 0; i < vertical_interposers.size() + 1; i++) {
            for (size_t j = 0; j < horizontal_interposers.size() + 1; j++) {
                DeviceDieId die_id = (DeviceDieId)die_region_counter;
                layer_reduced_die_id_matrix[i][j] = die_id;

                t_die_region region = {.x_die = static_cast<short>(i), .y_die = static_cast<short>(j), .layer = static_cast<short>(layer)};
                die_region_map_.update(die_id, region);

                die_region_counter++;
            }
        }
        vtr::NdMatrix<DeviceDieId, 2> layer_die_id_matrix = get_device_sized_matrix_from_reduced(device_width,
                                                                                                 device_height,
                                                                                                 horizontal_interposers,
                                                                                                 vertical_interposers,
                                                                                                 layer_reduced_die_id_matrix);
        die_id_matrix_.push_back(std::move(layer_die_id_matrix));
    }
}

static bool check_if_grid_has_interposer_cuts(const DeviceGrid& grid) {
    for (const std::vector<int>& layer_h_cuts : grid.get_horizontal_interposer_cuts()) {
        if (!layer_h_cuts.empty()) {
            return true;
        }
    }

    for (const std::vector<int>& layer_v_cuts : grid.get_vertical_interposer_cuts()) {
        if (!layer_v_cuts.empty()) {
            return true;
        }
    }

    return false;
}

bool DeviceGrid::has_interposer_cuts() const {
    return has_interposer_cuts_;
}

bool DeviceGrid::are_locs_on_same_die(t_physical_tile_loc loc_a, t_physical_tile_loc loc_b) const {
    const DeviceDieId first_id = die_id_matrix_[loc_a.layer_num][loc_a.x][loc_a.y];
    const DeviceDieId second_id = die_id_matrix_[loc_b.layer_num][loc_b.x][loc_b.y];

    return first_id == second_id;
}

DeviceDieId DeviceGrid::get_loc_die_id(t_physical_tile_loc loc) const {
    return die_id_matrix_[loc.layer_num][loc.x][loc.y];
}

DeviceDieId DeviceGrid::get_die_region_id(t_die_region die_region) const {
    return die_region_map_[die_region];
}

size_t DeviceGrid::get_die_count() const {
    size_t die_count = 0;
    for (size_t layer_num = 0; layer_num < get_num_layers(); layer_num++) {
        die_count += (get_vertical_interposer_cuts()[layer_num].size() + 1) * (get_horizontal_interposer_cuts()[layer_num].size() + 1);
    }

    return die_count;
}

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

/// Strip leading and trailing whitespace from a string in place.
void trim(std::string& str) {
    const auto start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        str.clear();
        return;
    }
    const auto end = str.find_last_not_of(" \t\r\n");
    str = str.substr(start, end - start + 1);
}

// Report a fatal .grid parse error; line_num is 1-based, or 0 if the error is not tied to a line.
[[noreturn]] void grid_file_fatal_error(const std::string& filepath, int line_num, const std::string& msg) {
    std::string full_msg = "Error parsing grid file '" + filepath + "'";
    if (line_num > 0) {
        full_msg += " at line " + std::to_string(line_num);
    }
    full_msg += ": " + msg;
    VPR_FATAL_ERROR(VPR_ERROR_ARCH, "%s", full_msg.c_str());
}

/// Parse a comma-separated list of integers (e.g. interposer cut locations).
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

/// Parse a "key: value" header line; returns nullopt if the line does not start with key.
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

} // end of anonymous namespace grid_file_parsing

void DeviceGrid::write_grid_file(const std::string& filepath) const {
    std::ofstream out{filepath};
    if (!out) {
        VPR_FATAL_ERROR(VPR_ERROR_ARCH, "Failed to open grid file '%s' for writing", filepath.c_str());
    }

    out << "# VTR DeviceGrid\n";
    out << "name: " << name_ << "\n";
    out << "layers: " << get_num_layers() << "\n";
    out << "width: " << width() << "\n";
    out << "height: " << height() << "\n";

    for (size_t layer = 0; layer < get_num_layers(); ++layer) {
        out << "interposer_cuts layer=" << layer << " horz:";
        const std::vector<int>& horz_cuts = horizontal_interposer_cuts_[layer];
        for (size_t i = 0; i < horz_cuts.size(); ++i) {
            if (i == 0) {
                out << " ";
            } else {
                out << ", ";
            }
            out << horz_cuts[i];
        }
        out << " vert:";
        const std::vector<int>& vert_cuts = vertical_interposer_cuts_[layer];
        for (size_t i = 0; i < vert_cuts.size(); ++i) {
            if (i == 0) {
                out << " ";
            } else {
                out << ", ";
            }
            out << vert_cuts[i];
        }
        out << "\n";
    }

    for (size_t layer = 0; layer < get_num_layers(); ++layer) {
        out << "tiles layer=" << layer << "\n";
        for (size_t x = 0; x < width(); ++x) {
            for (size_t y = 0; y < height(); ++y) {
                const t_grid_tile& tile = grid_[layer][x][y];
                out << "  " << x << " " << y << " " << tile.type->name << " "
                    << tile.width_offset << " " << tile.height_offset << "\n";
            }
        }
    }
}

DeviceGrid::DeviceGrid(const std::string& grid_filepath,
                       const std::vector<t_physical_tile_type_ptr>& physical_tile_types) {
    std::unordered_map<std::string, t_physical_tile_type_ptr> tile_type_map;
    for (t_physical_tile_type_ptr tile_type : physical_tile_types) {
        tile_type_map[tile_type->name] = tile_type;
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
    vtr::NdMatrix<t_grid_tile, 3> grid;

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

        if (auto value = parse_key_value(line, "name")) {
            name = *value;
            have_name = true;
        } else if (auto value = parse_key_value(line, "layers")) {
            num_layers = std::stoull(*value);
            have_layers = true;
        } else if (auto value = parse_key_value(line, "width")) {
            grid_width = std::stoull(*value);
            have_width = true;
        } else if (auto value = parse_key_value(line, "height")) {
            grid_height = std::stoull(*value);
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
            if (grid.empty()) {
                grid = vtr::NdMatrix<t_grid_tile, 3>({num_layers, grid_width, grid_height});
                tile_seen = vtr::NdMatrix<bool, 3>({num_layers, grid_width, grid_height}, false);
                if (horizontal_cuts.empty()) {
                    horizontal_cuts.resize(num_layers);
                    vertical_cuts.resize(num_layers);
                }
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
            grid[current_tile_layer][x][y].type = tile_type_iter->second;
            grid[current_tile_layer][x][y].width_offset = width_offset;
            grid[current_tile_layer][x][y].height_offset = height_offset;
        }
    }

    if (!have_name || !have_layers || !have_width || !have_height) {
        grid_file_fatal_error(grid_filepath, 0, "Missing required header fields");
    }
    if (grid.empty()) {
        grid_file_fatal_error(grid_filepath, 0, "Missing tile data");
    }

    for (size_t layer = 0; layer < num_layers; ++layer) {
        for (size_t x = 0; x < grid_width; ++x) {
            for (size_t y = 0; y < grid_height; ++y) {
                if (!tile_seen[layer][x][y]) {
                    grid_file_fatal_error(grid_filepath, 0, "Missing tile entry for layer " + std::to_string(layer) + " at (" + std::to_string(x) + ", " + std::to_string(y) + ")");
                }
            }
        }
    }

    // TODO: Validate that the grid is valid.

    name_ = name;
    grid_ = std::move(grid);
    horizontal_interposer_cuts_ = std::move(horizontal_cuts);
    vertical_interposer_cuts_ = std::move(vertical_cuts);
    has_interposer_cuts_ = check_if_grid_has_interposer_cuts(*this);
    count_instances();
    initialize_multi_die_data_structures();
}
