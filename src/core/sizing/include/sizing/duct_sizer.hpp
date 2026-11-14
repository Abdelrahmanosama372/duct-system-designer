#pragma once

#include "sizing/duct_sizer_config.hpp"
#include "sizing/errors.hpp"
#include "sizing/models.hpp"

#include <functional>
#include <utility>

namespace dsd::sizing {

inline constexpr double CFM_TO_M3_S = 0.3048 * 0.3048 * 0.3048 / 60.0;
inline constexpr double FPM_TO_M_S = 0.3048 / 60.0;
inline constexpr double IN_WG_PER_100_FT_TO_PA_M = 249.08891 / 30.48;

class DuctSizer {
public:
    explicit DuctSizer(const DuctSizerConfig& config);
    virtual ~DuctSizer() = default;

    [[nodiscard]] virtual DuctDimension size_duct(const SizingContext& context) const = 0;
    [[nodiscard]] static double equivalent_diameter(double width, double height);
    [[nodiscard]] DuctDimension find_eqv_rectangular_duct_dimensions(
        double diameter, int starting_width) const;

protected:
    const DuctSizerConfig config_;
    [[nodiscard]] int normalize_root_width(double width) const;
    [[nodiscard]] int normalize_preferred_width(double width) const;
    [[nodiscard]] DuctDimension select_dimensions(
        int starting_width,
        const std::function<std::optional<double>(int)>& required_height) const;

    // Caller establishes a bracket with a predicate changing from false to true.
    // Both bounds are returned so numerical callers can choose the conservative side.
    [[nodiscard]] static std::pair<double, double> bisect(
        double lower, double upper, const std::function<bool(double)>& predicate,
        double epsilon, int iteration_limit);

private:
    [[nodiscard]] std::optional<double> height_for_equivalent_diameter(
        int width, double diameter) const;
};

} // namespace dsd::sizing
