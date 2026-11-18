#include "application/network_geometry_generator.hpp"
#include "ductsystem/visitors/geometry_constructor.hpp"
#include "geometry/placement_context_manager.hpp"
#include "geometry/rectangular_geometry_builder.hpp"
#include "sizing/equal_friction.hpp"
#include "sizing/equal_velocity.hpp"
#include <stdexcept>

namespace dsd::application {
namespace {
ductsystem::Material material(NetworkMaterial value) {
    switch (value) {
    case NetworkMaterial::GalvanizedSteel:
        return ductsystem::Material::GalvanizedSteel;
    case NetworkMaterial::StainlessSteel:
        return ductsystem::Material::StainlessSteel;
    case NetworkMaterial::Aluminum:
        return ductsystem::Material::Aluminum;
    case NetworkMaterial::CarbonSteel:
        return ductsystem::Material::CarbonSteel;
    }
    throw std::invalid_argument("Unsupported network material");
}
ductsystem::SystemType system_type(NetworkSystemType value) {
    switch (value) {
    case NetworkSystemType::Supply:
        return ductsystem::SystemType::Supply;
    case NetworkSystemType::Return:
        return ductsystem::SystemType::Return;
    case NetworkSystemType::Exhaust:
        return ductsystem::SystemType::Exhaust;
    }
    throw std::invalid_argument("Unsupported network system type");
}
} // namespace
NetworkGenerationResult
NetworkGeometryGenerator::generate(const DuctNetwork &network,
                                   const ductsystem::FittingSizingConfig &fittings) const {
    NetworkGenerationResult result;
    result.network_id = network.id;
    result.topology = topology_analyzer_.analyze(network);
    auto system =
        system_builder_.build(network.id, result.topology,
                              {material(network.material), ductsystem::PressureLevel::LowPressure,
                               fittings, system_type(network.system_type)});
    const auto config = network.sizing.to_sizer_config();
    std::unique_ptr<sizing::DuctSizer> sizer;
    if (network.sizing.method == NetworkSizingMethod::EqualFriction)
        sizer = std::make_unique<sizing::EqualFrictionDuctSizer>(config);
    else
        sizer = std::make_unique<sizing::EqualVelocityDuctSizer>(config);
    system_sizer_.size_ducts(system, *sizer);
    system_sizer_.insert_reducers(system);
    system_sizer_.size_fittings(system);
    std::map<int, geometry::Line> sources;
    for (const auto &info : result.topology.ducts) {
        sources.emplace(info.component_id, info.line);
        result.item_components[info.id].push_back(info.component_id);
    }
    // Fittings belong to their upstream source item; equipment also maps to its connection.
    for (const auto *node : system.preorder_traversal()) {
        if (sources.contains(node->component().id))
            continue;
        auto parent = node->parent().lock();
        while (parent && !sources.contains(parent->component().id))
            parent = parent->parent().lock();
        if (parent)
            for (const auto &info : result.topology.ducts)
                if (info.component_id == parent->component().id)
                    result.item_components[info.id].push_back(node->component().id);
    }
    const auto map_equipment = [&](const auto &items) {
        for (const auto &item : items) {
            const auto &duct = result.topology.ducts[item.upstream_duct];
            result.item_components[item.block.id] = result.item_components.at(duct.id);
        }
    };
    map_equipment(result.topology.air_terminals);
    map_equipment(result.topology.dampers);
    geometry::PlacementContextManager placement{
        system.dftree(), geometry::SourceGeometryRepository{std::move(sources)}};
    geometry::RectangularGeometricRepresentationBuilder builder{fittings, system.config().system_type};
    ductsystem::GeometryConstructorVisitor visitor{placement, builder};
    visitor.construct();
    result.geometry = visitor.representations();
    result.system = std::make_shared<const ductsystem::DuctSystem>(std::move(system));
    return result;
}
} // namespace dsd::application
