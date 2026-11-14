#pragma once

#include "ductsystem/models/sheet_metal_component.hpp"
#include <optional>

namespace dsd::ductsystem {

struct RectangularSheetMetalComponent : SheetMetalComponent {
    std::optional<double> width; // mm
    std::optional<double> height; // mm

    RectangularSheetMetalComponent(int id, double flow, PressureLevel pressure_level,
                                  Material material, std::optional<double> width = std::nullopt,
                                  std::optional<double> height = std::nullopt);
};

} // namespace dsd::ductsystem
