#include "geometry/geometric_representation.hpp"

namespace dsd::geometry {

GeometricRepresentation GeometricRepresentation::transformed(const Transform2D& transform) const {
    GeometricRepresentation result{component_id, {}, {}};
    result.primitives.reserve(primitives.size());
    result.layout_skeleton.reserve(layout_skeleton.size());
    for (const auto& primitive : primitives) {
        result.primitives.push_back(std::visit([&transform](const auto& value) -> Primitive2D {
            return value.transformed(transform);
        }, primitive));
    }
    for (const auto& line : layout_skeleton) {
        result.layout_skeleton.push_back(line.transformed(transform));
    }
    return result;
}

} // namespace dsd::geometry
