#include "geometry/rectangular_geometry_builder.hpp"
#include <cmath>
#include <numbers>
#include <string>
#include <utility>

namespace dsd::geometry {
namespace {

double dimension(const ductsystem::SheetMetalComponent& component,
                 std::optional<double> value, const char* name) {
    if (!value || !std::isfinite(*value) || *value <= 0) {
        throw GeometryConstructionError("Component " + std::to_string(component.id) +
                                        " requires positive finite " + name);
    }
    return *value;
}
std::vector<Primitive2D> polygon(const std::vector<Point>& points) {
    std::vector<Primitive2D> edges;
    for (std::size_t i = 0; i < points.size(); ++i) {
        edges.emplace_back(Line{points[i], points[(i + 1) % points.size()]});
    }
    return edges;
}
Line skeleton(Point start, Point end) { return {start, end, LineType::CenterLine}; }

} // namespace

RectangularGeometricRepresentationBuilder::RectangularGeometricRepresentationBuilder(
    const ductsystem::FittingSizingConfig& config) : config_(config) {}

GeometricRepresentation RectangularGeometricRepresentationBuilder::place(
    int id, std::vector<double> key, const PlacementContext& context,
    const std::function<GeometricRepresentation()>& factory) {
    if (context.view_type != ViewType::Plan) {
        throw GeometryConstructionError("Only plan geometry is supported");
    }
    if (!keys_.contains(id) || keys_.at(id) != key) {
        auto normalized = std::make_shared<const GeometricRepresentation>(factory());
        normalized_.insert_or_assign(id, std::move(normalized));
        keys_.insert_or_assign(id, std::move(key));
    }
    return normalized_.at(id)->transformed(context.transform);
}

GeometricRepresentation RectangularGeometricRepresentationBuilder::build_duct(
    const ductsystem::RectangularDuct& c, const PlacementContext& context) {
    const double width = dimension(c, c.width, "width");
    const double height = dimension(c, c.height, "height");
    const double length = dimension(c, c.length, "length");
    return place(c.id, {1, width, height, length}, context, [&] {
        return GeometricRepresentation{c.id,
            polygon({{0, -width / 2}, {length, -width / 2}, {length, width / 2}, {0, width / 2}}),
            {skeleton({0, 0}, {length, 0})}};
    });
}

GeometricRepresentation RectangularGeometricRepresentationBuilder::build_elbow(
    const ductsystem::RectangularElbow& c, const PlacementContext& context) {
    const double width = dimension(c, c.width, "width");
    const double height = dimension(c, c.height, "height");
    const double angle = c.angle;
    if (!std::isfinite(angle) || angle == 0 || std::abs(angle) >= std::numbers::pi) {
        throw GeometryConstructionError("Plan elbow requires a turn strictly between 0 and 180 degrees");
    }
    return place(c.id, {2, width, height, angle}, context, [&] {
        const double inside = config_.elbow_inside_radius_ratio * width;
        const double radius = inside + width / 2;
        const double setback = radius * std::tan(std::abs(angle) / 2);
        const double side = angle > 0 ? 1 : -1;
        const Point center{-setback, side * radius};
        const Point inner_start{-setback, side * width / 2};
        const Point outer_start{-setback, -side * width / 2};
        const auto end = [&](double radial) {
            return Point{center.x + side * radial * std::sin(angle),
                         center.y - side * radial * std::cos(angle)};
        };
        return GeometricRepresentation{c.id,
            {Arc{inner_start, center, angle}, Arc{outer_start, center, angle},
             Line{inner_start, outer_start}, Line{end(inside), end(inside + width)}},
            {skeleton({-setback, 0}, {0, 0}),
             skeleton({0, 0}, {setback * std::cos(angle), setback * std::sin(angle)})}};
    });
}

GeometricRepresentation RectangularGeometricRepresentationBuilder::build_shoetakeoff(
    const ductsystem::RectangularShoeTakeoff& c, const PlacementContext& context) {
    const double width = dimension(c, c.width, "width");
    const double height = dimension(c, c.height, "height");
    const double length = dimension(c, c.takeoff_length, "takeoff_length");
    if (c.side != ductsystem::TakeoffSide::Left && c.side != ductsystem::TakeoffSide::Right) {
        throw GeometryConstructionError("Shoe side must be left or right");
    }
    return place(c.id, {3, width, height, length, static_cast<double>(c.side)}, context, [&] {
        const double low = -width / 2 - (c.side == ductsystem::TakeoffSide::Left ? length : 0);
        const double high = width / 2 + (c.side == ductsystem::TakeoffSide::Right ? length : 0);
        return GeometricRepresentation{c.id,
            polygon({{0, low}, {length, -width / 2}, {length, width / 2}, {0, high}}),
            {skeleton({0, 0}, {length, 0})}};
    });
}

GeometricRepresentation RectangularGeometricRepresentationBuilder::build_reducer(
    const ductsystem::RectangularReducer& c, const PlacementContext& context) {
    const double inlet = dimension(c, c.inlet_width, "inlet_width");
    const double outlet = dimension(c, c.outlet_width, "outlet_width");
    const double inlet_height = dimension(c, c.inlet_height, "inlet_height");
    const double outlet_height = dimension(c, c.outlet_height, "outlet_height");
    const double length = dimension(c, c.length, "length");
    return place(c.id, {4, inlet, outlet, inlet_height, outlet_height, length}, context, [&] {
        return GeometricRepresentation{c.id,
            polygon({{0, -inlet / 2}, {length, -outlet / 2}, {length, outlet / 2}, {0, inlet / 2}}),
            {skeleton({0, 0}, {length, 0})}};
    });
}

GeometricRepresentation RectangularGeometricRepresentationBuilder::build_endcap(
    const ductsystem::RectangularEndcap& c, const PlacementContext& context) {
    const double width = dimension(c, c.width, "width");
    const double height = dimension(c, c.height, "height");
    return place(c.id, {5, width, height}, context, [&] {
        return GeometricRepresentation{c.id, {Line{{0, -width / 2}, {0, width / 2}}}, {}};
    });
}

} // namespace dsd::geometry
