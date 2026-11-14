#include "sizing/models.hpp"

#include <cmath>
#include <stdexcept>

namespace dsd::sizing {

SizingContext::SizingContext(double flow_value, std::optional<double> width)
    : flow(flow_value), preferred_width(width) {
    if (!std::isfinite(flow) || flow <= 0) {
        throw std::invalid_argument("flow must be positive and finite (CFM)");
    }
    if (width && (!std::isfinite(*width) || *width <= 0)) {
        throw std::invalid_argument("preferred_width must be positive and finite (mm)");
    }
}

} // namespace dsd::sizing
