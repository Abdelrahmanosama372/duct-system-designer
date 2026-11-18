#include "application/network_sizing_configuration.hpp"
#include <stdexcept>

namespace dsd::application {
sizing::DuctSizerConfig NetworkSizingConfiguration::to_sizer_config() const {
    if (method != NetworkSizingMethod::EqualFriction &&
        method != NetworkSizingMethod::EqualVelocity)
        throw std::invalid_argument("Unsupported network sizing method");
    return {increment_mm,
            minimum_mm,
            maximum_mm,
            aspect_ratio,
            air_density_kg_m3,
            dynamic_viscosity_pa_s,
            roughness_mm / 1000,
            method == NetworkSizingMethod::EqualVelocity ? std::optional{velocity_fpm}
                                                         : std::nullopt,
            method == NetworkSizingMethod::EqualFriction ? std::optional{head_loss_in_wg_per_100ft}
                                                         : std::nullopt};
}
} // namespace dsd::application
