#include "ductsystem/models/rectangular/rectangular_duct.hpp"
#include "ductsystem/models/rectangular/rectangular_elbow.hpp"
#include "ductsystem/models/rectangular/rectangular_reducer.hpp"
#include "ductsystem/models/rectangular/rectangular_shoe_takeoff.hpp"
#include "validation.hpp"
#include <numbers>

namespace dsd::ductsystem {

RectangularSheetMetalComponent::RectangularSheetMetalComponent(
    int id, double flow, PressureLevel pressure, Material material,
    std::optional<double> width_value, std::optional<double> height_value)
    : SheetMetalComponent(id, flow, pressure, material),
      width(width_value), height(height_value) {
    detail::dimension(width, "width");
    detail::dimension(height, "height");
}

RectangularDuct::RectangularDuct(
    int id, double flow, PressureLevel pressure, Material material,
    std::optional<double> width, std::optional<double> height,
    std::optional<double> length_value, std::optional<int> reference)
    : RectangularSheetMetalComponent(id, flow, pressure, material, width, height),
      length(length_value), ref_line_id(reference) {
    if (width.has_value() != height.has_value()) {
        throw std::invalid_argument("Duct width and height must be supplied together");
    }
    detail::dimension(length, "length");
}

RectangularElbow::RectangularElbow(
    int id, double flow, PressureLevel pressure, Material material, double angle_value,
    std::optional<double> width, std::optional<double> height, ElbowOrientation orientation_value)
    : RectangularDuctFitting(id, flow, pressure, material, width, height), angle(angle_value),
      orientation(orientation_value) {
    if (!std::isfinite(angle) || std::abs(angle) <= 5 * std::numbers::pi / 180 ||
        std::abs(angle) >= 175 * std::numbers::pi / 180) {
        throw std::invalid_argument("Elbow angle magnitude must be strictly between 5 and 175 degrees");
    }
    if (orientation != ElbowOrientation::Horizontal && orientation != ElbowOrientation::Downward)
        throw std::invalid_argument("Invalid elbow orientation");
    if (orientation == ElbowOrientation::Downward && std::abs(angle - std::numbers::pi / 2) > 1e-10)
        throw std::invalid_argument("Downward elbows require a 90-degree bend");
}

std::optional<double> RectangularElbow::inside_radius(double ratio) const {
    detail::positive(ratio, "inside-radius ratio");
    const auto bend_dimension = orientation == ElbowOrientation::Downward ? height : width;
    if (!bend_dimension) return std::nullopt;
    return ratio * *bend_dimension;
}

std::optional<double> RectangularElbow::centerline_radius(double ratio) const {
    const auto inside = inside_radius(ratio);
    if (!inside) return std::nullopt;
    return *inside + *(orientation == ElbowOrientation::Downward ? height : width) / 2;
}

RectangularReducer::RectangularReducer(
    int id, double flow, PressureLevel pressure, Material material,
    std::optional<double> width, std::optional<double> height,
    std::optional<double> inlet_width_value, std::optional<double> inlet_height_value,
    std::optional<double> outlet_width_value, std::optional<double> outlet_height_value,
    std::optional<double> length_value)
    : RectangularDuctFitting(id, flow, pressure, material, width, height),
      inlet_width(inlet_width_value), inlet_height(inlet_height_value),
      outlet_width(outlet_width_value), outlet_height(outlet_height_value), length(length_value) {
    detail::dimension(inlet_width, "inlet_width");
    detail::dimension(inlet_height, "inlet_height");
    detail::dimension(outlet_width, "outlet_width");
    detail::dimension(outlet_height, "outlet_height");
    detail::dimension(length, "length");
}

RectangularShoeTakeoff::RectangularShoeTakeoff(
    int id, double flow, PressureLevel pressure, Material material, TakeoffSide side_value,
    std::optional<double> width, std::optional<double> height,
    std::optional<int> branch, std::optional<double> length)
    : RectangularDuctFitting(id, flow, pressure, material, width, height),
      side(side_value), branch_duct_id(branch), takeoff_length(length) {
    detail::validate(side);
    detail::dimension(takeoff_length, "takeoff_length");
}

std::optional<double> RectangularShoeTakeoff::width1() const {
    if (!width || !takeoff_length) return std::nullopt;
    return *width + *takeoff_length;
}

} // namespace dsd::ductsystem
