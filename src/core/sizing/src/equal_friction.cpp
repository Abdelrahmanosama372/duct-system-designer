#include "sizing/equal_friction.hpp"

#include <cmath>
#include <numbers>

namespace dsd::sizing {

EqualFrictionDuctSizer::EqualFrictionDuctSizer(const DuctSizerConfig& config)
    : DuctSizer(config) {
    if (!config_.required_head_loss) {
        throw std::invalid_argument(
            "Equal friction sizing requires required_head_loss (in. w.g./100 ft)");
    }
    target_pa_m_ = *config_.required_head_loss * IN_WG_PER_100_FT_TO_PA_M;
    if (!std::isfinite(target_pa_m_) || target_pa_m_ <= 0) {
        throw std::invalid_argument("required_head_loss exceeds numerical range after conversion");
    }
}

DuctDimension EqualFrictionDuctSizer::size_duct(const SizingContext& context) const {
    const double diameter = find_equivalent_diameter(context.flow);
    int width;
    if (context.preferred_width) {
        width = normalize_preferred_width(*context.preferred_width);
    } else {
        const double ratio = config_.aspect_ratio;
        const double height = diameter * std::pow(ratio + 1, 0.25) /
                              (1.30 * std::pow(ratio, 0.625));
        width = normalize_root_width(ratio * height);
    }
    const auto result = find_eqv_rectangular_duct_dimensions(diameter, width);
    if (pressure_loss_pa_m(context.flow, equivalent_diameter(result.width, result.height)) >
        target_pa_m_) {
        throw NumericalConvergenceError("Rounded dimensions do not meet the friction target");
    }
    return result;
}

double EqualFrictionDuctSizer::find_equivalent_diameter(double flow) const {
    const SizingContext validated(flow);
    const double lower = 20.0;
    const double upper = equivalent_diameter(config_.maximum_duct_dimension, config_.maximum_duct_dimension);
    if (upper < lower || pressure_loss_pa_m(flow, upper) > target_pa_m_) {
        throw NoSolutionWithinBounds("Maximum permitted duct cannot meet the friction target");
    }
    if (pressure_loss_pa_m(flow, lower) <= target_pa_m_) return lower;
    return bisect(lower, upper, [this, flow](double diameter) {
        return pressure_loss_pa_m(flow, diameter) <= target_pa_m_;
    }, config_.diameter_tolerance_mm, config_.dimension_solver_max_iterations).second;
}

double EqualFrictionDuctSizer::pressure_loss_pa_m(double flow, double diameter) const {
    const SizingContext validated(flow);
    if (!std::isfinite(diameter) || diameter <= 0) {
        throw std::invalid_argument("diameter must be positive and finite (mm)");
    }
    const double diameter_m = diameter / 1000;
    const double velocity = flow * CFM_TO_M3_S /
                            (std::numbers::pi * diameter_m * diameter_m / 4);
    const double reynolds = config_.air_density * velocity * diameter_m /
                            config_.dynamic_viscosity;
    if (!std::isfinite(reynolds) || reynolds <= 0) {
        throw NumericalConvergenceError("Reynolds number exceeds numerical range");
    }
    const double friction = friction_factor(reynolds, diameter);
    const double loss = friction * config_.air_density * velocity * velocity / (2 * diameter_m);
    if (!std::isfinite(loss)) {
        throw NumericalConvergenceError("Pressure loss exceeds numerical range");
    }
    return loss;
}

double EqualFrictionDuctSizer::friction_factor(double reynolds, double diameter) const {
    if (!std::isfinite(reynolds) || reynolds <= 0 || !std::isfinite(diameter) || diameter <= 0) {
        throw std::invalid_argument("Reynolds number and diameter must be positive and finite");
    }
    if (reynolds < 2300) return 64 / reynolds;
    const double roughness_term = config_.roughness / (3.7 * diameter / 1000);
    const auto residual = [roughness_term, reynolds](double x) {
        return x + 2 * std::log10(roughness_term + 2.51 * x / reynolds);
    };
    constexpr double lower = 1e-12;
    if (residual(lower) >= 0) {
        throw NoSolutionWithinBounds("Roughness is too large for the requested diameter");
    }
    double upper = 1;
    bool bracketed = false;
    for (int iteration = 0; iteration < config_.friction_factor_solver_max_iterations; ++iteration) {
        if (residual(upper) >= 0) {
            bracketed = true;
            break;
        }
        upper *= 2;
    }
    if (!bracketed) {
        throw NumericalConvergenceError("Could not bracket the Colebrook friction factor");
    }
    const double x = bisect(lower, upper, [&residual](double value) {
        return residual(value) >= 0;
    }, config_.friction_tolerance, config_.friction_factor_solver_max_iterations).first;
    return 1 / (x * x); // Lower x gives a conservative, larger Darcy factor.
}

} // namespace dsd::sizing
