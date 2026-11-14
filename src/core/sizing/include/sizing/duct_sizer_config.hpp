#pragma once

#include <optional>

namespace dsd::sizing {

// Construction stores the inputs, validates them, then normalizes dimension bounds.
struct DuctSizerConfig {
    DuctSizerConfig(
        int size_increment,
        int minimum_duct_dimension,
        int maximum_duct_dimension,
        double aspect_ratio,
        double air_density,
        double dynamic_viscosity,
        double roughness,
        std::optional<double> required_velocity = std::nullopt,
        std::optional<double> required_head_loss = std::nullopt,
        double diameter_tolerance_mm = 0.0001,
        int dimension_solver_max_iterations = 30,
        double friction_tolerance = 1e-10,
        int friction_factor_solver_max_iterations = 80);

    const int size_increment; // mm; grid anchored at zero
    int minimum_duct_dimension; // mm; rounded up to the size grid
    int maximum_duct_dimension; // mm; rounded down to the size grid
    const double aspect_ratio; // upper limit on width / height (not its reciprocal)
    const double air_density; // kg/m^3
    const double dynamic_viscosity; // Pa.s
    const double roughness; // m
    const std::optional<double> required_velocity; // FPM
    const std::optional<double> required_head_loss; // in. w.g./100 ft
    const double diameter_tolerance_mm;
    const int dimension_solver_max_iterations;
    const double friction_tolerance;
    const int friction_factor_solver_max_iterations;

private:
    void validate() const;
};

} // namespace dsd::sizing
