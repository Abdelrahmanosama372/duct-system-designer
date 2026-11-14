#include "ductsystem/visitors/sheet_metal_component_visitor.hpp"
#include "ductsystem/models/rectangular/rectangular_duct.hpp"
#include "ductsystem/models/rectangular/rectangular_elbow.hpp"
#include "ductsystem/models/rectangular/rectangular_shoe_takeoff.hpp"
#include "ductsystem/models/rectangular/rectangular_reducer.hpp"
#include "ductsystem/models/rectangular/rectangular_endcap.hpp"
#include <stdexcept>

namespace dsd::ductsystem {

void SheetMetalComponentVisitor::visit(SheetMetalComponent& component) {
    component.accept(*this);
}

void SheetMetalComponent::accept(SheetMetalComponentVisitor&) {
    throw std::invalid_argument("Unsupported sheet-metal component");
}

void RectangularDuct::accept(SheetMetalComponentVisitor& visitor) {
    visitor.visit_rectangular_duct(*this);
}
void RectangularElbow::accept(SheetMetalComponentVisitor& visitor) {
    visitor.visit_rectangular_elbow(*this);
}
void RectangularShoeTakeoff::accept(SheetMetalComponentVisitor& visitor) {
    visitor.visit_rectangular_shoetakeoff(*this);
}
void RectangularReducer::accept(SheetMetalComponentVisitor& visitor) {
    visitor.visit_rectangular_reducer(*this);
}
void RectangularEndcap::accept(SheetMetalComponentVisitor& visitor) {
    visitor.visit_rectangular_endcap(*this);
}

} // namespace dsd::ductsystem
