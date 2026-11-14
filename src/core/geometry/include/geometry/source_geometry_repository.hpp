#pragma once

#include "geometry/primitives.hpp"
#include <map>

namespace dsd::geometry {

// Owns processed source centerlines, keyed by duct component ID.
class SourceGeometryRepository {
public:
    explicit SourceGeometryRepository(std::map<int, Line> lines);
    [[nodiscard]] const Line& get_line(int component_id) const;

private:
    const std::map<int, Line> lines_;
};

} // namespace dsd::geometry
