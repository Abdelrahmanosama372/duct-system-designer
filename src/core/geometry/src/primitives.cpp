#include "geometry/primitives.hpp"
#include <cmath>
#include <stdexcept>

namespace dsd::geometry {

Point::Point(double x_value, double y_value, double z_value)
    : x(x_value), y(y_value), z(z_value) {
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) {
        throw std::invalid_argument("Point coordinates must be finite");
    }
}
double Point::distance(const Point& other) const {
    return std::hypot(x - other.x, y - other.y, z - other.z);
}

Transform2D::Transform2D(Point origin_value, double rotation)
    : origin(origin_value), angle(rotation) {
    if (!std::isfinite(angle)) throw std::invalid_argument("Rotation must be finite");
}
Point Transform2D::apply(const Point& point) const {
    const double c = std::cos(angle), s = std::sin(angle);
    return {origin.x + c * point.x - s * point.y,
            origin.y + s * point.x + c * point.y};
}
Transform2D Transform2D::translated_local(double dx, double dy) const {
    return {apply({dx, dy}), angle};
}
Transform2D Transform2D::rotated_local(double rotation) const {
    return {origin, angle + rotation};
}
Transform2D Transform2D::composed(const Transform2D& other) const {
    return {apply(other.origin), angle + other.angle};
}

Line::Line(Point from, Point to, LineType type, int identifier)
    : start(from), end(to), line_type(type), id(identifier) {}
double Line::length() const { return start.distance(end); }
double Line::direction() const { return std::atan2(end.y - start.y, end.x - start.x); }
Line Line::transformed(const Transform2D& transform) const {
    return {transform.apply(start), transform.apply(end), line_type, id};
}

Arc::Arc(Point from, Point origin, double sweep, LineType type, int identifier)
    : start(from), center(origin), angle(sweep), line_type(type), id(identifier) {
    if (!std::isfinite(angle)) throw std::invalid_argument("Arc sweep must be finite");
}
Point Arc::end() const {
    const double dx = start.x - center.x, dy = start.y - center.y;
    return {center.x + dx * std::cos(angle) - dy * std::sin(angle),
            center.y + dx * std::sin(angle) + dy * std::cos(angle)};
}
Arc Arc::transformed(const Transform2D& transform) const {
    return {transform.apply(start), transform.apply(center), angle, line_type, id};
}

} // namespace dsd::geometry
