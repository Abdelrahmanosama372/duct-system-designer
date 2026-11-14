#include "geometry/source_geometry_repository.hpp"
#include <stdexcept>
#include <string>
#include <utility>

namespace dsd::geometry {

SourceGeometryRepository::SourceGeometryRepository(std::map<int, Line> lines)
    : lines_(std::move(lines)) {
    for (const auto& [id, line] : lines_) {
        if (id != line.id) throw std::invalid_argument("Repository keys must match processed line IDs");
    }
}
const Line& SourceGeometryRepository::get_line(int id) const {
    const auto found = lines_.find(id);
    if (found == lines_.end()) {
        throw std::invalid_argument("Missing processed source line " + std::to_string(id));
    }
    return found->second;
}

} // namespace dsd::geometry
