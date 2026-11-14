#include "sizing/duct_sizer.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace dsd::sizing {

DuctSizer::DuctSizer(const DuctSizerConfig& config) : config_(config) {}

double DuctSizer::equivalent_diameter(double width, double height) {
    if (!std::isfinite(width) || !std::isfinite(height) || width <= 0 || height <= 0) {
        throw std::invalid_argument("Equivalent diameter requires positive finite dimensions");
    }
    const double diameter = 1.30 * std::pow(width * height, 0.625) /
                            std::pow(width + height, 0.25);
    if (!std::isfinite(diameter) || diameter <= 0) {
        throw NumericalConvergenceError("Equivalent diameter exceeds numerical range");
    }
    return diameter;
}

int DuctSizer::normalize_root_width(double width) const {
    if (std::isnan(width)) {
        throw NumericalConvergenceError("Root width exceeds numerical range");
    }
    const int step = config_.size_increment;
    return static_cast<int>(std::clamp(std::ceil(width / step) * step,
                                       static_cast<double>(config_.minimum_duct_dimension),
                                       static_cast<double>(config_.maximum_duct_dimension)));
}

int DuctSizer::normalize_preferred_width(double width) const {
    const int step = config_.size_increment;
    return static_cast<int>(std::min(static_cast<double>(config_.maximum_duct_dimension),
                                     std::floor(width / step) * step));
}

DuctDimension DuctSizer::select_dimensions(
    int starting_width,
    const std::function<std::optional<double>(int)>& required_height) const {
    const int step = config_.size_increment;
    if (starting_width < 0 || starting_width > config_.maximum_duct_dimension ||
        starting_width % step != 0) {
        throw std::invalid_argument("starting_width must be a nonnegative grid size within the maximum (mm)");
    }
    for (int width = starting_width; width >= config_.minimum_duct_dimension; width -= step) {
        const auto needed = required_height(width);
        if (needed) {
            const double height = std::max(static_cast<double>(config_.minimum_duct_dimension),
                                            std::ceil(*needed / step) * step);
            if (height <= config_.maximum_duct_dimension && width / height <= config_.aspect_ratio) {
                return {width, static_cast<int>(height)};
            }
        }
    }
    throw NoSolutionWithinBounds(
        "No size satisfies dimension, increment, aspect-ratio, and sizing constraints");
}

DuctDimension DuctSizer::find_eqv_rectangular_duct_dimensions(
    double diameter, int starting_width) const {
    if (!std::isfinite(diameter) || diameter <= 0) {
        throw std::invalid_argument("equivalent_diameter must be positive and finite (mm)");
    }
    return select_dimensions(starting_width, [this, diameter](int width) {
        return height_for_equivalent_diameter(width, diameter);
    });
}

std::optional<double> DuctSizer::height_for_equivalent_diameter(int width, double diameter) const {
    if (equivalent_diameter(width, config_.minimum_duct_dimension) >= diameter) return config_.minimum_duct_dimension;
    if (equivalent_diameter(width, config_.maximum_duct_dimension) < diameter) return std::nullopt;
    return bisect(config_.minimum_duct_dimension, config_.maximum_duct_dimension, [width, diameter](double height) {
        return equivalent_diameter(width, height) >= diameter;
    }, config_.diameter_tolerance_mm, config_.dimension_solver_max_iterations).second;
}

std::pair<double, double> DuctSizer::bisect(
    double lower, double upper, const std::function<bool(double)>& predicate,
    double epsilon, int iteration_limit) {
    for (int iteration = 0; iteration < iteration_limit; ++iteration) {
        if (upper - lower <= epsilon) return {lower, upper};
        const double middle = std::midpoint(lower, upper);
        if (predicate(middle)) upper = middle;
        else lower = middle;
    }
    if (upper - lower <= epsilon) return {lower, upper};
    throw NumericalConvergenceError("Bisection did not reach the configured tolerance");
}

} // namespace dsd::sizing
