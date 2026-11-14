#pragma once

#include <variant>

namespace dsd::geometry {

// Geometry required by plan drawings, in mm and signed radians.
inline constexpr double PLACEMENT_EPS_MM = 1.0;
enum class LineType { Normal, CenterLine, Hidden };

struct Point {
    double x;
    double y;
    double z;
    Point(double x = 0, double y = 0, double z = 0);
    [[nodiscard]] double distance(const Point& other) const;
};

struct Transform2D {
    Point origin;
    double angle;
    Transform2D(Point origin = {}, double angle = 0);
    [[nodiscard]] Point apply(const Point& point) const;
    [[nodiscard]] Transform2D translated_local(double dx, double dy) const;
    [[nodiscard]] Transform2D rotated_local(double angle) const;
    [[nodiscard]] Transform2D composed(const Transform2D& other) const;
};

struct Line {
    Point start;
    Point end;
    LineType line_type;
    int id;
    Line(Point start, Point end, LineType line_type = LineType::Normal, int id = -1);
    [[nodiscard]] double length() const;
    [[nodiscard]] double direction() const;
    [[nodiscard]] Line transformed(const Transform2D& transform) const;
};

struct Arc {
    Point start;
    Point center;
    double angle;
    LineType line_type;
    int id;
    Arc(Point start, Point center, double angle,
        LineType line_type = LineType::Normal, int id = -1);
    [[nodiscard]] Point end() const;
    [[nodiscard]] Arc transformed(const Transform2D& transform) const;
};

using Primitive2D = std::variant<Line, Arc>;

} // namespace dsd::geometry
