#pragma once

/**
 * @file data_frame_processor.h
 * @brief Simple dataframe implementation for processing switch template data
 *
 * This class provides functions for processing switch template files
 * containing switch block configuration data.
 */

#include <string>
#include <vector>
#include <variant>

#include "crr_common.h"
namespace crrgenerator {

/**
 * @brief Represents a cell in the dataframe
 */
struct Cell {
    using Value = std::variant<std::monostate, std::string, int64_t>;

    Value value;

    Cell() = default;
    explicit Cell(const std::string& v)
        : value(v) {}
    explicit Cell(int64_t v)
        : value(v) {}

    bool is_empty() const {
        return std::holds_alternative<std::monostate>(value);
    }

    bool is_string() const {
        return std::holds_alternative<std::string>(value);
    }

    bool is_integer() const {
        return std::holds_alternative<int64_t>(value);
    }

    bool is_number() const {
        return is_integer();
    }

    int64_t as_int() const {
        if (auto p = std::get_if<int64_t>(&value)) {
            return *p;
        }
        if (auto p = std::get_if<std::string>(&value)) {
            return std::stoll(*p);
        }
        return 0;
    }

    std::string as_string() const {
        if (auto p = std::get_if<std::string>(&value)) {
            return *p;
        }
        if (auto p = std::get_if<int64_t>(&value)) {
            return std::to_string(*p);
        }
        return {};
    }
};

/**
 * @brief Simple dataframe implementation for processing switch template data
 *
 * This class provides functions for processing switch template files
 * containing switch block configuration data.
 */
class DataFrame {
  public:
    DataFrame() = default;
    DataFrame(size_t rows, size_t cols);

    // Access methods
    Cell& at(size_t row, size_t col);
    const Cell& at(size_t row, size_t col) const;
    Cell& operator()(size_t row, size_t col) { return at(row, col); }
    const Cell& operator()(size_t row, size_t col) const { return at(row, col); }

    // Dimensions
    size_t rows() const { return rows_; }
    size_t cols() const { return cols_; }
    std::pair<size_t, size_t> shape() const { return {rows_, cols_}; }

    // Resize operations
    void resize(size_t rows, size_t cols);
    void clear();

    // Row/column operations
    std::vector<Cell> get_row(size_t row) const;
    std::vector<Cell> get_column(size_t col) const;
    void set_row(size_t row, const std::vector<Cell>& values);
    void set_column(size_t col, const std::vector<Cell>& values);

    // Statistics and counting
    size_t count_non_empty() const;
    size_t count_non_empty_in_range(size_t start_row, size_t start_col, size_t end_row, size_t end_col) const;

    // Iteration support
    class RowIterator {
      public:
        RowIterator(DataFrame* df, size_t row)
            : df_(df)
            , row_(row) {}

        Cell& operator[](size_t col) { return df_->at(row_, col); }
        const Cell& operator[](size_t col) const { return df_->at(row_, col); }

        size_t size() const { return df_->cols(); }
        size_t get_row_index() const { return row_; }

        RowIterator& operator++() {
            ++row_;
            return *this;
        }
        bool operator!=(const RowIterator& other) const {
            return df_ != other.df_ || row_ != other.row_;
        }
        RowIterator& operator*() { return *this; }

      private:
        DataFrame* df_;
        size_t row_;
    };

    class ConstRowIterator {
      public:
        ConstRowIterator(const DataFrame* df, size_t row)
            : df_(df)
            , row_(row) {}

        const Cell& operator[](size_t col) const { return df_->at(row_, col); }

        size_t size() const { return df_->cols(); }
        size_t get_row_index() const { return row_; }

        ConstRowIterator& operator++() {
            ++row_;
            return *this;
        }
        bool operator!=(const ConstRowIterator& other) const {
            return row_ != other.row_;
        }
        const ConstRowIterator& operator*() const { return *this; }

      private:
        const DataFrame* df_;
        size_t row_;
    };

    RowIterator begin() { return RowIterator(this, 0); }
    RowIterator end() { return RowIterator(this, rows_); }
    ConstRowIterator begin() const { return ConstRowIterator(this, 0); }
    ConstRowIterator end() const { return ConstRowIterator(this, rows_); }

    // Metadata
    std::string source_file;
    size_t connections = 0;

  private:
    std::vector<std::vector<Cell>> data_;
    size_t rows_ = 0;
    size_t cols_ = 0;

    void validate_bounds(size_t row, size_t col) const;
};

/**
 * @brief Processes switch template files and converts them to DataFrames
 *
 * This class handles reading switch template files and performing the cell merging
 * operations similar to the Python pandas functionality.
 */
class DataFrameProcessor {
  public:
    DataFrameProcessor() = default;

    /**
     * @brief Read switch template file and create DataFrame
     * @param filename Path to switch template file
     * @return DataFrame containing the switch template data
     * @throws FileException if file cannot be read
     */
    DataFrame read_csv(const std::string& filename);

    /**
     * @brief Process DataFrame with merging operations
     * @param df DataFrame to process
     * @param merge_rows_count Number of rows to merge
     * @param merge_cols_count Number of columns to merge
     */
    void process_dataframe(DataFrame& df,
                           int merge_rows_count = NUM_EMPTY_ROWS,
                           int merge_cols_count = NUM_EMPTY_COLS);

    /**
     * @brief Update switch delay min/max based on DataFrame content
     * @param df DataFrame to analyze
     * @param switch_delay_max_ps Current maximum switch delay (will be updated)
     * @param switch_delay_min_ps Current minimum switch delay (will be updated)
     */
    void update_switch_delays(const DataFrame& df, int& switch_delay_max_ps, int& switch_delay_min_ps);

  private:
    // switch template parsing helpers
    Cell parse_csv_cell(const std::string& value);
    size_t count_csv_columns(const std::string& line);
    std::vector<std::string> parse_csv_line(const std::string& line);
    void merge_rows(DataFrame& df, const std::vector<size_t>& merge_row_indices);
    void merge_columns(DataFrame& df, const std::vector<size_t>& merge_col_indices);

    // Validation
    void validate_csv_file(const std::string& filename);
};

} // namespace crrgenerator
