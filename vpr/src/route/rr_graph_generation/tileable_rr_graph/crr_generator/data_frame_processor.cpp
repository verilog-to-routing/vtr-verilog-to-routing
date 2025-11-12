#include "data_frame_processor.h"

#include "vtr_log.h"

#include <filesystem>
#include <algorithm>

namespace crrgenerator {

// DataFrame implementation

DataFrame::DataFrame(size_t rows, size_t cols)
    : rows_(rows)
    , cols_(cols) {
    data_.resize(rows);
    for (auto& row : data_) {
        row.resize(cols);
    }
}

Cell& DataFrame::at(size_t row, size_t col) {
    validate_bounds(row, col);
    return data_[row][col];
}

const Cell& DataFrame::at(size_t row, size_t col) const {
    validate_bounds(row, col);
    return data_[row][col];
}

void DataFrame::resize(size_t rows, size_t cols) {
    rows_ = rows;
    cols_ = cols;
    data_.resize(rows);
    for (auto& row : data_) {
        row.resize(cols);
    }
}

void DataFrame::clear() {
    data_.clear();
    rows_ = 0;
    cols_ = 0;
    source_file.clear();
    connections = 0;
}

std::vector<Cell> DataFrame::get_row(size_t row) const {
    if (row >= rows_) {
        VTR_LOG_ERROR("Row index out of range: %zu - max %zu", row, rows_);
    }
    return data_[row];
}

std::vector<Cell> DataFrame::get_column(size_t col) const {
    if (col >= cols_) {
        VTR_LOG_ERROR("Column index out of range: %zu - max %zu", col, cols_);
    }

    std::vector<Cell> column;
    column.reserve(rows_);
    for (size_t row = 0; row < rows_; ++row) {
        column.push_back(data_[row][col]);
    }
    return column;
}

void DataFrame::set_row(size_t row, const std::vector<Cell>& values) {
    if (row >= rows_) {
        VTR_LOG_ERROR("Row index out of range: %zu - max %zu", row, rows_);
    }

    for (size_t col = 0; col < std::min(values.size(), cols_); ++col) {
        data_[row][col] = values[col];
    }
}

void DataFrame::set_column(size_t col, const std::vector<Cell>& values) {
    if (col >= cols_) {
        VTR_LOG_ERROR("Column index out of range: %zu - max %zu", col, cols_);
    }

    for (size_t row = 0; row < std::min(values.size(), rows_); ++row) {
        data_[row][col] = values[row];
    }
}

size_t DataFrame::count_non_empty() const {
    size_t count = 0;
    for (const auto& row : data_) {
        for (const auto& cell : row) {
            if (!cell.is_empty()) {
                ++count;
            }
        }
    }
    return count;
}

size_t DataFrame::count_non_empty_in_range(size_t start_row, size_t start_col, size_t end_row, size_t end_col) const {
    size_t count = 0;
    for (size_t row = start_row; row < std::min(end_row, rows_); ++row) {
        for (size_t col = start_col; col < std::min(end_col, cols_); ++col) {
            if (!data_[row][col].is_empty()) {
                ++count;
            }
        }
    }
    return count;
}

void DataFrame::validate_bounds(size_t row, size_t col) const {
    if (row >= rows_ || col >= cols_) {
        VTR_LOG_ERROR("DataFrame index out of range: %zu,%zu - max %zu,%zu\n", row, col, rows_, cols_);
    }
}

// DataFrameProcessor implementation
DataFrame DataFrameProcessor::read_csv(const std::string& filename) {

    validate_csv_file(filename);

    VTR_LOG_DEBUG("Reading CSV file: %s\n", filename.c_str());

    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            VTR_LOG_ERROR("Failed to open CSV file: %s\n", filename.c_str());
            return DataFrame();
        }

        VTR_LOG_DEBUG("File %s opened successfully\n", filename.c_str());

        // Read all lines first to determine dimensions
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty()) {
                lines.push_back(line);
            }
        }
        file.close();

        if (lines.empty()) {
            VTR_LOG_ERROR("CSV file appears to be empty: %s\n", filename.c_str());
            return DataFrame();
        }

        // Parse first line to get column count
        size_t num_cols = count_csv_columns(lines[0]);
        size_t num_rows = lines.size();

        VTR_LOG_DEBUG("CSV dimensions: %zux%zu\n", num_rows, num_cols);

        DataFrame df(num_rows, num_cols);
        VTR_LOG_DEBUG("Created DataFrame with dimensions: %zux%zu\n", num_rows, num_cols);

        // Read all cells
        VTR_LOG_DEBUG("Reading cell data...\n");
        size_t cells_read = 0;

        for (size_t row = 0; row < num_rows; ++row) {
            std::vector<std::string> tokens = parse_csv_line(lines[row]);
            VTR_ASSERT_DEBUG(tokens.size() <= num_cols);
            
            for (size_t col = 0; col < tokens.size(); ++col) {
                df.at(row, col) = parse_csv_cell(tokens[col]);
                cells_read++;
            }

            // Fill remaining columns with empty cells if row has fewer columns
            for (size_t col = tokens.size(); col < num_cols; ++col) {
                df.at(row, col) = Cell();
            }

            // Progress logging for large files
            if ((row + 1) % 100 == 0) {
                VTR_LOG_DEBUG("Read %zu rows (%zu cells)\n", row + 1, cells_read);
            }
        }

        df.source_file = std::filesystem::path(filename).stem().string();
        VTR_LOG_DEBUG("Successfully read CSV file with dimensions: %zux%zu, %zu cells\n",
                      df.rows(), df.cols(), cells_read);

        return df;

    } catch (const std::exception& e) {
        VTR_LOG_ERROR("Error reading CSV file %s: %s\n", filename.c_str(), e.what());
        return DataFrame();
    }
}


size_t DataFrameProcessor::count_csv_columns(const std::string& line) {
    size_t count = 1;
    bool in_quotes = false;
    
    for (char c : line) {
        if (c == '"') {
            in_quotes = !in_quotes;
        } else if (c == ',' && !in_quotes) {
            count++;
        }
    }
    
    return count;
}

std::vector<std::string> DataFrameProcessor::parse_csv_line(const std::string& line) {
    std::vector<std::string> tokens;
    std::string current;
    bool in_quotes = false;
    
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        
        if (c == '"') {
            // Handle escaped quotes ("")
            if (in_quotes && i + 1 < line.size() && line[i + 1] == '"') {
                current += '"';
                i++; // Skip next quote
            } else {
                in_quotes = !in_quotes;
            }
        } else if (c == ',' && !in_quotes) {
            tokens.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    
    tokens.push_back(current); // Add last token
    return tokens;
}

DataFrame DataFrameProcessor::process_dataframe(DataFrame df,
                                                int merge_rows_count,
                                                int merge_cols_count) {
    VTR_LOG_DEBUG("Processing dataframe with merge_rows=%d, merge_cols=%d\n",
                  merge_rows_count, merge_cols_count);

    // Perform row merging
    std::vector<size_t> merged_row_indices = {0, 1};
    merge_rows(df, merged_row_indices);

    // Perform column merging
    std::vector<size_t> merged_col_indices = {2};
    merge_columns(df, merged_col_indices);

    // Count connections in the data area
    df.connections = df.count_non_empty_in_range(static_cast<size_t>(merge_rows_count),
                                                 static_cast<size_t>(merge_cols_count),
                                                 df.rows(),
                                                 df.cols());

    VTR_LOG_DEBUG("Processed dataframe with %zu connections\n", df.connections);

    return df;
}

void DataFrameProcessor::update_switch_delays(const DataFrame& df,
                                              int& switch_delay_max_ps,
                                              int& switch_delay_min_ps) {
    for (size_t col = NUM_EMPTY_COLS; col < df.cols(); ++col) {
        for (size_t row = NUM_EMPTY_ROWS; row < df.rows(); ++row) {
            const Cell& cell = df.at(row, col);

            if (!cell.is_empty() && cell.is_number()) {
                int switch_delay_ps = cell.as_int();

                if (switch_delay_ps > switch_delay_max_ps) {
                    switch_delay_max_ps = switch_delay_ps;
                }
                if (switch_delay_ps < switch_delay_min_ps) {
                    switch_delay_min_ps = switch_delay_ps;
                }
            }
        }
    }

    VTR_LOG_DEBUG("Updated switch delays: min=%d, max=%d\n",
                  switch_delay_min_ps,
                  switch_delay_max_ps);
}

// Parse csv cell to our Cell type
Cell DataFrameProcessor::parse_csv_cell(const std::string& value) {
    // Trim whitespace
    std::string trimmed = value;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
    trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);
    
    if (trimmed.empty()) {
        return Cell(); // Empty cell
    }
    
    // Try to parse as number
    try {
        size_t pos;
        double num = std::stod(trimmed, &pos);
        if (pos == trimmed.length()) {
            return Cell(num);
        }
    } catch (...) {
        // Not a number, treat as string
    }
    
    return Cell(trimmed);
}

void DataFrameProcessor::merge_rows(DataFrame& df, const std::vector<size_t>& merge_row_indices) {
    for (const auto& row : merge_row_indices) {
        Cell value;
        for (size_t col = 0; col < df.cols(); ++col) {
            if (!df.at(row, col).is_empty()) {
                value = df.at(row, col);
            }
            df.at(row, col) = value;
        }
    }
}

void DataFrameProcessor::merge_columns(DataFrame& df, const std::vector<size_t>& merge_col_indices) {
    for (const auto& col : merge_col_indices) {
        Cell value;
        for (size_t row = 0; row < df.rows(); ++row) {
            if (!df.at(row, col).is_empty()) {
                value = df.at(row, col);
            }
            df.at(row, col) = value;
        }
    }
}

void DataFrameProcessor::validate_csv_file(const std::string& filename) {
    if (!std::filesystem::exists(filename)) {
        VTR_LOG_ERROR("CSV file does not exist: %s\n", filename.c_str());
    }

    // Check file extension
    std::string extension = std::filesystem::path(filename).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    if (extension != ".csv") {
        VTR_LOG_ERROR("Unsupported file format: %s. Expected .csv\n", extension.c_str());
    }
}

} // namespace crrgenerator
