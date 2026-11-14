#include "sizing/equal_velocity.hpp"

#include <cmath>

namespace dsd::sizing {

EqualVelocityDuctSizer::EqualVelocityDuctSizer(const DuctSizerConfig& config)
    : DuctSizer(config) {
    if (!config_.required_velocity) {
        throw std::invalid_argument("Equal velocity sizing requires required_velocity (FPM)");
    }
    velocity_m_s_ = *config_.required_velocity * FPM_TO_M_S;
}

DuctDimension EqualVelocityDuctSizer::size_duct(const SizingContext& context) const {
    const double area_mm2 = context.flow * CFM_TO_M3_S / velocity_m_s_ * 1e6;
    const int width = context.preferred_width
        ? normalize_preferred_width(*context.preferred_width)
        : normalize_root_width(std::sqrt(area_mm2 * config_.aspect_ratio));
    return select_dimensions(width, [area_mm2](int width) {
        return area_mm2 / width;
    });
}

} // namespace dsd::sizing
