#include "sizing/duct_sizer_config.hpp"

#include <cmath>
#include <stdexcept>
#include <string>

namespace dsd::sizing {
namespace {
void positive(double value, const char* name) {
    if (!std::isfinite(value) || value <= 0) {
        throw std::invalid_argument(std::string(name) + " must be positive and finite");
    }
}

} // namespace

DuctSizerConfig::DuctSizerConfig(
    int size_increment_value,
    int minimum_duct_dimension_value,
    int maximum_duct_dimension_value,
    double aspect_ratio_value,
    double air_density_value,
    double dynamic_viscosity_value,
    double roughness_value,
    std::optional<double> required_velocity_value,
    std::optional<double> required_head_loss_value,
    double diameter_tolerance_mm_value,
    int dimension_solver_max_iterations_value,
    double friction_tolerance_value,
    int friction_factor_solver_max_iterations_value)
    : size_increment(size_increment_value),
      minimum_duct_dimension(minimum_duct_dimension_value),
      maximum_duct_dimension(maximum_duct_dimension_value),
      aspect_ratio(aspect_ratio_value),
      air_density(air_density_value),
      dynamic_viscosity(dynamic_viscosity_value),
      roughness(roughness_value),
      required_velocity(required_velocity_value),
      required_head_loss(required_head_loss_value),
      diameter_tolerance_mm(diameter_tolerance_mm_value),
      dimension_solver_max_iterations(dimension_solver_max_iterations_value),
      friction_tolerance(friction_tolerance_value),
      friction_factor_solver_max_iterations(friction_factor_solver_max_iterations_value) {
    validate();
    minimum_duct_dimension =
        (minimum_duct_dimension / size_increment +
         (minimum_duct_dimension % size_increment != 0)) * size_increment;
    maximum_duct_dimension = maximum_duct_dimension / size_increment * size_increment;
}

void DuctSizerConfig::validate() const {
    positive(size_increment, "size_increment");
    positive(minimum_duct_dimension, "minimum_duct_dimension");
    positive(maximum_duct_dimension, "maximum_duct_dimension");
    positive(dimension_solver_max_iterations, "dimension_solver_max_iterations");
    positive(friction_factor_solver_max_iterations, "friction_factor_solver_max_iterations");
    positive(aspect_ratio, "aspect_ratio");
    positive(air_density, "air_density");
    positive(dynamic_viscosity, "dynamic_viscosity");
    positive(diameter_tolerance_mm, "diameter_tolerance_mm");
    positive(friction_tolerance, "friction_tolerance");
    if (required_velocity) positive(*required_velocity, "required_velocity");
    if (required_head_loss) positive(*required_head_loss, "required_head_loss");
    if (!std::isfinite(roughness) || roughness < 0) {
        throw std::invalid_argument("roughness must be finite and nonnegative (m)");
    }
    if (minimum_duct_dimension > maximum_duct_dimension) {
        throw std::invalid_argument("minimum_duct_dimension exceeds maximum_duct_dimension");
    }
    const int first_multiple = minimum_duct_dimension / size_increment +
                               (minimum_duct_dimension % size_increment != 0);
    if (first_multiple > maximum_duct_dimension / size_increment) {
        throw std::invalid_argument("Dimension bounds contain no permitted size increments");
    }
}

} // namespace dsd::sizing
