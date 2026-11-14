#include "ductsystem/duct_system_config.hpp"
#include "validation.hpp"

namespace dsd::ductsystem {

FittingSizingConfig::FittingSizingConfig(double takeoff, double reducer, double radius_ratio)
    : takeoff_length(takeoff), reducer_length(reducer), elbow_inside_radius_ratio(radius_ratio) {
    detail::positive(takeoff_length, "takeoff_length");
    detail::positive(reducer_length, "reducer_length");
    detail::positive(elbow_inside_radius_ratio, "elbow_inside_radius_ratio");
}

DuctSystemConfig::DuctSystemConfig(Material material_value, PressureLevel pressure,
                                   FittingSizingConfig fitting_config, SystemType type)
    : material(material_value), pressure_level(pressure), fittings(fitting_config), system_type(type) {
    detail::validate(material);
    detail::validate(pressure_level);
    detail::validate(system_type);
}

} // namespace dsd::ductsystem
