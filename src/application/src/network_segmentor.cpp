#include "application/network_segmentor.hpp"
#include "network_geometry.hpp"
#include "network_identity.hpp"
#include <deque>
#include <numbers>

namespace dsd::application {
namespace {
using namespace detail;

std::vector<Edge> split_at_equipment(const std::vector<Edge>& edges,
                                     const std::vector<Point>& locations, double tolerance) {
    std::vector<Edge> pieces;
    for (const auto& edge : edges) {
        std::vector<double> cuts{0, 1};
        for (const auto& location : locations) {
            const auto parameter = projection(location, edge, tolerance);
            if (!parameter)
                continue;
            const auto cut = point_at(edge, *parameter);
            // Equipment near an existing endpoint must not create a tiny extra duct.
            if (distance(cut, edge.a) > tolerance && distance(cut, edge.b) > tolerance)
                cuts.push_back(*parameter);
        }
        std::sort(cuts.begin(), cuts.end());
        cuts.erase(std::unique(cuts.begin(), cuts.end(),
                               [](double a, double b) { return std::abs(a - b) < epsilon; }),
                   cuts.end());
        for (std::size_t i = 1; i < cuts.size(); ++i)
            pieces.push_back({point_at(edge, cuts[i - 1]), point_at(edge, cuts[i]), edge.source});
    }
    return pieces;
}

struct Connections {
    std::vector<std::array<std::size_t, 2>> ports;
    std::vector<std::vector<std::size_t>> incident;
    std::vector<bool> equipment;
};

Connections build_connections(const std::vector<Edge>& pieces, const std::vector<Point>& equipment,
                              double tolerance) {
    std::vector<Point> points;
    for (const auto& edge : pieces) {
        points.push_back(edge.a);
        points.push_back(edge.b);
    }
    const auto clusters = cluster_points(points, tolerance);
    Connections connections;
    connections.incident.resize(clusters.count);
    connections.equipment.resize(clusters.count);
    for (std::size_t i = 0; i < pieces.size(); ++i) {
        const auto a = clusters.ids[2 * i], b = clusters.ids[2 * i + 1];
        connections.ports.push_back({a, b});
        connections.incident[a].push_back(i);
        connections.incident[b].push_back(i);
        for (int end = 0; end < 2; ++end) {
            const auto port = clusters.ids[2 * i + end];
            for (const auto& location : equipment)
                if (distance(points[2 * i + end], location) <= tolerance)
                    connections.equipment[port] = true;
        }
    }
    return connections;
}

struct Chain {
    std::deque<std::size_t> pieces;
    std::array<std::size_t, 2> ports;
    std::array<Point, 2> ends;
};

double turn_angle(const Edge& current, bool current_at_start, const Edge& next,
                  bool next_at_start) {
    const auto from = current_at_start ? current.b : current.a;
    const auto joint = current_at_start ? current.a : current.b;
    const auto start = next_at_start ? next.a : next.b;
    const auto to = next_at_start ? next.b : next.a;
    const double ax = joint.x - from.x, ay = joint.y - from.y;
    const double bx = to.x - start.x, by = to.y - start.y;
    const double cosine =
        std::clamp((ax * bx + ay * by) / (std::hypot(ax, ay) * std::hypot(bx, by)), -1.0, 1.0);
    return std::acos(cosine) * 180 / std::numbers::pi;
}

void extend_chain(Chain& chain, int side, std::size_t seed, const std::vector<Edge>& pieces,
                  const Connections& connections, std::vector<bool>& consumed, double threshold) {
    auto current = seed;
    while (connections.incident[chain.ports[side]].size() == 2 &&
           !connections.equipment[chain.ports[side]]) {
        const auto& incident = connections.incident[chain.ports[side]];
        const auto next = incident[0] == current ? incident[1] : incident[0];
        if (consumed[next])
            break;
        const bool current_at_start = connections.ports[current][0] == chain.ports[side];
        const bool next_at_start = connections.ports[next][0] == chain.ports[side];
        const double angle =
            turn_angle(pieces[current], current_at_start, pieces[next], next_at_start);
        if (!std::isfinite(angle) || angle > threshold + epsilon)
            break;
        consumed[next] = true;
        if (side == 0)
            chain.pieces.push_front(next);
        else
            chain.pieces.push_back(next);
        chain.ports[side] = connections.ports[next][next_at_start ? 1 : 0];
        chain.ends[side] = next_at_start ? pieces[next].b : pieces[next].a;
        current = next;
    }
}

LineInfo make_segment(const Chain& chain, const std::vector<Edge>& pieces,
                      const std::vector<LineInfo>& previous, std::size_t& next_id) {
    std::vector<SourcePiece> sources;
    for (const auto index : chain.pieces)
        sources.push_back(pieces[index].source);
    const auto old = std::find_if(previous.begin(), previous.end(), [&](const auto& segment) {
        return !segment.sources.empty() &&
               segment.sources.front().source_id == sources.front().source_id &&
               distance(segment.raw_line.start, chain.ends[0]) < epsilon &&
               distance(segment.raw_line.end, chain.ends[1]) < epsilon;
    });
    const bool reuse = old != previous.end();
    const auto id = reuse ? old->id : network_item_id("DUCT", next_id++);
    const auto metadata =
        reuse ? old->metadata : Metadata{{"flow", "0"}, {"width", "AC"}, {"height", "AC"}};
    return {id, geometry::Line{chain.ends[0], chain.ends[1]}, metadata, std::move(sources)};
}
} // namespace

std::vector<LineInfo> NetworkSegmentor::segment(
    const std::vector<SourcePath>& lines, const std::vector<geometry::Point>& split_points,
    const NetworkProcessingOptions& options, const std::vector<LineInfo>& previous_segments) const {
    detail::validate(options);
    for (const auto& line : lines)
        if (line.has_curves)
            throw std::invalid_argument("Curved polylines are not supported");
    const auto pieces =
        split_at_equipment(detail::edges(lines), split_points, options.connection_tolerance_mm);
    const auto connections =
        build_connections(pieces, split_points, options.connection_tolerance_mm);
    std::vector<bool> consumed(pieces.size());
    std::vector<LineInfo> result;
    auto next_id = detail::next_item_number(previous_segments);
    for (std::size_t seed = 0; seed < pieces.size(); ++seed) {
        if (consumed[seed])
            continue;
        consumed[seed] = true;
        Chain chain{{seed}, connections.ports[seed], {pieces[seed].a, pieces[seed].b}};
        extend_chain(chain, 0, seed, pieces, connections, consumed,
                     options.angle_threshold_degrees);
        extend_chain(chain, 1, seed, pieces, connections, consumed,
                     options.angle_threshold_degrees);
        if (detail::distance(chain.ends[0], chain.ends[1]) <= options.minimum_segment_length_mm)
            throw std::invalid_argument(
                "Segmentation produced a duct at or below the minimum valid length");
        result.push_back(make_segment(chain, pieces, previous_segments, next_id));
        result.back().raw_line.id = static_cast<int>(result.size());
    }
    return result;
}
} // namespace dsd::application
