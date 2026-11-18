#include "application/duct_annotations.hpp"
#include "application/registered_network.hpp"
#include "ductsystem/models/rectangular/rectangular_duct.hpp"
#include <cmath>
#include <iomanip>
#include <locale>
#include <numbers>
#include <sstream>
#include <stdexcept>

namespace dsd::application {
void AnnotationSettings::validate() const {
    if (!flow && !dimensions && !bod)
        throw std::invalid_argument("Select at least one annotation content option");
    if (!std::isfinite(text_height_mm) || text_height_mm <= 0 || !std::isfinite(offset_mm) ||
        offset_mm < 0)
        throw std::invalid_argument(
            "Annotation height must be positive and offset non-negative and finite");
    if (placement != AnnotationPlacement::Above && placement != AnnotationPlacement::Below &&
        placement != AnnotationPlacement::Centered)
        throw std::invalid_argument("Invalid annotation placement");
    if (layer.empty() || text_style.empty())
        throw std::invalid_argument("Select a drawing layer and text style for annotations");
}
namespace {
std::string number(double value) {
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::setprecision(12) << value;
    return stream.str();
}
} // namespace
AnnotationGenerationResult DuctAnnotationFactory::create(const NetworkGenerationResult& generation,
                                                         const AnnotationSettings& settings) const {
    settings.validate();
    if (!generation.system || generation.geometry.empty())
        throw std::invalid_argument("Generate duct geometry before generating annotations");
    AnnotationGenerationResult result{generation.network_id,
                                      generation.source_revision,
                                      generation.geometry_revision,
                                      settings,
                                      {}};
    std::map<int, std::string> item_ids;
    for (const auto& info : generation.topology.ducts)
        item_ids.emplace(info.component_id, info.id);
    for (const auto* node : generation.system->preorder_traversal()) {
        const auto* duct = dynamic_cast<const ductsystem::RectangularDuct*>(&node->component());
        if (!duct)
            continue;
        if (!duct->width || !duct->height || *duct->width <= 0 || *duct->height <= 0)
            throw std::invalid_argument("Annotations require sized ducts");
        const auto& skeleton = generation.geometry.at(duct->id).layout_skeleton;
        if (skeleton.size() != 1)
            throw std::invalid_argument("Duct requires one final centerline");
        const auto& line = skeleton.front();
        const double dx = line.end.x - line.start.x, dy = line.end.y - line.start.y;
        const double length = std::hypot(dx, dy);
        if (!std::isfinite(length) || length <= 0)
            throw std::invalid_argument("Annotation duct centerline has no usable length");
        const double angle = std::atan2(dy, dx);
        const bool flipped = angle > std::numbers::pi / 2 || angle < -std::numbers::pi / 2;
        double rotation = angle;
        if (flipped)
            rotation += angle > 0 ? -std::numbers::pi : std::numbers::pi;
        double distance = settings.offset_mm;
        auto attachment = AnnotationAttachment::MiddleCenter;
        if (settings.placement != AnnotationPlacement::Centered) {
            distance += *duct->width / 2;
            const bool above = settings.placement == AnnotationPlacement::Above;
            if (!above)
                distance = -distance;
            attachment = above != flipped ? AnnotationAttachment::BottomCenter
                                          : AnnotationAttachment::TopCenter;
        }
        geometry::Point anchor{line.start.x / 2 + line.end.x / 2 - dy / length * distance,
                               line.start.y / 2 + line.end.y / 2 + dx / length * distance};
        std::vector<std::string> lines;
        if (settings.flow)
            lines.push_back(number(duct->flow) + " cfm");
        if (settings.dimensions)
            lines.push_back(number(*duct->width) + " x " + number(*duct->height));
        if (settings.bod)
            lines.push_back("BOD 0");
        result.annotations.push_back(
            {item_ids.at(duct->id), duct->id, anchor, rotation, attachment, std::move(lines)});
    }
    return result;
}
} // namespace dsd::application
