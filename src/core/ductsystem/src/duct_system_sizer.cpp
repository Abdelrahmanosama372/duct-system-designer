#include "ductsystem/duct_system_sizer.hpp"
#include "ductsystem/models/rectangular/rectangular_duct.hpp"
#include "ductsystem/models/rectangular/rectangular_elbow.hpp"
#include "ductsystem/models/rectangular/rectangular_endcap.hpp"
#include "ductsystem/models/rectangular/rectangular_reducer.hpp"
#include "ductsystem/models/rectangular/rectangular_shoe_takeoff.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace dsd::ductsystem {
namespace {
const RectangularDuct *upstream(const SheetMetalNode &node) {
    auto parent = node.parent().lock();
    while (parent) {
        if (const auto *duct = dynamic_cast<const RectangularDuct *>(&parent->component()))
            return duct;
        parent = parent->parent().lock();
    }
    return nullptr;
}
const RectangularDuct &downstream(const SheetMetalNode &node) {
    if (node.children().size() != 1)
        throw std::invalid_argument("Fitting requires one downstream duct");
    const auto *duct = dynamic_cast<const RectangularDuct *>(&node.children().front()->component());
    if (!duct)
        throw std::invalid_argument("Fitting must connect directly to a duct");
    return *duct;
}
void require_dimensions(const RectangularSheetMetalComponent &c) {
    if (!c.width || !c.height || !std::isfinite(*c.width) || !std::isfinite(*c.height) ||
        *c.width <= 0 || *c.height <= 0)
        throw std::invalid_argument("Component " + std::to_string(c.id) +
                                    " requires valid dimensions");
}
} // namespace
void DuctSystemSizer::size_ducts(DuctSystem &system, const sizing::DuctSizer &sizer) const {
    for (auto *node : system.preorder_traversal()) {
        auto *duct = dynamic_cast<RectangularDuct *>(&node->component());
        if (!duct)
            continue;
        if (!duct->width || !duct->height) {
            const auto *parent = upstream(*node);
            const auto size = sizer.size_duct(
                sizing::SizingContext{duct->flow, parent ? parent->width : std::nullopt});
            duct->width = size.width;
            duct->height = size.height;
        }
        require_dimensions(*duct);
    }
}
void DuctSystemSizer::insert_reducers(DuctSystem &system) const {
    const auto nodes = system.preorder_traversal();
    int next = 0;
    for (const auto *node : nodes)
        next = std::max(next, node->component().id + 1);
    std::vector<std::pair<SheetMetalNode *, SheetMetalNode *>> edges;
    for (auto *node : nodes) {
        const auto *parent = dynamic_cast<const RectangularDuct *>(&node->component());
        if (!parent)
            continue;
        require_dimensions(*parent);
        for (const auto &child : node->children()) {
            if (const auto *duct = dynamic_cast<const RectangularDuct *>(&child->component())) {
                require_dimensions(*duct);
                if (parent->width != duct->width || parent->height != duct->height)
                    edges.emplace_back(node, child.get());
            }
        }
    }
    for (const auto &[parent, child] : edges) {
        const auto &duct = child->component();
        parent->insert_between(
            *child, std::make_shared<SheetMetalNode>(std::make_unique<RectangularReducer>(
                        next++, duct.flow, duct.pressure_level, duct.material)));
    }
}
void DuctSystemSizer::size_fittings(DuctSystem &system) const {
    for (auto *node : system.preorder_traversal()) {
        if (dynamic_cast<RectangularDuct *>(&node->component()))
            continue;
        auto &fitting = dynamic_cast<RectangularSheetMetalComponent &>(node->component());
        const auto *parent = upstream(*node);
        if (!parent)
            throw std::invalid_argument("Fitting requires an upstream duct");
        require_dimensions(*parent);
        fitting.width = parent->width;
        fitting.height = parent->height;
        if (auto *elbow = dynamic_cast<RectangularElbow *>(&fitting)) {
            if (elbow->orientation == ElbowOrientation::Horizontal) {
                const auto &child = downstream(*node);
                if (child.width != parent->width || child.height != parent->height)
                    throw std::invalid_argument(
                        "Elbow " + std::to_string(elbow->id) +
                        ": upstream and downstream duct dimensions must match");
            }
        } else if (auto *shoe = dynamic_cast<RectangularShoeTakeoff *>(&fitting)) {
            const auto &child = downstream(*node);
            require_dimensions(child);
            shoe->width = child.width;
            shoe->height = child.height;
            shoe->takeoff_length = system.config().fittings.takeoff_length;
        } else if (auto *reducer = dynamic_cast<RectangularReducer *>(&fitting)) {
            const auto &child = downstream(*node);
            require_dimensions(child);
            reducer->inlet_width = parent->width;
            reducer->inlet_height = parent->height;
            reducer->outlet_width = child.width;
            reducer->outlet_height = child.height;
            reducer->length = system.config().fittings.reducer_length;
        } else if (!dynamic_cast<RectangularEndcap *>(&fitting)) {
            throw std::invalid_argument("Unsupported fitting during sizing");
        }
    }
}
} // namespace dsd::ductsystem
