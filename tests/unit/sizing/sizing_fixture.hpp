#pragma once

#include "sizing/duct_sizer_config.hpp"
#include <gtest/gtest.h>

namespace dsd::sizing::test {

class SizingTest : public ::testing::Test {
public:
    // Editable test inputs; production configs are constructed by make_config().
    int size_increment = 25;
    int minimum_duct_dimension = 25;
    int maximum_duct_dimension = 2000;
    double aspect_ratio = 2;
    double air_density = 1.1180887435024176;
    double dynamic_viscosity = 1.8891414505869057e-5;
    double roughness = 0.00015;
    std::optional<double> required_velocity = 1000;
    std::optional<double> required_head_loss = 0.1;
    double diameter_tolerance_mm = 0.0001;
    int dimension_solver_max_iterations = 30;
    double friction_tolerance = 1e-10;
    int friction_factor_solver_max_iterations = 80;

    DuctSizerConfig make_config() const {
        return {size_increment, minimum_duct_dimension, maximum_duct_dimension,
                aspect_ratio, air_density, dynamic_viscosity, roughness,
                required_velocity, required_head_loss, diameter_tolerance_mm,
                dimension_solver_max_iterations, friction_tolerance, friction_factor_solver_max_iterations};
    }
};

} // namespace dsd::sizing::test
