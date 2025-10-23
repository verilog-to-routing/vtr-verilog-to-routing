#include "data_frame_processor.h"

#include "vtr_log.h"

#include <filesystem>
#include <algorithm>

namespace crrgenerator {

// DataFrame implementation

DataFrame::DataFrame(size_t rows, size_t cols) : rows_(rows), cols_(cols) {
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

size_t DataFrame::count_non_empty_in_range(size_t start_row, size_t start_col,
                                           size_t end_row,
                                           size_t end_col) const {
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
DataFrame DataFrameProcessor::read_excel(const std::string& filename) {

    validate_excel_file(filename);

    VTR_LOG_DEBUG("Reading Excel file: %s\n", filename.c_str());

    try {
        // Open the Excel document
        xlnt::workbook wb;
        wb.load(filename);

        VTR_LOG_DEBUG("Document %s opened successfully\n", filename.c_str());

        // Get the first worksheet
        auto worksheet = wb.active_sheet();
        std::string sheet_name = worksheet.title();
        
        VTR_LOG_DEBUG("Got worksheet: '%s'\n", sheet_name.c_str());

        // Get the used range dimensions
        auto used_range = worksheet.calculate_dimension();
        size_t last_row = used_range.bottom_right().row();
        size_t last_col = used_range.bottom_right().column_index();

        VTR_LOG_DEBUG("Worksheet dimensions: %zux%zu\n", last_row, last_col);

        // Safety check
        if (last_row > 10000 || last_col > 1000) {
            VTR_LOG_ERROR("Excel file too large: %zux%zu (limit: 10000x1000)\n",
                        last_row, last_col);
        }

        if (last_row == 0 || last_col == 0) {
            VTR_LOG_ERROR("Excel file appears to be empty: %s\n", filename.c_str());
        }

        DataFrame df(last_row, last_col);
        VTR_LOG_DEBUG("Created DataFrame with dimensions: %zux%zu\n", last_row, last_col);

        // Read all cells
        VTR_LOG_DEBUG("Reading cell data...\n");
        size_t cells_read = 0;

        for (size_t row = 1; row <= last_row; ++row) {
            for (size_t col = 1; col <= last_col; ++col) {
                try {
                    // xlnt uses 1-based indexing like Excel
                    auto cell = worksheet.cell(col, row);
                    df.at(row - 1, col - 1) = parse_excel_cell(cell);
                    cells_read++;
                } catch (const std::exception& e) {
                    VTR_LOG_DEBUG("Error reading cell (%zu, %zu): %s\n", row, col, e.what());
                    df.at(row - 1, col - 1) = Cell();  // Empty cell
                }
            }

            // Progress logging for large files
            if (row % 100 == 0) {
                VTR_LOG_DEBUG("Read %zu rows (%zu cells)\n", row, cells_read);
            }
        }

        df.source_file = std::filesystem::path(filename).stem().string();
        VTR_LOG_DEBUG("Successfully read Excel file with dimensions: %zux%zu, %zu cells\n",
                       df.rows(), df.cols(), cells_read);

        return df;

    } catch (const std::exception& e) {
        VTR_LOG_ERROR("Error reading Excel file %s: %s\n", filename.c_str(), e.what());
        return DataFrame();
    }
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

// Parse xlnt cell to our Cell type
Cell DataFrameProcessor::parse_excel_cell(const xlnt::cell& cell) {
    // Check if cell has a value
    if (!cell.has_value()) {
        return Cell();
    }

    // Get the data type
    auto data_type = cell.data_type();

    switch (data_type) {
        case xlnt::cell::type::boolean:
            return Cell(static_cast<int64_t>(cell.value<bool>() ? 1 : 0));

        case xlnt::cell::type::number: {
            double value = cell.value<double>();
            // Check if it's actually an integer
            if (value == std::floor(value) && std::abs(value) < 9007199254740992.0) {
                return Cell(static_cast<int64_t>(value));
            }
            return Cell(value);
        }

        case xlnt::cell::type::shared_string:
        case xlnt::cell::type::inline_string:
        case xlnt::cell::type::formula_string: {
            std::string value = cell.to_string();
            if (value.empty()) {
                return Cell();
            }
            return Cell(value);
        }

        case xlnt::cell::type::date: {
            // Convert date to string representation
            return Cell(cell.to_string());
        }

        case xlnt::cell::type::empty:
        default:
            return Cell();
    }
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

void DataFrameProcessor::validate_excel_file(const std::string& filename) {
    if (!std::filesystem::exists(filename)) {
        VTR_LOG_ERROR("Excel file does not exist: %s\n", filename.c_str());
    }

    // Check file extension
    std::string extension = std::filesystem::path(filename).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    if (extension != ".xlsx" && extension != ".xls") {
        VTR_LOG_ERROR("Unsupported file format: %s. Expected .xlsx or .xls\n", extension.c_str());
    }
}

}  // namespace crrgenerator