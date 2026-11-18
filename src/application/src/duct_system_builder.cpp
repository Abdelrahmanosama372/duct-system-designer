#include "application/duct_system_builder.hpp"
#include "ductsystem/models/rectangular/rectangular_duct.hpp"
#include "ductsystem/models/rectangular/rectangular_elbow.hpp"
#include "ductsystem/models/rectangular/rectangular_endcap.hpp"
#include "ductsystem/models/rectangular/rectangular_shoe_takeoff.hpp"
#include <cmath>
#include <numbers>
#include <set>
#include <stdexcept>

namespace dsd::application {
using namespace ductsystem;
namespace {
double turn(const geometry::Line &a, const geometry::Line &b) {
    const double ax = a.end.x - a.start.x, ay = a.end.y - a.start.y;
    const double bx = b.end.x - b.start.x, by = b.end.y - b.start.y;
    return std::atan2(ax * by - ay * bx, ax * bx + ay * by);
}
} // namespace
DuctSystem DuctSystemBuilder::build(const std::string &name, const NetworkTopology &topology,
                                    const DuctSystemConfig &config) const {
    if (topology.ducts.empty())
        throw std::invalid_argument("Topology has no ducts");
    std::vector<std::shared_ptr<SheetMetalNode>> nodes;
    std::set<int> ids;
    int next_id = 0;
    for (std::size_t i = 0; i < topology.ducts.size(); ++i) {
        const auto &info = topology.ducts[i];
        if (!ids.insert(info.component_id).second ||
            (i == 0 ? info.parent.has_value() : !info.parent || *info.parent >= i))
            throw std::invalid_argument(
                "Topology must have unique IDs and one parent-ordered root");
        next_id = std::max(next_id, info.component_id + 1);
        nodes.push_back(std::make_shared<SheetMetalNode>(std::make_unique<RectangularDuct>(
            info.component_id, info.flow, config.pressure_level, config.material, info.width,
            info.height, info.line.length(), info.component_id)));
    }
    std::set<std::size_t> connected;
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        const auto &info = topology.ducts[i];
        bool straight = false, left = false, right = false;
        for (const auto child : info.children) {
            if (child >= nodes.size() || topology.ducts[child].parent != i ||
                !connected.insert(child).second)
                throw std::invalid_argument("Invalid topology child");
            const auto &downstream = topology.ducts[child];
            if (std::hypot(info.line.end.x - downstream.line.start.x,
                           info.line.end.y - downstream.line.start.y) > 1e-6)
                throw std::invalid_argument(info.id + ": child does not start at the junction");
            const auto angle = turn(info.line, downstream.line);
            if (std::abs(angle) < 1e-8) {
                if (straight)
                    throw std::invalid_argument(info.id + ": multiple straight continuations");
                straight = true;
                nodes[i]->add_child(nodes[child]);
            } else if (info.children.size() == 1) {
                auto elbow = std::make_shared<SheetMetalNode>(std::make_unique<RectangularElbow>(
                    next_id++, downstream.flow, config.pressure_level, config.material, angle));
                nodes[i]->add_child(elbow);
                elbow->add_child(nodes[child]);
            } else {
                if (std::abs(std::abs(angle) - std::numbers::pi / 2) > 1e-8)
                    throw std::invalid_argument(info.id + ": branches must be perpendicular");
                bool &occupied = angle > 0 ? left : right;
                if (occupied)
                    throw std::invalid_argument(info.id + ": duplicate branch side");
                occupied = true;
                auto shoe =
                    std::make_shared<SheetMetalNode>(std::make_unique<RectangularShoeTakeoff>(
                        next_id++, downstream.flow, config.pressure_level, config.material,
                        angle > 0 ? TakeoffSide::Left : TakeoffSide::Right, std::nullopt,
                        std::nullopt, downstream.component_id));
                nodes[i]->add_child(shoe);
                shoe->add_child(nodes[child]);
            }
        }
        if (info.children.size() > 1 && !straight)
            nodes[i]->add_child(
                std::make_shared<SheetMetalNode>(std::make_unique<RectangularEndcap>(
                    next_id++, 0, config.pressure_level, config.material)));
    }
    if (connected.size() + 1 != nodes.size())
        throw std::invalid_argument("Topology has disconnected ducts");
    std::set<std::size_t> terminal_ends;
    for (const auto &terminal : topology.air_terminals) {
        if (!terminal.at_duct_end)
            continue;
        if (terminal.upstream_duct >= nodes.size() ||
            !topology.ducts[terminal.upstream_duct].children.empty() ||
            !terminal_ends.insert(terminal.upstream_duct).second)
            throw std::invalid_argument(terminal.block.id + ": ambiguous terminal endpoint");
        nodes[terminal.upstream_duct]->add_child(
            std::make_shared<SheetMetalNode>(std::make_unique<RectangularElbow>(
                next_id++, topology.ducts[terminal.upstream_duct].flow, config.pressure_level,
                config.material, std::numbers::pi / 2, std::nullopt, std::nullopt,
                ElbowOrientation::Downward)));
    }
    return {name, DuctFittingTree{nodes.front()}, config};
}
} // namespace dsd::application
