#include "geometry/placement_context_manager.hpp"
#include "geometry/rectangular_geometry_builder.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

namespace dsd::geometry {
namespace {

struct Axis { double x; double y; double length; };
Axis axis(const Line& line) {
    const double dx = line.end.x - line.start.x, dy = line.end.y - line.start.y;
    const double length = std::hypot(dx, dy);
    if (!std::isfinite(length) || length <= PLACEMENT_EPS_MM) {
        throw PlacementError("Source line has no usable XY length");
    }
    return {dx / length, dy / length, length};
}
Point along(const Line& line, double distance) {
    const auto direction = axis(line);
    return {line.start.x + distance * direction.x, line.start.y + distance * direction.y};
}
double width_of(const ductsystem::RectangularSheetMetalComponent& c) {
    if (!c.width || !std::isfinite(*c.width) || *c.width <= 0) {
        throw PlacementError("Placement requires sized component " + std::to_string(c.id));
    }
    return *c.width;
}

} // namespace

PlacementContextManager::PlacementContextManager(ductsystem::DuctFittingTree& tree,
                                                 SourceGeometryRepository source)
    : tree_(tree), source_(std::move(source)) {}
void PlacementContextManager::set_fitting_layouts(const GeometryRepresentations& layouts) {
    layouts_ = layouts;
}
ductsystem::SheetMetalNode& PlacementContextManager::node_for(
    const ductsystem::SheetMetalComponent& component) const {
    for (auto* node : tree_.preorder_traversal()) {
        if (&node->component() == &component) return *node;
    }
    throw PlacementError("Component is not in this tree");
}
const Line& PlacementContextManager::source(const ductsystem::SheetMetalNode& node) const {
    if (!dynamic_cast<const ductsystem::RectangularDuct*>(&node.component())) {
        throw PlacementError("A source line is required for a duct node");
    }
    return source_.get_line(node.component().id);
}
std::shared_ptr<const ductsystem::SheetMetalNode> PlacementContextManager::upstream(
    const ductsystem::SheetMetalNode& node) const {
    auto current = node.parent().lock();
    while (current) {
        if (dynamic_cast<const ductsystem::RectangularDuct*>(&current->component())) return current;
        current = current->parent().lock();
    }
    throw PlacementError("Fitting has no upstream duct");
}

double PlacementContextManager::extension(const ductsystem::SheetMetalNode& node) const {
    const bool capped = std::any_of(node.children().begin(), node.children().end(), [](const auto& child) {
        return dynamic_cast<const ductsystem::RectangularEndcap*>(&child->component()) != nullptr;
    });
    if (!capped) return 0;
    double largest = 0;
    for (const auto& child : node.children()) {
        if (const auto* shoe = dynamic_cast<const ductsystem::RectangularShoeTakeoff*>(&child->component())) {
            largest = std::max(largest, width_of(*shoe));
        }
    }
    return 2 * largest;
}

PlacementContext PlacementContextManager::find_context(
    const ductsystem::SheetMetalComponent& component) const {
    auto& node = node_for(component);
    if (dynamic_cast<const ductsystem::RectangularDuct*>(&component)) return duct_context(node);
    const auto parent = upstream(node);
    const auto& line = source(*parent);
    const auto direction = axis(line);
    double angle = std::atan2(direction.y, direction.x);
    Point origin;
    if (dynamic_cast<const ductsystem::RectangularElbow*>(&component)) {
        origin = along(line, direction.length);
    } else if (dynamic_cast<const ductsystem::RectangularReducer*>(&component)) {
        const auto& duct = dynamic_cast<const ductsystem::RectangularDuct&>(parent->component());
        origin = along(line, direction.length + 1.5 * width_of(duct));
    } else if (const auto* shoe = dynamic_cast<const ductsystem::RectangularShoeTakeoff*>(&component)) {
        if (node.children().size() != 1 || !shoe->branch_duct_id ||
            node.children().front()->component().id != *shoe->branch_duct_id) {
            throw PlacementError("Shoe requires its sole branch duct child");
        }
        const auto& duct = dynamic_cast<const ductsystem::RectangularDuct&>(parent->component());
        const auto& branch = source(*node.children().front());
        const auto branch_direction = axis(branch);
        origin = along(branch, width_of(duct) / 2);
        angle = std::atan2(branch_direction.y, branch_direction.x);
    } else if (dynamic_cast<const ductsystem::RectangularEndcap*>(&component)) {
        origin = along(line, direction.length + extension(*parent));
    } else {
        throw PlacementError("Unsupported fitting");
    }
    return {Transform2D{origin, angle}};
}

std::pair<double, double> PlacementContextManager::skeleton_interval(
    const ductsystem::SheetMetalNode& fitting, const Line& line) const {
    const auto found = layouts_.find(fitting.component().id);
    if (found == layouts_.end()) {
        throw PlacementError("Construct fitting geometry before connecting ducts");
    }
    const auto direction = axis(line);
    double low = std::numeric_limits<double>::infinity();
    double high = -std::numeric_limits<double>::infinity();
    for (const auto& segment : found->second.layout_skeleton) {
        const auto on_axis = [&](const Point& p) {
            return std::abs((p.x - line.start.x) * direction.y -
                            (p.y - line.start.y) * direction.x) <= PLACEMENT_EPS_MM;
        };
        if (on_axis(segment.start) && on_axis(segment.end)) {
            for (const auto& p : {segment.start, segment.end}) {
                const double distance = (p.x - line.start.x) * direction.x +
                                        (p.y - line.start.y) * direction.y;
                low = std::min(low, distance);
                high = std::max(high, distance);
            }
        }
    }
    if (!std::isfinite(low) || !std::isfinite(high)) {
        throw PlacementError("Fitting has no skeleton along source duct");
    }
    return {low, high};
}

PlacementContext PlacementContextManager::duct_context(const ductsystem::SheetMetalNode& node) const {
    const auto& line = source(node);
    const auto direction = axis(line);
    double start = 0, end = direction.length + extension(node);
    if (const auto parent = node.parent().lock();
        parent && !dynamic_cast<const ductsystem::RectangularDuct*>(&parent->component())) {
        start = skeleton_interval(*parent, line).second;
    }
    for (const auto& child : node.children()) {
        if (dynamic_cast<const ductsystem::RectangularElbow*>(&child->component())) {
            end = std::min(end, skeleton_interval(*child, line).first);
        } else if (dynamic_cast<const ductsystem::RectangularReducer*>(&child->component())) {
            end = skeleton_interval(*child, line).first;
        }
    }
    if (!std::isfinite(end - start) || end - start <= PLACEMENT_EPS_MM) {
        throw PlacementError("Fittings leave no positive length for duct " +
                             std::to_string(node.component().id));
    }
    return {Transform2D{along(line, start), std::atan2(direction.y, direction.x)},
            ViewType::Plan, 0, end - start};
}

} // namespace dsd::geometry
