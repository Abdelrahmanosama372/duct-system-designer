#include "ductsystem/models/sheet_metal_component.hpp"
#include "validation.hpp"

namespace dsd::ductsystem {

SheetMetalComponent::SheetMetalComponent(int id_value, double flow_value,
                                         PressureLevel pressure, Material material_value)
    : id(id_value), flow(flow_value), pressure_level(pressure), material(material_value) {
    if (id < 0) throw std::invalid_argument("Component id must be nonnegative");
    if (!std::isfinite(flow) || flow < 0) {
        throw std::invalid_argument("Component flow must be finite and nonnegative (CFM)");
    }
    detail::validate(pressure_level);
    detail::validate(material);
}

} // namespace dsd::ductsystem
