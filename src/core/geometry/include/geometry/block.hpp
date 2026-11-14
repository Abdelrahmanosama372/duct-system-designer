#pragma once

#include "geometry/primitives.hpp"
#include <string>

namespace dsd::geometry {

struct Bounds2D {
    Point minimum;
    Point maximum;
};

struct Block {
    std::string source_id;
    std::wstring name; // Original block definition name
    Point insertion;
    Bounds2D bounds;     // Drawing bounds, independent of equipment metadata dimensions
    double rotation = 0; // Rotation in the XY plane, in radians.
};

} // namespace dsd::geometry
