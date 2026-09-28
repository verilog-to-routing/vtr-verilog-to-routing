#pragma once

/**
 * @file crr_pattern_matcher.h
 * @brief Helper class used by the CRR Generator to determine which switch block pattern
 * should be applied to each tile.
 *
 * Patterns have the shape "SB_<spec>__<spec>_", where <spec> is a literal
 * number, "*" (any value), a value list "[a,b,...]" or a range
 * "[start:end:step]" (end inclusive). Each pattern is parsed once at
 * registration into numeric coordinate matchers, so a per-tile query is a
 * couple of integer comparisons. Patterns outside this grammar are rejected
 * with a fatal error at registration.
 */

#include <algorithm>
#include <charconv>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "vpr_error.h"

namespace crrgenerator {

/**
 * @brief Parses switch block patterns into numeric coordinate matchers and
 *        answers per-tile pattern queries.
 */
class CRRPatternMatcher {
  private:
    /**
     * @brief Numeric matcher for one coordinate of an "SB_<x>__<y>_" pattern.
     *
     * ANY accepts every value, VALUE accepts one value, LIST accepts an
     * explicit set, RANGE accepts [start, end] (end inclusive) filtered by
     * step, and NEVER accepts nothing (e.g. a literal with leading zeros,
     * which can never equal a formatted coordinate, or a bracket expression
     * whose colon-separated part count is not exactly 3).
     */
    struct CoordSpec {
        enum class e_kind { NEVER,
                            ANY,
                            VALUE,
                            LIST,
                            RANGE };
        e_kind kind = e_kind::NEVER;
        int start = 0;           ///< VALUE: the value; RANGE: range start
        int end = 0;             ///< RANGE: range end (inclusive)
        int step = 1;            ///< RANGE: step
        std::vector<int> values; ///< LIST: accepted values

        bool matches(int v) const {
            switch (kind) {
                case e_kind::ANY:
                    return true;
                case e_kind::VALUE:
                    return v == start;
                case e_kind::LIST:
                    return std::find(values.begin(), values.end(), v) != values.end();
                case e_kind::RANGE:
                    if (step == 0 || v < start || v > end) return false;
                    return (v - start) % step == 0;
                case e_kind::NEVER:
                default:
                    return false;
            }
        }
    };

  public:
    /**
     * @brief Parse a pattern and append it to the matcher. Patterns are
     *        indexed in registration order; matches() takes that index.
     *        Raises a fatal error for patterns outside the supported grammar.
     */
    void register_pattern(const std::string& pattern) {
        std::optional<std::pair<CoordSpec, CoordSpec>> parsed = parse_sb_pattern(pattern);
        if (!parsed.has_value()) {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                            "Unsupported switch block pattern '%s': expected the shape "
                            "SB_<spec>__<spec>_ where <spec> is a number, '*', a value list "
                            "'[a,b,...]' or a range '[start:end:step]'\n",
                            pattern.c_str());
        }
        patterns_.push_back(std::move(*parsed));
    }

    /**
     * @brief Returns true if the switch block at (x, y) matches the pattern
     *        with the given registration index.
     */
    bool matches(size_t pattern_idx, size_t x, size_t y) const {
        const std::pair<CoordSpec, CoordSpec>& spec = patterns_[pattern_idx];
        return spec.first.matches(static_cast<int>(x))
               && spec.second.matches(static_cast<int>(y));
    }

  private:
    /// Parsed coordinate matchers, in registration order
    std::vector<std::pair<CoordSpec, CoordSpec>> patterns_;

    // Parse a non-negative integer strictly (whole string, digits only).
    static std::optional<int> parse_strict_int(std::string_view s) {
        if (s.empty()) return std::nullopt;
        int value = 0;
        const char* first = s.data();
        const char* last = s.data() + s.size();
        auto [ptr, ec] = std::from_chars(first, last, value);
        if (ec != std::errc() || ptr != last) return std::nullopt;
        return value;
    }

    // Parse one coordinate spec: "*", all-digits literal, "[a,b,...]",
    // "[start:end:step]" or "[value]". Returns nullopt when the spec does not
    // fit these shapes.
    static std::optional<CoordSpec> parse_coord_spec(std::string_view spec) {
        CoordSpec result;
        if (spec == "*") {
            result.kind = CoordSpec::e_kind::ANY;
            return result;
        }
        if (!spec.empty() && spec.front() == '[' && spec.back() == ']') {
            std::string_view content = spec.substr(1, spec.size() - 2);
            std::vector<std::string_view> parts;
            char sep = (content.find(',') != std::string_view::npos) ? ',' : ':';
            size_t pos = 0;
            while (pos <= content.size()) {
                size_t sep_pos = content.find(sep, pos);
                if (sep_pos == std::string_view::npos) sep_pos = content.size();
                parts.push_back(content.substr(pos, sep_pos - pos));
                pos = sep_pos + 1;
            }
            std::vector<int> ints;
            for (std::string_view part : parts) {
                std::optional<int> v = parse_strict_int(part);
                if (!v.has_value()) return std::nullopt;
                ints.push_back(*v);
            }
            if (sep == ',' && ints.size() >= 2) {
                result.kind = CoordSpec::e_kind::LIST;
                result.values = std::move(ints);
            } else if (sep == ':' && ints.size() > 1) {
                // Exactly start:end:step is a range; any other colon-separated
                // shape never matches.
                if (ints.size() == 3) {
                    result.kind = CoordSpec::e_kind::RANGE;
                    result.start = ints[0];
                    result.end = ints[1];
                    result.step = ints[2];
                } else {
                    result.kind = CoordSpec::e_kind::NEVER;
                }
            } else if (ints.size() == 1) {
                result.kind = CoordSpec::e_kind::VALUE;
                result.start = ints[0];
            } else {
                return std::nullopt;
            }
            return result;
        }
        // Literal: must be all digits, and free of leading zeros so that it can
        // equal a std::to_string()-formatted coordinate.
        std::optional<int> v = parse_strict_int(spec);
        if (!v.has_value()) return std::nullopt;
        result.kind = (std::to_string(*v) == spec) ? CoordSpec::e_kind::VALUE : CoordSpec::e_kind::NEVER;
        result.start = *v;
        return result;
    }

    // Parse a full "SB_<spec>__<spec>_" pattern into per-coordinate matchers.
    static std::optional<std::pair<CoordSpec, CoordSpec>> parse_sb_pattern(const std::string& pattern) {
        std::string_view p(pattern);
        if (p.size() < 7 || p.substr(0, 3) != "SB_" || p.back() != '_') return std::nullopt;
        std::string_view middle = p.substr(3, p.size() - 4);
        size_t sep = middle.find("__");
        if (sep == std::string_view::npos || middle.find("__", sep + 1) != std::string_view::npos) {
            return std::nullopt;
        }
        std::optional<CoordSpec> x_spec = parse_coord_spec(middle.substr(0, sep));
        std::optional<CoordSpec> y_spec = parse_coord_spec(middle.substr(sep + 2));
        if (!x_spec.has_value() || !y_spec.has_value()) return std::nullopt;
        return std::make_pair(std::move(*x_spec), std::move(*y_spec));
    }
};

} // namespace crrgenerator
