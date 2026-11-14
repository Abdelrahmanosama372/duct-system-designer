#include "ductsystem/visitors/geometry_constructor.hpp"

namespace dsd::ductsystem {

GeometryConstructorVisitor::GeometryConstructorVisitor(
    geometry::PlacementContextManager& manager,
    geometry::RectangularGeometricRepresentationBuilder& builder)
    : context_manager_(manager), builder_(builder) {}

void GeometryConstructorVisitor::construct() {
    const auto nodes = context_manager_.tree().preorder_traversal();
    representations_.clear();
    for (auto* node : nodes) {
        if (!dynamic_cast<RectangularDuct*>(&node->component())) node->component().accept(*this);
    }
    context_manager_.set_fitting_layouts(representations_);
    std::map<int, geometry::PlacementContext> contexts;
    // Check every duct connection before changing any duct lengths.
    for (auto* node : nodes) {
        if (dynamic_cast<RectangularDuct*>(&node->component())) {
            contexts.emplace(node->component().id, context_manager_.find_context(node->component()));
        }
    }
    for (auto* node : nodes) {
        if (auto* component = dynamic_cast<RectangularDuct*>(&node->component())) {
            duct(*component, contexts.at(component->id));
        }
    }
}

void GeometryConstructorVisitor::duct(
    RectangularDuct& component, const geometry::PlacementContext& context) {
    if (!context.length) {
        throw geometry::GeometryConstructionError("Duct placement requires a final length");
    }
    auto sized = component;
    sized.length = context.length;
    auto representation = builder_.build_duct(sized, context);
    component.length = context.length;
    representations_.insert_or_assign(component.id, std::move(representation));
}

void GeometryConstructorVisitor::visit_rectangular_duct(RectangularDuct& component) {
    duct(component, context_manager_.find_context(component));
}
void GeometryConstructorVisitor::visit_rectangular_elbow(RectangularElbow& component) {
    representations_.insert_or_assign(component.id,
        builder_.build_elbow(component, context_manager_.find_context(component)));
}
void GeometryConstructorVisitor::visit_rectangular_shoetakeoff(RectangularShoeTakeoff& component) {
    representations_.insert_or_assign(component.id,
        builder_.build_shoetakeoff(component, context_manager_.find_context(component)));
}
void GeometryConstructorVisitor::visit_rectangular_reducer(RectangularReducer& component) {
    representations_.insert_or_assign(component.id,
        builder_.build_reducer(component, context_manager_.find_context(component)));
}
void GeometryConstructorVisitor::visit_rectangular_endcap(RectangularEndcap& component) {
    representations_.insert_or_assign(component.id,
        builder_.build_endcap(component, context_manager_.find_context(component)));
}

} // namespace dsd::ductsystem
