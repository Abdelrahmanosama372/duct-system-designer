#pragma once

#include "ductsystem/fitting_sizing_config.hpp"
#include "ductsystem/models/rectangular/rectangular_duct.hpp"
#include "ductsystem/models/rectangular/rectangular_elbow.hpp"
#include "ductsystem/models/rectangular/rectangular_endcap.hpp"
#include "ductsystem/models/rectangular/rectangular_reducer.hpp"
#include "ductsystem/models/rectangular/rectangular_shoe_takeoff.hpp"
#include "geometry/geometric_representation.hpp"
#include "geometry/placement_context.hpp"
#include <functional>
#include <memory>
#include <stdexcept>

namespace dsd::geometry {

class GeometryConstructionError : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
};

class RectangularGeometricRepresentationBuilder {
public:
    explicit RectangularGeometricRepresentationBuilder(const ductsystem::FittingSizingConfig& config);
    [[nodiscard]] GeometricRepresentation build_duct(
        const ductsystem::RectangularDuct& component, const PlacementContext& context);
    [[nodiscard]] GeometricRepresentation build_elbow(
        const ductsystem::RectangularElbow& component, const PlacementContext& context);
    [[nodiscard]] GeometricRepresentation build_shoetakeoff(
        const ductsystem::RectangularShoeTakeoff& component, const PlacementContext& context);
    [[nodiscard]] GeometricRepresentation build_reducer(
        const ductsystem::RectangularReducer& component, const PlacementContext& context);
    [[nodiscard]] GeometricRepresentation build_endcap(
        const ductsystem::RectangularEndcap& component, const PlacementContext& context);

    [[nodiscard]] const std::map<int, std::shared_ptr<const GeometricRepresentation>>&
    normalized_representations() const { return normalized_; }

private:
    const ductsystem::FittingSizingConfig config_;
    std::map<int, std::shared_ptr<const GeometricRepresentation>> normalized_;
    std::map<int, std::vector<double>> keys_;

    GeometricRepresentation place(int id, std::vector<double> key,
        const PlacementContext& context,
        const std::function<GeometricRepresentation()>& factory);
};

} // namespace dsd::geometry
