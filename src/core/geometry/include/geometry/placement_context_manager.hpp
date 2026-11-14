#pragma once

#include "ductsystem/duct_fitting_tree.hpp"
#include "geometry/geometric_representation.hpp"
#include "geometry/placement_context.hpp"
#include "geometry/source_geometry_repository.hpp"
#include <stdexcept>
#include <utility>

namespace dsd::geometry {

class PlacementError : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
};

class PlacementContextManager {
public:
    PlacementContextManager(ductsystem::DuctFittingTree& tree, SourceGeometryRepository source);
    [[nodiscard]] ductsystem::DuctFittingTree& tree() const { return tree_; }
    void set_fitting_layouts(const GeometryRepresentations& layouts);
    [[nodiscard]] PlacementContext find_context(const ductsystem::SheetMetalComponent& component) const;

private:
    ductsystem::DuctFittingTree& tree_;
    const SourceGeometryRepository source_;
    GeometryRepresentations layouts_;

    ductsystem::SheetMetalNode& node_for(const ductsystem::SheetMetalComponent& component) const;
    const Line& source(const ductsystem::SheetMetalNode& node) const;
    std::shared_ptr<const ductsystem::SheetMetalNode> upstream(
        const ductsystem::SheetMetalNode& node) const;
    double extension(const ductsystem::SheetMetalNode& node) const;
    std::pair<double, double> skeleton_interval(
        const ductsystem::SheetMetalNode& fitting, const Line& source) const;
    PlacementContext duct_context(const ductsystem::SheetMetalNode& node) const;
};

} // namespace dsd::geometry
