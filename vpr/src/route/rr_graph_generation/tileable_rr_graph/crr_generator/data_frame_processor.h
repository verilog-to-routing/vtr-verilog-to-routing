#pragma once

#include <string>
#include <vector>

#include "OpenXLSX.hpp"

namespace crrgenerator {

constexpr int NUM_EMPTY_ROWS = 5;
constexpr int NUM_EMPTY_COLS = 4;


/**
 * @brief Represents a cell in the dataframe
 */
struct Cell {
    enum class Type { EMPTY, STRING, INTEGER, DOUBLE };

    Type type = Type::EMPTY;
    std::string string_value;
    int64_t int_value = 0;
    double double_value = 0.0;

    Cell() = default;
    explicit Cell(const std::string& value)
        : type(Type::STRING), string_value(value) {}
    explicit Cell(int64_t value) : type(Type::INTEGER), int_value(value) {}
    explicit Cell(double value) : type(Type::DOUBLE), double_value(value) {}

    bool is_empty() const { return type == Type::EMPTY; }
    bool is_string() const { return type == Type::STRING; }
    bool is_integer() const { return type == Type::INTEGER; }
    bool is_double() const { return type == Type::DOUBLE; }
    bool is_number() const { return is_integer() || is_double(); }

    int64_t as_int() const {
        if (is_integer()) return int_value;
        if (is_double()) return static_cast<int64_t>(double_value);
        if (is_string()) return std::stoll(string_value);
        return 0;
    }

    double as_double() const {
        if (is_double()) return double_value;
        if (is_integer()) return static_cast<double>(int_value);
        if (is_string()) return std::stod(string_value);
        return 0.0;
    }

    std::string as_string() const {
        if (is_string()) return string_value;
        if (is_integer()) return std::to_string(int_value);
        if (is_double()) return std::to_string(double_value);
        return "";
    }
};

/**
 * @brief Simple dataframe implementation for processing Excel data
 *
 * This class provides pandas-like functionality for processing Excel files
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
        size_t count_non_empty_in_range(size_t start_row, size_t start_col,
                                        size_t end_row, size_t end_col) const;

        // Iteration support
        class RowIterator {
            public:
                RowIterator(DataFrame* df, size_t row) : df_(df), row_(row) {}

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
                ConstRowIterator(const DataFrame* df, size_t row) : df_(df), row_(row) {}

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
 * @brief Processes Excel files and converts them to DataFrames
 *
 * This class handles reading Excel files and performing the cell merging
 * operations similar to the Python pandas functionality.
 */
class DataFrameProcessor {
    public:
        DataFrameProcessor() = default;

        /**
        * @brief Read Excel file and create DataFrame
        * @param filename Path to Excel file
        * @return DataFrame containing the Excel data
        * @throws FileException if file cannot be read
        */
        DataFrame read_excel(const std::string& filename);

        /**
        * @brief Process DataFrame with merging operations
        * @param df DataFrame to process
        * @param merge_rows_count Number of rows to merge
        * @param merge_cols_count Number of columns to merge
        * @return Processed DataFrame
        */
        DataFrame process_dataframe(DataFrame df,
                                    int merge_rows_count = NUM_EMPTY_ROWS,
                                    int merge_cols_count = NUM_EMPTY_COLS);

        /**
        * @brief Update switch delay min/max based on DataFrame content
        * @param df DataFrame to analyze
        * @param switch_delay_max_ps Current maximum switch delay (will be updated)
        * @param switch_delay_min_ps Current minimum switch delay (will be updated)
        */
        void update_switch_delays(const DataFrame& df, int& switch_delay_max_ps,
                                  int& switch_delay_min_ps);

    private:
        // Excel parsing helpers
        Cell parse_excel_cell(const OpenXLSX::XLCellValue& cell_value);
        void merge_rows(DataFrame& df, const std::vector<size_t>& merge_row_indices);
        void merge_columns(DataFrame& df, const std::vector<size_t>& merge_col_indices);

        // Validation
        void validate_excel_file(const std::string& filename);
};

}  // namespace crrgenerator
