#pragma once
#include "sizing/duct_sizer_config.hpp"

namespace dsd::application {
enum class NetworkSizingMethod { EqualFriction, EqualVelocity };

// Editable inputs in UI units; conversion and validation happen at the boundary.
struct NetworkSizingConfiguration {
    NetworkSizingMethod method = NetworkSizingMethod::EqualFriction;
    int minimum_mm = 100;
    int maximum_mm = 2000;
    int increment_mm = 25;
    double aspect_ratio = 2;
    double head_loss_in_wg_per_100ft = 0.1;
    double velocity_fpm = 1000;
    double roughness_mm = 0.15;
    double air_density_kg_m3 = 1.1180887435024176;
    double dynamic_viscosity_pa_s = 1.8891414505869057e-5;

    [[nodiscard]] sizing::DuctSizerConfig to_sizer_config() const;
};
} // namespace dsd::application
