#pragma once
#include "application/duct_network.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace dsd::application::detail {
using geometry::Point;
constexpr double epsilon = 1e-8;
struct Edge {
    Point a, b;
    SourcePiece source;
};

inline double distance(const Point& a, const Point& b) { return std::hypot(a.x - b.x, a.y - b.y); }
inline void validate(const NetworkProcessingOptions& o) {
    for (double value : {o.connection_tolerance_mm, o.angle_threshold_degrees,
                         o.visualization_width_mm, o.visualization_end_inset_mm,
                         o.block_visualization_offset_mm, o.minimum_segment_length_mm}) {
        if (!std::isfinite(value) || value <= 0)
            throw std::invalid_argument("Network options must be positive and finite");
    }
    if (o.angle_threshold_degrees > 180)
        throw std::invalid_argument("Invalid angle threshold");
}
inline std::vector<Edge> edges(const std::vector<SourcePath>& paths) {
    std::vector<Edge> result;
    for (const auto& path : paths) {
        if (path.vertices.size() < 2)
            throw std::invalid_argument("Source path requires at least two vertices");
        const std::size_t count = path.vertices.size() - 1 + (path.closed ? 1 : 0);
        for (std::size_t i = 0; i < count; ++i) {
            const auto& a = path.vertices[i];
            const auto& b = path.vertices[(i + 1) % path.vertices.size()];
            if (distance(a, b) <= epsilon)
                continue;
            result.push_back({Point{a.x, a.y}, Point{b.x, b.y}, {path.source_id, i}});
        }
    }
    return result;
}
inline std::optional<double> projection(const Point& p, const Edge& edge, double tolerance) {
    const double dx = edge.b.x - edge.a.x, dy = edge.b.y - edge.a.y;
    const double length2 = dx * dx + dy * dy;
    const double t =
        std::clamp(((p.x - edge.a.x) * dx + (p.y - edge.a.y) * dy) / length2, 0.0, 1.0);
    if (distance(p, {edge.a.x + t * dx, edge.a.y + t * dy}) > tolerance)
        return std::nullopt;
    return t;
}
inline Point point_at(const Edge& edge, double t) {
    return {edge.a.x + t * (edge.b.x - edge.a.x), edge.a.y + t * (edge.b.y - edge.a.y)};
}

struct EdgeJunction {
    double first_parameter;
    double second_parameter;
};

// Parameters on both finite edges, including endpoint gaps within tolerance.
// Parallel/collinear edges have no unique crossing; their endpoint connections
// continue to be handled separately.
inline std::optional<EdgeJunction> junction(const Edge& a, const Edge& b, double tolerance) {
    const double first_length = distance(a.a, a.b), second_length = distance(b.a, b.b);
    const double rx = (a.b.x - a.a.x) / first_length;
    const double ry = (a.b.y - a.a.y) / first_length;
    const double sx = (b.b.x - b.a.x) / second_length;
    const double sy = (b.b.y - b.a.y) / second_length;
    const double determinant = rx * sy - ry * sx;
    if (std::abs(determinant) <= 16 * std::numeric_limits<double>::epsilon())
        return std::nullopt;
    const double dx = b.a.x - a.a.x, dy = b.a.y - a.a.y;
    const double first_offset = (dx * sy - dy * sx) / determinant;
    const double second_offset = (dx * ry - dy * rx) / determinant;
    if (first_offset >= 0 && first_offset <= first_length && second_offset >= 0 &&
        second_offset <= second_length)
        return EdgeJunction{first_offset / first_length, second_offset / second_length};

    // A near T-junction is measured by perpendicular endpoint distance, not
    // distance along the branch's infinite extension (which depends on angle).
    if (const auto u = projection(a.a, b, tolerance))
        return EdgeJunction{0, *u};
    if (const auto u = projection(a.b, b, tolerance))
        return EdgeJunction{1, *u};
    if (const auto t = projection(b.a, a, tolerance))
        return EdgeJunction{*t, 0};
    if (const auto t = projection(b.b, a, tolerance))
        return EdgeJunction{*t, 1};
    return std::nullopt;
}

inline bool connected(const Edge& a, const Edge& b, double tolerance) {
    return distance(a.a, b.a) <= tolerance || distance(a.a, b.b) <= tolerance ||
           distance(a.b, b.a) <= tolerance || distance(a.b, b.b) <= tolerance ||
           junction(a, b, tolerance).has_value();
}

struct PointClusters {
    std::vector<std::size_t> ids;
    std::size_t count = 0;
};

inline PointClusters cluster_points(const std::vector<Point>& points, double tolerance) {
    std::vector<std::size_t> parents(points.size());
    for (std::size_t i = 0; i < parents.size(); ++i)
        parents[i] = i;
    const auto root = [&](std::size_t index) {
        while (parents[index] != index) {
            parents[index] = parents[parents[index]];
            index = parents[index];
        }
        return index;
    };
    for (std::size_t i = 0; i < points.size(); ++i)
        for (std::size_t j = 0; j < i; ++j)
            if (distance(points[i], points[j]) <= tolerance)
                parents[root(i)] = root(j);
    PointClusters result;
    std::map<std::size_t, std::size_t> cluster_ids;
    for (std::size_t i = 0; i < points.size(); ++i) {
        const auto representative = root(i);
        if (!cluster_ids.contains(representative))
            cluster_ids.emplace(representative, result.count++);
        result.ids.push_back(cluster_ids.at(representative));
    }
    return result;
}

} // namespace dsd::application::detail
