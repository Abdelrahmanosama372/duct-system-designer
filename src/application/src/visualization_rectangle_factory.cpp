#include "application/visualization_rectangle_factory.hpp"
#include "network_geometry.hpp"

namespace dsd::application {
using namespace detail;
VisualizationRectangle
VisualizationRectangleFactory::create_for_segment(const std::string& network_id,
                                                  const LineInfo& info,
                                                  const NetworkProcessingOptions& options) const {
    validate(options);
    const auto& line = info.raw_line;
    const double length = distance(line.start, line.end);
    const double inset = options.visualization_end_inset_mm;
    if (length <= 2 * inset)
        throw std::invalid_argument("Duct segment is too short for its visualization");
    const double ux = (line.end.x - line.start.x) / length,
                 uy = (line.end.y - line.start.y) / length;
    const double half = options.visualization_width_mm / 2;
    const Point a{line.start.x + ux * inset, line.start.y + uy * inset};
    const Point b{line.end.x - ux * inset, line.end.y - uy * inset};
    return {network_id + ":VIS:" + info.id,
            info.id,
            NetworkItemType::Segment,
            {Point{a.x - uy * half, a.y + ux * half}, Point{b.x - uy * half, b.y + ux * half},
             Point{b.x + uy * half, b.y - ux * half}, Point{a.x + uy * half, a.y - ux * half}},
            info.metadata};
}
VisualizationRectangle
VisualizationRectangleFactory::create_for_block(const std::string& network_id,
                                                const BlockInfo& info, NetworkItemType type,
                                                const NetworkProcessingOptions& options) const {
    validate(options);
    if (type == NetworkItemType::Segment)
        throw std::invalid_argument("Block rectangle requires an equipment type");
    const auto& bounds = info.raw_block.bounds;
    if (bounds.minimum.x > bounds.maximum.x || bounds.minimum.y > bounds.maximum.y)
        throw std::invalid_argument("Invalid block bounds");
    const double offset = options.block_visualization_offset_mm;
    const double x0 = bounds.minimum.x - offset, y0 = bounds.minimum.y - offset;
    const double x1 = bounds.maximum.x + offset, y1 = bounds.maximum.y + offset;
    return {network_id + ":VIS:" + info.id,
            info.id,
            type,
            {Point{x0, y0}, Point{x1, y0}, Point{x1, y1}, Point{x0, y1}},
            info.metadata};
}
} // namespace dsd::application
