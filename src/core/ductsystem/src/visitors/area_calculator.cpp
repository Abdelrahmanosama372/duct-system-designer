#include "ductsystem/visitors/area_calculator.hpp"
#include "ductsystem/models/rectangular/rectangular_duct.hpp"
#include "ductsystem/models/rectangular/rectangular_elbow.hpp"
#include "ductsystem/models/rectangular/rectangular_shoe_takeoff.hpp"
#include "ductsystem/models/rectangular/rectangular_reducer.hpp"
#include "ductsystem/models/rectangular/rectangular_endcap.hpp"
#include <cmath>
#include <numbers>
#include <optional>
#include <string>

namespace dsd::ductsystem {
namespace {

double dimension(const SheetMetalComponent& component, std::optional<double> value,
                 const char* name) {
    if (!value || !std::isfinite(*value) || *value <= 0) {
        throw AreaCalculationError("Component " + std::to_string(component.id) +
                                   " requires positive finite " + name);
    }
    return *value;
}

double square_metres(const SheetMetalComponent& component, double area_mm2) {
    if (!std::isfinite(area_mm2) || area_mm2 < 0) {
        throw AreaCalculationError("Invalid surface area for component " +
                                   std::to_string(component.id));
    }
    return area_mm2 / 1'000'000;
}

} // namespace

AreaCalculatorVisitor::AreaCalculatorVisitor(const FittingSizingConfig& config)
    : config_(config) {}

void AreaCalculatorVisitor::visit_rectangular_duct(RectangularDuct& component) {
    area_.reset();
    const double width = dimension(component, component.width, "width");
    const double height = dimension(component, component.height, "height");
    const double length = dimension(component, component.length, "length");
    area_ = square_metres(component, 2 * (width + height) * length);
}

void AreaCalculatorVisitor::visit_rectangular_elbow(RectangularElbow& component) {
    area_.reset();
    const double width = dimension(component, component.width, "width");
    const double height = dimension(component, component.height, "height");
    const double angle = std::abs(component.angle);
    if (!std::isfinite(angle) || angle == 0 || angle > std::numbers::pi) {
        throw AreaCalculationError("Elbow requires a nonzero angle between -pi and pi");
    }
    const double inside = *component.inside_radius(config_.elbow_inside_radius_ratio);
    const double outside = inside + width;
    // Two annular-sector cheeks and inner/outer curved faces.
    // Factoring outside^2 - inside^2 avoids cancellation for large radii.
    area_ = square_metres(component, angle * (inside + outside) * (width + height));
}

void AreaCalculatorVisitor::visit_rectangular_shoetakeoff(
    RectangularShoeTakeoff& component) {
    area_.reset();
    const double width = dimension(component, component.width, "width");
    const double height = dimension(component, component.height, "height");
    const double length = dimension(component, component.takeoff_length, "takeoff_length");
    const double opening = width + length;
    // Two trapezoidal cheeks, a straight side, and a 45-degree bevel side.
    area_ = square_metres(component, (width + opening) * length +
                             height * length * (1 + std::sqrt(2.0)));
}

void AreaCalculatorVisitor::visit_rectangular_reducer(RectangularReducer& component) {
    area_.reset();
    const double inlet_width = dimension(component, component.inlet_width, "inlet_width");
    const double inlet_height = dimension(component, component.inlet_height, "inlet_height");
    const double outlet_width = dimension(component, component.outlet_width, "outlet_width");
    const double outlet_height = dimension(component, component.outlet_height, "outlet_height");
    const double length = dimension(component, component.length, "length");
    // Centered ports slope through half the difference in each dimension.
    const double top_slant = std::hypot(length, (inlet_height - outlet_height) / 2);
    const double side_slant = std::hypot(length, (inlet_width - outlet_width) / 2);
    area_ = square_metres(component, (inlet_width + outlet_width) * top_slant +
                             (inlet_height + outlet_height) * side_slant);
}

void AreaCalculatorVisitor::visit_rectangular_endcap(RectangularEndcap& component) {
    area_.reset();
    const double width = dimension(component, component.width, "width");
    const double height = dimension(component, component.height, "height");
    area_ = square_metres(component, width * height);
}

double AreaCalculatorVisitor::area_m2() const {
    if (!area_) throw std::logic_error("No component area is available");
    return *area_;
}

} // namespace dsd::ductsystem
