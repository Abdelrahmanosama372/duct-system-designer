#pragma once

#include "geometry/primitives.hpp"
#include <optional>

namespace dsd::geometry {

enum class ViewType { Plan };

struct PlacementContext {
    Transform2D transform;
    ViewType view_type = ViewType::Plan;
    double elevation = 0;
    std::optional<double> length = std::nullopt; // Final connected duct length, mm
};

} // namespace dsd::geometry
