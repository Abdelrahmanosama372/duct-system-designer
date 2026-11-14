#pragma once

#include "geometry/primitives.hpp"
#include <map>
#include <vector>

namespace dsd::geometry {

struct GeometricRepresentation {
    int component_id;
    std::vector<Primitive2D> primitives;
    std::vector<Line> layout_skeleton;

    [[nodiscard]] GeometricRepresentation transformed(const Transform2D& transform) const;
};

using GeometryRepresentations = std::map<int, GeometricRepresentation>;

} // namespace dsd::geometry
