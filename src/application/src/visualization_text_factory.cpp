#include "application/visualization_text_factory.hpp"
#include "network_geometry.hpp"

namespace dsd::application {
namespace {
std::string value(const Metadata& metadata, const char* key, const char* fallback) {
    const auto item = metadata.find(key);
    return item == metadata.end() ? fallback : item->second;
}
std::vector<std::string> text_lines(const Metadata& metadata, NetworkItemType type,
                                    const VisualizationTextFields& fields) {
    std::vector<std::string> lines;
    if (fields.flow)
        lines.push_back(value(metadata, "flow", "0") + " cfm");
    if (fields.dimensions) {
        const auto fallback = type == NetworkItemType::AirTerminal ? "none" : "AC";
        lines.push_back(value(metadata, "width", fallback) + " x " +
                        value(metadata, "height", fallback));
    }
    if (fields.bod)
        lines.push_back("BOD 0");
    return lines;
}
const VisualizationRectangle& rectangle_for(const std::vector<VisualizationRectangle>& rectangles,
                                            const std::string& item_id) {
    const auto found =
        std::find_if(rectangles.begin(), rectangles.end(),
                     [&](const auto& rectangle) { return rectangle.item_id == item_id; });
    if (found == rectangles.end())
        throw std::invalid_argument("Visualization rectangle is missing for " + item_id);
    return *found;
}
VisualizationText layout(const DuctNetwork& network, const VisualizationRectangle& rectangle,
                         double rotation, std::vector<std::string> lines) {
    if (!std::isfinite(rotation))
        throw std::invalid_argument("Visualization rotation must be finite");
    geometry::Point center;
    for (const auto& corner : rectangle.corners) {
        center.x += corner.x / 4;
        center.y += corner.y / 4;
    }
    const double nx = -std::sin(rotation), ny = std::cos(rotation);
    double extent = 0;
    for (const auto& corner : rectangle.corners)
        extent = std::max(extent, (corner.x - center.x) * nx + (corner.y - center.y) * ny);
    const double offset = extent + network.options.visualization_text_offset_mm;
    return {network.id + ":TEXT:" + rectangle.item_id,
            rectangle.item_id,
            rectangle.type,
            {center.x + nx * offset, center.y + ny * offset},
            rotation,
            network.options.visualization_text_height_mm,
            std::move(lines)};
}
} // namespace
std::vector<VisualizationText>
VisualizationTextFactory::create(const DuctNetwork& network,
                                 const VisualizationTextFields& fields) const {
    detail::validate(network.options);
    std::vector<VisualizationText> result;
    if (!fields.flow && !fields.dimensions && !fields.bod)
        return result;
    for (const auto& segment : network.segmented_lines_infos) {
        const auto& rectangle = rectangle_for(network.segmented_lines_vis_rectangles, segment.id);
        const double rotation = std::atan2(segment.raw_line.end.y - segment.raw_line.start.y,
                                           segment.raw_line.end.x - segment.raw_line.start.x);
        result.push_back(layout(network, rectangle, rotation,
                                text_lines(segment.metadata, NetworkItemType::Segment, fields)));
    }
    for (const auto& terminal : network.air_terminal_blocks_infos) {
        const auto& rectangle =
            rectangle_for(network.air_terminals_blocks_vis_rectangles, terminal.id);
        result.push_back(
            layout(network, rectangle, terminal.raw_block.rotation,
                   text_lines(terminal.metadata, NetworkItemType::AirTerminal, fields)));
    }
    return result;
}
} // namespace dsd::application
